#include "fusionui/fo4/Style.h"
#include "fusionui/fo4/Hud.h"
#include "fusionui/Paths.h"
#include "fusionui/modern/Theme.h"
#include "fusionframework/Config.h"

#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <mutex>
#include <optional>
#include <vector>

namespace fusionui::fo4 {
    namespace {
        // Flash sizes are em sizes; ImGui sizes are ascent + descent, which for Roboto (2400 / 2048 units)
        // is 1.172 em.
        constexpr float kEmToImGui = 2400.0F / 2048.0F;
        // FO4's embedded Roboto Condensed (fonts_en.swf) has the TTF's glyph shapes and cap height but
        // tighter advances: its "Leather Chest Piece" is 7.413 em against the TTF's 7.892 (bold 7.484 /
        // 8.038). Glyphs keep their size and are placed at FO4's advances.
        constexpr float kAdvanceRegular = 7.413F / 7.892F, kAdvanceBold = 7.484F / 8.038F;
        float AdvanceScale(bool bold) { return bold ? kAdvanceBold : kAdvanceRegular; }
        struct Frame {
            float scale = 1.0F;
            float base = 1.0F;  // scale before the current part's transform
            ImVec2 origin{};
            StageSize stage{};
            ImDrawList* list{};
            hud::Edge edge = hud::Edge::Center;
            hud::LayoutId part = hud::LayoutId::None;
            int rangeStart = -1;  // first vertex the current part drew since its last Anchor
        };
        Frame& Current() { return common::ContextSlot<Frame>(); }

        namespace hl = core::hudlayout;
        struct Parts {
            hl::Layout layout;
            hl::Family family{};
            const hl::Layout* preview{};
            float hiddenAlpha = 0.0F;
            // Boxes from the last frame each part drew: unscaled stage units (the scale pivot) and screen pixels.
            std::array<hl::Rect, hl::kCount> local{}, screen{};
            std::array<int, hl::kCount> drawnFrame{};
        };
        Parts& CurrentParts() { return common::ContextSlot<Parts>(); }

        struct SavedLayouts {
            std::mutex mutex;
            std::filesystem::path file;
            std::optional<std::array<hl::Layout, 5>> layouts;
        };
        SavedLayouts& SavedStore() {
            static SavedLayouts saved;
            return saved;
        }

        void CloseRange() {
            auto& f = Current();
            if (f.part == hud::LayoutId::None || !f.list || f.rangeStart < 0) return;
            auto& parts = CurrentParts();
            const auto index = static_cast<std::size_t>(f.part);
            const bool hidden = !parts.layout[f.part].visible && hl::Editable(parts.family, f.part);
            hl::Rect box{FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
            auto& vertices = f.list->VtxBuffer;
            for (int i = f.rangeStart; i < vertices.Size; ++i) {
                auto& v = vertices[i];
                box = {std::min(box.x0, v.pos.x), std::min(box.y0, v.pos.y), std::max(box.x1, v.pos.x), std::max(box.y1, v.pos.y)};
                if (hidden) {
                    const auto a = static_cast<ImU32>(static_cast<float>(v.col >> IM_COL32_A_SHIFT) * parts.hiddenAlpha);
                    v.col = (v.col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
                }
            }
            f.rangeStart = -1;
            if (box.Empty()) return;
            const hl::Rect local{(box.x0 - f.origin.x) / f.scale, (box.y0 - f.origin.y) / f.scale, (box.x1 - f.origin.x) / f.scale,
                                 (box.y1 - f.origin.y) / f.scale};
            const int frame = ImGui::GetFrameCount();
            if (parts.drawnFrame[index] != frame) {
                parts.drawnFrame[index] = frame;
                parts.local[index] = local;
                parts.screen[index] = box;
            } else {
                parts.local[index] = hl::Union(parts.local[index], local);
                parts.screen[index] = hl::Union(parts.screen[index], box);
            }
        }

        hl::Family FamilyOf(modern::theme::Family family) {
            using F = modern::theme::Family;
            switch (family) {
            case F::Fallout4: return hl::Family::Fo4;
            case F::Fallout3:
            case F::NewVegas: return hl::Family::Pipboy3000;
            case F::Skyrim: return hl::Family::Skyrim;
            case F::Starfield: return hl::Family::Starfield;
            default: return hl::Family::Fo76;
            }
        }

        ImFont* FontOf(bool bold) {
            const auto& fonts = common::CurrentFonts();
            return bold && fonts.bold ? fonts.bold : fonts.regular ? fonts.regular : ImGui::GetFont();
        }

        std::string Upper(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }

        void Triangle(ImVec2 a, ImVec2 b, ImVec2 c, ImU32 color) { Canvas()->AddTriangleFilled(Stage(a.x, a.y), Stage(b.x, b.y), Stage(c.x, c.y), color); }

        void Rect(float x0, float y0, float x1, float y1, ImU32 color) { Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), color); }
    }

    void InitContext() {
        // fo4 menus navigate themselves, as FO4's do; ImGui's nav would fight them for the same keys.
        ImGui::GetIO().ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);
        const auto apply = [](float) {
            auto& s = ImGui::GetStyle();
            s.WindowPadding = {0, 0};
            s.WindowBorderSize = 0.0F;
            s.WindowRounding = 0.0F;
            s.Colors[ImGuiCol_WindowBg] = {0, 0, 0, 0};
        };
        ImGui::GetStyle() = ImGuiStyle();
        apply(common::UiScale());
        common::SetStyleRebuild(apply);
    }

    float Px(float units) { return units * Current().scale; }

    ImVec2 Stage(float x, float y) {
        const auto& f = Current();
        return {f.origin.x + x * f.scale, f.origin.y + y * f.scale};
    }

    ImDrawList* Canvas() {
        auto& f = Current();
        return f.list ? f.list : ImGui::GetForegroundDrawList();
    }

    namespace {
        void BeginLayer(const char* id, StageSize stage, ImGuiWindowFlags flags) {
            const auto& io = ImGui::GetIO();
            auto& f = Current();
            f.stage = stage;
            f.scale = std::min(io.DisplaySize.x / stage.w, io.DisplaySize.y / stage.h);
            f.base = f.scale;
            f.origin = {(io.DisplaySize.x - stage.w * f.scale) / 2, (io.DisplaySize.y - stage.h * f.scale) / 2};
            f.part = hud::LayoutId::None;
            f.rangeStart = -1;
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin(id, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | flags);
            f.list = ImGui::GetWindowDrawList();
        }
    }

    void BeginMenu(const char* id, float dim, StageSize stage) {
        BeginLayer(id, stage, ImGuiWindowFlags_NoScrollWithMouse);
        const auto& io = ImGui::GetIO();
        if (dim > 0.0F) Canvas()->AddRectFilled({0, 0}, io.DisplaySize, IM_COL32(0, 0, 0, static_cast<int>(dim * 255)));
    }

    StageSize CurrentStage() { return Current().stage; }

    void EndMenu() {
        Current().list = nullptr;
        ImGui::End();
    }

    namespace hud {
        void Begin(const char* id, StageSize stage) {
            BeginLayer(id, stage, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus);
            // HUD groups hug the screen edges at any aspect, scaled by height like FO4's and FO76's HUDs.
            auto& f = Current();
            f.scale = f.base = ImGui::GetIO().DisplaySize.y / stage.h;
            auto& parts = CurrentParts();
            parts.family = layout::CurrentFamily();
            parts.layout = parts.preview ? *parts.preview : layout::Saved(parts.family);
            Anchor(Edge::Center);
        }

        void Anchor(Edge edge) {
            CloseRange();
            auto& f = Current();
            f.edge = edge;
            const float width = ImGui::GetIO().DisplaySize.x, stageWidth = f.stage.w * f.base;
            const float edgeX = edge == Edge::Left ? 0.0F : edge == Edge::Right ? width - stageWidth : (width - stageWidth) / 2;
            f.origin = {edgeX, 0.0F};
            f.scale = f.base;
            const auto& parts = CurrentParts();
            if (f.part == LayoutId::None || !hl::Editable(parts.family, f.part)) return;
            auto transform = parts.layout[f.part];
            const auto& box = parts.local[static_cast<std::size_t>(f.part)];
            // shortcut: a part's first frame has no box to pivot on, so it draws unscaled for that one frame.
            if (box.Empty()) transform.scale = 1;
            const auto pivot = hl::Pivot(box, edge == Edge::Left ? -1 : edge == Edge::Right ? 1 : 0, f.stage.h);
            const auto placed = hl::Place(edgeX, 0.0F, f.base, transform, pivot[0], pivot[1]);
            f.origin = {placed.x, placed.y};
            f.scale = placed.scale;
            f.rangeStart = f.list ? f.list->VtxBuffer.Size : -1;
        }

        void End() {
            CloseRange();
            Current().part = LayoutId::None;
            EndMenu();
        }

        Part::Part(LayoutId id) : previous_(Current().part) {
            CloseRange();
            Current().part = id;
            Anchor(Current().edge);
        }

        Part::~Part() {
            CloseRange();
            Current().part = previous_;
            Anchor(Current().edge);
        }

        namespace layout {
            hl::Family CurrentFamily() { return FamilyOf(modern::theme::CurrentAppearance().family); }

            std::filesystem::path File() {
                auto& saved = SavedStore();
                std::scoped_lock lock(saved.mutex);
                return saved.file.empty() ? AssetDir() / "hud_layout.toml" : saved.file;
            }

            void SetFile(std::filesystem::path file) {
                auto& saved = SavedStore();
                std::scoped_lock lock(saved.mutex);
                saved.file = std::move(file);
                saved.layouts.reset();
            }

            hl::Layout Saved(hl::Family family) {
                const auto file = File();
                auto& saved = SavedStore();
                std::scoped_lock lock(saved.mutex);
                if (!saved.layouts) {
                    const auto document = hl::Load(file);
                    saved.layouts.emplace();
                    for (std::size_t i = 0; i < saved.layouts->size(); ++i)
                        (*saved.layouts)[i] = hl::Read(document, static_cast<hl::Family>(i));
                }
                return (*saved.layouts)[static_cast<std::size_t>(family)];
            }

            void SetSaved(hl::Family family, const hl::Layout& layout) {
                Saved(family);
                auto& saved = SavedStore();
                std::scoped_lock lock(saved.mutex);
                (*saved.layouts)[static_cast<std::size_t>(family)] = layout;
            }

            void Use(const hl::Layout* layout, float hiddenAlpha) {
                auto& parts = CurrentParts();
                parts.preview = layout;
                parts.hiddenAlpha = hiddenAlpha;
            }

            hl::Rect Box(LayoutId id) {
                const auto& parts = CurrentParts();
                const auto index = static_cast<std::size_t>(id);
                return parts.drawnFrame[index] == ImGui::GetFrameCount() ? parts.screen[index] : hl::Rect{};
            }
        }

        ImVec2 ScreenToStage(ImVec2 pixel) {
            const auto& f = Current();
            return {(pixel.x - f.origin.x) / f.scale, (pixel.y - f.origin.y) / f.scale};
        }
    }

    ImU32 Fg(float alpha) {
        const auto hud = common::HudColor();
        return (hud & ~IM_COL32_A_MASK) | static_cast<ImU32>(std::clamp(alpha, 0.0F, 1.0F) * 255.0F + 0.5F) << IM_COL32_A_SHIFT;
    }

    float TextWidth(float size, std::string_view text, bool bold, float spacing) {
        auto* font = FontOf(bold);
        const float px = size * kEmToImGui;
        const float width = font->CalcTextSizeA(px, FLT_MAX, 0.0F, text.data(), text.data() + text.size()).x;
        int glyphs = 0;
        for (const char c : text)
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++glyphs;
        return width * AdvanceScale(bold) + spacing * static_cast<float>(std::max(0, glyphs - 1));
    }

    void Text(float x, float y, float size, std::string_view text, ImU32 color, bool bold, float spacing, Align align) {
        if (text.empty()) return;
        const float width = TextWidth(size, text, bold, spacing);
        if (align == Align::Center) x -= width / 2;
        else if (align == Align::Right) x -= width;
        auto* font = FontOf(bold);
        const float px = Px(size * kEmToImGui);
        auto* list = Canvas();
        const float advance = AdvanceScale(bold);
        // ImGui has neither letter spacing nor FO4's advances: place each UTF-8 glyph at the scaled width
        // of the text before it (one rounding, where per-glyph widths each round up a pixel) plus tracking.
        const char* begin = text.data();
        const char* end = begin + text.size();
        const auto origin = Stage(x, y);
        int index = 0;
        for (const char* p = begin; p < end; ++index) {
            const char* next = p + 1;
            while (next < end && (static_cast<unsigned char>(*next) & 0xC0) == 0x80) ++next;
            const float before = p == begin ? 0.0F : font->CalcTextSizeA(px, FLT_MAX, 0.0F, begin, p).x;
            list->AddText(font, px, {origin.x + before * advance + Px(spacing) * static_cast<float>(index), origin.y}, color, p, next);
            p = next;
        }
    }

    float TextWidthIn(ImFont* font, float size, std::string_view text) {
        if (!font) return TextWidth(size, text);
        return font->CalcTextSizeA(size * kEmToImGui, FLT_MAX, 0.0F, text.data(), text.data() + text.size()).x;
    }

    void TextIn(ImFont* font, float x, float y, float size, std::string_view text, ImU32 color, Align align) {
        if (!font) return Text(x, y, size, text, color, false, 0.0F, align);
        if (text.empty()) return;
        const float width = TextWidthIn(font, size, text);
        if (align == Align::Center) x -= width / 2;
        else if (align == Align::Right) x -= width;
        Canvas()->AddText(font, Px(size * kEmToImGui), Stage(x, y), color, text.data(), text.data() + text.size());
    }

    Nav ReadNav() {
        const auto held = [](std::initializer_list<ImGuiKey> keys) {
            return std::ranges::any_of(keys, [](ImGuiKey k) { return ImGui::IsKeyPressed(k, true); });
        };
        const auto once = [](std::initializer_list<ImGuiKey> keys) {
            return std::ranges::any_of(keys, [](ImGuiKey k) { return ImGui::IsKeyPressed(k, false); });
        };
        Nav nav;
        if (held({ImGuiKey_UpArrow, ImGuiKey_W, ImGuiKey_GamepadDpadUp, ImGuiKey_GamepadLStickUp})) nav.dy -= 1;
        if (held({ImGuiKey_DownArrow, ImGuiKey_S, ImGuiKey_GamepadDpadDown, ImGuiKey_GamepadLStickDown})) nav.dy += 1;
        if (held({ImGuiKey_LeftArrow, ImGuiKey_A, ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadLStickLeft})) nav.dx -= 1;
        if (held({ImGuiKey_RightArrow, ImGuiKey_D, ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadLStickRight})) nav.dx += 1;
        nav.accept = once({ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_E, ImGuiKey_GamepadFaceDown});
        nav.cancel = once({ImGuiKey_Escape, ImGuiKey_Tab, ImGuiKey_GamepadFaceRight});
        return nav;
    }

    void Brackets(float x0, float x1, float topY, float bottomY, float gapFrom, float gapTo) {
        const auto fg = Fg();
        if (gapTo > gapFrom) {
            Rect(x0, topY - 1, gapFrom, topY + 1, fg);
            Rect(gapTo, topY - 1, x1, topY + 1, fg);
        } else {
            Rect(x0, topY - 1, x1, topY + 1, fg);
        }
        Rect(x0 - 1, topY - 1, x0 + 1, topY + 4, fg);
        Rect(x1 - 1, topY - 1, x1 + 1, topY + 4, fg);
        Rect(x0, bottomY - 1, x1, bottomY + 1, fg);
        Rect(x0 - 1, bottomY - 4, x0 + 1, bottomY + 1, fg);
        Rect(x1 - 1, bottomY - 4, x1 + 1, bottomY + 1, fg);
    }

    float ListHeader(float x, float y, std::string_view title, bool arrows) {
        const auto text = Upper(title);
        const float width = TextWidth(25.0F, text, true, 0.5F);
        Text(x + 19.0F, y + 3.0F, 25.0F, text, Fg(), true, 0.5F);
        if (arrows) {
            Triangle({x + 4.8F, y + 15.85F}, {x + 12.0F, y + 8.65F}, {x + 12.0F, y + 23.05F}, Fg());
            const float rx = x + 19.0F + width + 10.0F;
            Triangle({rx + 7.2F, y + 15.85F}, {rx, y + 8.65F}, {rx, y + 23.05F}, Fg());
        }
        return 19.0F + width + 10.0F + 14.3F;
    }

    void ScrollMarker(float x, float y, bool up) {
        constexpr float w = 11.85F, h = 4.6F, gap = 3.2F;
        auto* list = Canvas();
        for (int i = 0; i < 2; ++i) {
            const float top = y + static_cast<float>(i) * (h + gap);
            const ImVec2 points[3]{Stage(x, up ? top + h : top), Stage(x + w / 2, up ? top : top + h), Stage(x + w, up ? top + h : top)};
            list->AddPolyline(points, 3, Fg(), ImDrawFlags_None, Px(1.41F));
        }
    }

    int HintBar(std::span<const Hint> hints, float y, const HintBarStyle& s) {
        if (hints.empty()) return -1;
        const bool pad = common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD;
        std::vector<float> keyWidths, widths;
        std::vector<std::string> labels, keys;
        float total = 0.0F;
        for (const auto& hint : hints) {
            keys.push_back(std::string(hint.key) + ")");
            labels.push_back(Upper(hint.label));
            const float keyW = pad ? common::PadGlyphWidth(Px(s.glyphRadius), hint.padButton) / Current().scale
                                   : TextWidth(s.keySize, keys.back());
            keyWidths.push_back(keyW);
            widths.push_back(s.keyPad + keyW + s.keyGap + TextWidth(s.labelSize, labels.back(), s.labelBold) + s.tailPad);
            total += widths.back();
        }
        total += s.spacing * static_cast<float>(hints.size() - 1);
        const float left = s.centerX - total / 2;
        // Filled "[" and "]" beside the group.
        const float w = s.bracketWidth, t = s.bracketThickness, h = s.height;
        const float l0 = left - s.bracketGap - w, l1 = left - s.bracketGap;
        Rect(l0, y, l0 + t, y + h, Fg());
        Rect(l0, y, l1, y + t, Fg());
        Rect(l0, y + h - t, l1, y + h, Fg());
        const float r0 = left + total + s.bracketGap, r1 = r0 + w;
        Rect(r1 - t, y, r1, y + h, Fg());
        Rect(r0, y, r1, y + t, Fg());
        Rect(r0, y + h - t, r1, y + h, Fg());

        int clicked = -1;
        float x = left;
        for (std::size_t i = 0; i < hints.size(); ++i) {
            const auto color = Fg(hints[i].enabled ? 1.0F : 0.5F);
            const auto keyColor = hints[i].flashing ? Fg((hints[i].enabled ? 1.0F : 0.5F) * HintFlash(ImGui::GetTime())) : color;
            if (pad) common::PadGlyph(Canvas(), Stage(x + s.keyPad + keyWidths[i] / 2, y + h / 2), Px(s.glyphRadius), hints[i].padButton, keyColor);
            else Text(x + s.keyPad, y + s.keyDy, s.keySize, keys[i], keyColor);
            Text(x + s.keyPad + keyWidths[i] + s.keyGap, y + s.labelDy, s.labelSize, labels[i], color, s.labelBold);
            if (hints[i].enabled && ImGui::IsMouseHoveringRect(Stage(x, y), Stage(x + widths[i], y + h)) &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                clicked = static_cast<int>(i);
            x += widths[i] + s.spacing;
        }
        return clicked;
    }

    int MessageBox(std::string_view body, std::span<const char* const> buttons, int& selected, int cancelIndex, const Nav& nav) {
        constexpr float kWidth = 543.9F, kBodyWidth = 320.0F, kBodySize = 18.0F, kLine = 21.6F;
        constexpr float kButtonW = 450.0F, kButtonH = 30.0F, kButtonGap = 7.5F;
        // Word-wrap the body to FO4's 320-unit field.
        std::vector<std::string> lines{""};
        std::string_view rest = body;
        while (!rest.empty()) {
            const auto space = rest.find(' ');
            const auto word = rest.substr(0, space);
            const auto candidate = lines.back().empty() ? std::string(word) : lines.back() + " " + std::string(word);
            if (!lines.back().empty() && TextWidth(kBodySize, candidate) > kBodyWidth) lines.emplace_back(word);
            else lines.back() = candidate;
            rest = space == std::string_view::npos ? std::string_view{} : rest.substr(space + 1);
        }
        const int count = std::min<int>(static_cast<int>(buttons.size()), 5);
        const float listY = 18.0F + 4.0F + static_cast<float>(lines.size()) * kLine + 30.0F;
        const float buttonsH = static_cast<float>(count) * kButtonH + static_cast<float>(std::max(0, count - 1)) * kButtonGap;
        const float height = listY + buttonsH + 16.65F;
        const float left = 640.0F - kWidth / 2, top = 367.0F - height / 2;

        Canvas()->AddRectFilled(Stage(left, top), Stage(left + kWidth, top + height), IM_COL32(0, 0, 0, 229));
        Canvas()->AddRect(Stage(left, top), Stage(left + kWidth, top + height), Fg(), 0.0F, 0, Px(2.2F));
        for (std::size_t i = 0; i < lines.size(); ++i)
            Text(640.0F, top + 20.0F + static_cast<float>(i) * kLine, kBodySize, lines[i], Fg(), false, 0.0F, Align::Center);

        selected = std::clamp(selected, 0, std::max(0, count - 1));
        if (nav.dy != 0 && count > 0) selected = (selected + nav.dy + count) % count;
        int chosen = nav.accept ? selected : nav.cancel ? cancelIndex : -1;
        const bool moved = ImGui::GetIO().MouseDelta.x != 0.0F || ImGui::GetIO().MouseDelta.y != 0.0F;
        for (int i = 0; i < count; ++i) {
            const float bx = 640.0F - kButtonW / 2, by = top + listY + static_cast<float>(i) * (kButtonH + kButtonGap);
            if (ImGui::IsMouseHoveringRect(Stage(bx, by), Stage(bx + kButtonW, by + kButtonH))) {
                if (moved) selected = i;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) chosen = i;
            }
            const bool on = i == selected;
            if (on) Rect(bx, by, bx + kButtonW, by + kButtonH, Fg());
            Text(640.0F, by + 6.2F, 18.0F, Upper(buttons[static_cast<std::size_t>(i)]), on ? Black : Fg(), true, -0.5F, Align::Center);
        }
        return chosen;
    }
}
