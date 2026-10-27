#include "fusionui/modern/Widgets.h"

#include "fusionui/modern/Layout.h"
#include "fusionui/modern/GameArt.h"
#include "fusionui/Icons.h"
#include "fusionui/Crosshairs.h"
#include "fusionui/Common.h"

#include <Windows.h>
#include <shellapi.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <format>
#include <string>
#include <unordered_map>
#include <utility>

namespace fusionui::modern::w {
    namespace {
        // Full pill radius in the Modern family, square in the game families.
        float Pill(float height) { return theme::CurrentAppearance().family == theme::Family::Modern ? height / 2 : 0.0F; }

        void Glyph(ImDrawList* list, const char* glyph, ImVec2 center, float size, ImU32 color) {
            const auto& appearance = theme::CurrentAppearance();
            const auto key = art::IconKey(art::Pack(appearance.iconSet, appearance.family), glyph);
            if (!key.empty() && icons::HasArt(key)) {
                icons::Draw(list, key, center, size, color);
            } else {
                auto* font = theme::CurrentFonts().body;
                if (!font) font = ImGui::GetFont();
                const auto extent = font->CalcTextSizeA(size, FLT_MAX, 0, glyph);
                list->AddText(font, size, {center.x - extent.x / 2, center.y - extent.y / 2}, color, glyph);
            }
        }

        void Selection(ImDrawList* list, ImVec2 min, ImVec2 max) {
            const auto family = theme::CurrentAppearance().family;
            const float line = theme::Px(1), inset = theme::Px(7);
            if (family == theme::Family::Fallout76) {
                const ImVec2 points[]{min, {max.x - inset, min.y}, max, {min.x + inset, max.y}};
                list->AddConvexPolyFilled(points, 4, theme::SelectionFill());
            } else {
                list->AddRectFilled(min, max, theme::SelectionFill(), theme::Round(6));
            }
            switch (family) {
            case theme::Family::Skyrim:
                list->AddLine(min, {max.x, min.y}, theme::Border, line);
                list->AddLine({min.x, max.y}, max, theme::Border, line);
                break;
            case theme::Family::Starfield:
                list->AddRectFilled({min.x, min.y}, {min.x + theme::Px(4), max.y}, theme::Text);
                break;
            case theme::Family::Fallout3:
                list->AddRect({min.x - line * 3, min.y}, {max.x + line * 3, max.y}, theme::Accent, 0, 0, line);
                break;
            case theme::Family::NewVegas:
                list->AddLine({min.x, max.y + line * 3}, {max.x, max.y + line * 3}, theme::Accent, line);
                break;
            case theme::Family::Modern:
                list->AddRectFilled({min.x, min.y + inset}, {min.x + line * 3, max.y - inset}, theme::Accent, theme::Round(2));
                break;
            default: break;
            }
        }

        void PositionPopover(const char* id, float width) {
            if (!ImGui::IsPopupOpen(id)) return;
            const auto* viewport = ImGui::GetMainViewport();
            const auto anchor = ImGui::GetItemRectMax();
            const float margin = theme::Px(8);
            const float x = std::clamp(anchor.x - width, viewport->WorkPos.x + margin,
                                       viewport->WorkPos.x + viewport->WorkSize.x - width - margin);
            const float top = viewport->WorkPos.y, bottom = top + viewport->WorkSize.y;
            // An anchor in the lower half (a bottom-docked console) opens the popover upward.
            if (anchor.y > top + viewport->WorkSize.y / 2) {
                const float y = ImGui::GetItemRectMin().y - theme::Px(6);
                ImGui::SetNextWindowPos({std::floor(x), std::floor(y)}, ImGuiCond_None, {0, 1});
                ImGui::SetNextWindowSizeConstraints({width, 0}, {width, std::max(1.0F, y - top - margin)});
                return;
            }
            const float y = anchor.y + theme::Px(6);
            ImGui::SetNextWindowPos({std::floor(x), std::floor(y)});
            ImGui::SetNextWindowSizeConstraints({width, 0}, {width, std::max(1.0F, bottom - y - margin)});
        }

        const nlohmann::json& EmptyObject() {
            static const nlohmann::json empty = nlohmann::json::object();
            return empty;
        }
        const nlohmann::json& EmptyArray() {
            static const nlohmann::json empty = nlohmann::json::array();
            return empty;
        }

        ImU32 Alpha(ImU32 color, float alpha) {
            auto v = ImGui::ColorConvertU32ToFloat4(color);
            v.w *= alpha;
            return ImGui::ColorConvertFloat4ToU32(v);
        }

        bool Styled(const char* label, bool enabled, ImVec2 size, ImU32 fill, ImU32 hover, ImU32 active, ImU32 text) {
            ImGui::PushStyleColor(ImGuiCol_Button, fill);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
            ImGui::PushStyleColor(ImGuiCol_Text, text);
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::BeginDisabled(!enabled);
            const bool pressed = ImGui::Button(label, size);
            ImGui::EndDisabled();
            ImGui::PopStyleColor(5);
            return pressed;
        }
    }

    const nlohmann::json& Obj(const nlohmann::json& value, std::string_view key) {
        if (!value.is_object()) return EmptyObject();
        const auto found = value.find(key);
        return found != value.end() && found->is_object() ? *found : EmptyObject();
    }

    const nlohmann::json& Arr(const nlohmann::json& value, std::string_view key) {
        if (!value.is_object()) return EmptyArray();
        const auto found = value.find(key);
        return found != value.end() && found->is_array() ? *found : EmptyArray();
    }

    std::string Text(const nlohmann::json& value) {
        if (value.is_string()) return value.get<std::string>();
        if (value.is_boolean()) return value.get<bool>() ? "Yes" : "No";
        if (value.is_number_integer()) return std::to_string(value.get<std::int64_t>());
        if (value.is_number_unsigned()) return std::to_string(value.get<std::uint64_t>());
        if (value.is_number_float()) return Number(value.get<double>());
        if (value.is_null()) return "";
        return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    }

    std::string Str(const nlohmann::json& value, std::string_view key, std::string_view fallback) {
        if (!value.is_object()) return std::string(fallback);
        const auto found = value.find(key);
        if (found == value.end() || found->is_null()) return std::string(fallback);
        auto text = Text(*found);
        return text.empty() ? std::string(fallback) : text;
    }

    double Num(const nlohmann::json& value, std::string_view key, double fallback) {
        if (!value.is_object()) return fallback;
        const auto found = value.find(key);
        return found != value.end() && found->is_number() ? found->get<double>() : fallback;
    }

    bool Bool(const nlohmann::json& value, std::string_view key, bool fallback) {
        if (!value.is_object()) return fallback;
        const auto found = value.find(key);
        if (found == value.end()) return fallback;
        if (found->is_boolean()) return found->get<bool>();
        if (found->is_number()) return found->get<double>() != 0.0;
        return fallback;
    }

    std::string First(std::initializer_list<std::string> values, std::string_view fallback) {
        for (const auto& value : values)
            if (!value.empty()) return value;
        return std::string(fallback);
    }

    std::string Number(double value, int decimals) {
        if (!std::isfinite(value)) return "—";
        auto text = std::format("{:.{}f}", value, decimals);
        if (text.find('.') != std::string::npos) {
            while (!text.empty() && text.back() == '0') text.pop_back();
            if (!text.empty() && text.back() == '.') text.pop_back();
        }
        return text == "-0" ? "0" : text;
    }

    std::string Hex(std::string_view formID) { return formID.empty() ? "—" : "0x" + std::string(formID); }

    bool Matches(const nlohmann::json& value, std::string_view query) {
        if (query.empty()) return true;
        const auto text = value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        const auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
        return std::search(text.begin(), text.end(), query.begin(), query.end(),
                           [&](char a, char b) { return lower(a) == lower(b); }) != text.end();
    }

    FontScope::FontScope(ImFont* font, float cssSize) { ImGui::PushFont(font, theme::Px(cssSize)); }
    FontScope::~FontScope() { ImGui::PopFont(); }

    ImU32 ToneColor(Tone tone) {
        switch (tone) {
        case Tone::Accent: return theme::Accent;
        case Tone::Ok: return theme::Ok;
        case Tone::Warn: return theme::Warn;
        case Tone::Error: return theme::Error;
        default: return theme::Muted;
        }
    }

    void Title(std::string_view text) {
        FontScope font(theme::CurrentFonts().bold, theme::TitleSize);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
    }

    void Heading(std::string_view text, int count) {
        ImGui::Dummy({0, theme::Px(2)});
        {
            FontScope font(theme::CurrentFonts().bold, theme::SmallSize);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
            std::string upper(text);
            std::ranges::transform(upper, upper.begin(), [](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
            ImGui::TextUnformatted(upper.c_str());
            ImGui::PopStyleColor();
        }
        if (count >= 0) {
            ImGui::SameLine();
            Badge(std::to_string(count));
        }
        const auto* window = ImGui::GetCurrentWindow();
        const auto y = ImGui::GetCursorScreenPos().y - theme::Px(3);
        window->DrawList->AddLine({window->WorkRect.Min.x, y}, {window->WorkRect.Max.x, y}, theme::Border, 1.0F);
        const auto family = theme::CurrentAppearance().family;
        const float left = window->WorkRect.Min.x, right = window->WorkRect.Max.x, d = theme::Px(3);
        if (family == theme::Family::Skyrim) {
            for (const float x : {left + d, right - d}) {
                const ImVec2 diamond[]{{x, y - d}, {x + d, y}, {x, y + d}, {x - d, y}};
                window->DrawList->AddConvexPolyFilled(diamond, 4, theme::Muted);
            }
        } else if (family == theme::Family::NewVegas) {
            window->DrawList->AddLine({left, y + d}, {right, y + d}, theme::Border);
        } else if (family == theme::Family::Fallout3 || family == theme::Family::Starfield) {
            window->DrawList->AddLine({left, y - d}, {left, y + d}, theme::Accent, theme::Px(2));
            window->DrawList->AddLine({right, y - d}, {right, y + d}, theme::Accent, theme::Px(2));
        }
    }

    void Muted(std::string_view text) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    void Wrapped(std::string_view text, ImU32 color) {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    void Mono(std::string_view text, ImU32 color) {
        FontScope font(theme::CurrentFonts().mono, theme::MonoSize);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopStyleColor();
    }

    void Badge(std::string_view text, Tone tone) {
        FontScope font(theme::CurrentFonts().bold, 12.5F);
        const auto color = tone == Tone::Neutral ? theme::Muted : ToneColor(tone);
        const ImVec2 pad{theme::Px(7), theme::Px(2)};
        const auto size = ImGui::CalcTextSize(text.data(), text.data() + text.size());
        const auto pos = ImGui::GetCursorScreenPos();
        const ImVec2 box{size.x + pad.x * 2, size.y + pad.y * 2};
        auto* list = ImGui::GetWindowDrawList();
        list->AddRectFilled(pos, {pos.x + box.x, pos.y + box.y}, tone == Tone::Neutral ? theme::Input : Alpha(color, 0.16F), Pill(box.y));
        list->AddText({pos.x + pad.x, pos.y + pad.y}, color, text.data(), text.data() + text.size());
        ImGui::Dummy(box);
    }

    void Callout(std::string_view text, Tone tone) {
        const auto color = ToneColor(tone);
        const auto* window = ImGui::GetCurrentWindow();
        const float width = window->WorkRect.Max.x - ImGui::GetCursorScreenPos().x;
        const float pad = theme::Px(10);
        const auto start = ImGui::GetCursorScreenPos();
        auto* list = ImGui::GetWindowDrawList();
        list->ChannelsSplit(2);
        list->ChannelsSetCurrent(1);
        ImGui::SetCursorScreenPos({start.x + pad + theme::Px(3), start.y + pad});
        ImGui::PushTextWrapPos(start.x + width - pad - window->Pos.x + window->Scroll.x);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::Text);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        const float bottom = ImGui::GetItemRectMax().y + pad;
        list->ChannelsSetCurrent(0);
        list->AddRectFilled(start, {start.x + width, bottom}, Alpha(color, 0.10F), theme::Round(6));
        list->AddRectFilled(start, {start.x + theme::Px(3), bottom}, color, theme::Round(6), ImDrawFlags_RoundCornersLeft);
        list->ChannelsMerge();
        ImGui::SetCursorScreenPos(start);
        ImGui::Dummy({width, bottom - start.y});
    }

    void EmptyState(std::string_view text) {
        const auto avail = ImGui::GetContentRegionAvail();
        const auto size = ImGui::CalcTextSize(text.data(), text.data() + text.size(), false, avail.x * 0.8F);
        ImGui::SetCursorPos({ImGui::GetCursorPosX() + std::max(0.0F, (avail.x - size.x) / 2),
                             ImGui::GetCursorPosY() + std::max(theme::Px(12), std::min(avail.y * 0.3F, theme::Px(80)))});
        ImGui::PushStyleColor(ImGuiCol_Text, theme::Faint);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + avail.x * 0.8F);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    bool Button(const char* label, bool enabled, ImVec2 size) {
        ImGui::BeginDisabled(!enabled);
        const bool pressed = ImGui::Button(label, size);
        ImGui::EndDisabled();
        return pressed;
    }

    bool Primary(const char* label, bool enabled, ImVec2 size) {
        return Styled(label, enabled, size, theme::Accent, theme::AccentHover, theme::AccentMuted, theme::OnAccent);
    }

    bool Danger(const char* label, bool enabled, ImVec2 size) {
        return Styled(label, enabled, size, theme::ErrorFill, Alpha(theme::Error, 0.30F), Alpha(theme::Error, 0.45F), theme::Error);
    }

    namespace {
    bool IconTinted(const char* id, const char* glyph, const char* tooltip, bool active, bool enabled, ImU32 ink) {
        const float side = ImGui::GetFrameHeight();
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Button, active ? theme::AccentSoft : IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? theme::Accent : theme::Muted);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, ImGui::GetStyle().FramePadding.y});
        ImGui::BeginDisabled(!enabled);
        const bool pressed = ImGui::Button("##icon", {side, side});
        const auto pos = ImGui::GetItemRectMin();
        Glyph(ImGui::GetWindowDrawList(), glyph, {pos.x + side / 2, pos.y + side / 2},
              ImGui::GetFontSize(), ImGui::GetColorU32(ink));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (tooltip) Tooltip(tooltip);
        ImGui::PopID();
        return pressed;
    }
    }

    bool Icon(const char* id, const char* glyph, const char* tooltip, bool active, bool enabled) {
        return IconTinted(id, glyph, tooltip, active, enabled, active ? theme::Accent : theme::Muted);
    }

    bool NavigationItem(const char* id, const char* label, const char* glyph, bool selected, bool* expanded) {
        if (expanded) ImGui::BeginGroup();
        const auto pos = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x, height = theme::Px(44);
        const float toggleWidth = expanded ? height : 0.0F;
        ImGui::PushStyleColor(ImGuiCol_Header, {0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0, 0, 0, 0});
        const bool pressed = ImGui::Selectable(id, selected, ImGuiSelectableFlags_None, {width - toggleWidth, height});
        ImGui::PopStyleColor(3);
        auto* list = ImGui::GetWindowDrawList();
        const ImVec2 end{pos.x + width, pos.y + height};
        if (selected) {
            Selection(list, pos, end);
        } else if (ImGui::IsItemHovered()) list->AddRectFilled(pos, end, theme::Surface, theme::Round(6));
        const float textSize = theme::Px(theme::BodySize), y = pos.y + (height - textSize) / 2;
        Glyph(list, glyph, {pos.x + theme::Px(22), pos.y + height / 2}, textSize,
              selected ? theme::SelectionText() : theme::Muted);
        list->PushClipRect({pos.x + theme::Px(44), pos.y}, {end.x - toggleWidth, end.y}, true);
        list->AddText(theme::CurrentFonts().body, textSize, {pos.x + theme::Px(44), y},
                      selected ? theme::SelectionText() : theme::Muted, label);
        list->PopClipRect();
        if (selected) ImGui::SetItemDefaultFocus();
        Tooltip(label);
        if (expanded) {
            ImGui::SameLine(0, 0);
            const float inset = (height - ImGui::GetFrameHeight()) / 2;
            ImGui::SetCursorScreenPos({end.x - height + inset, pos.y + inset});
            ImGui::PushID(id);
            if (IconTinted("pages", *expanded ? icon::Down : icon::Right,
                           *expanded ? "Hide pages" : "Show pages", false, true,
                           selected ? theme::SelectionText() : theme::Muted)) *expanded = !*expanded;
            ImGui::PopID();
            ImGui::EndGroup();
        }
        return pressed;
    }

    bool Switch(const char* label, bool on, bool enabled) {
        const float height = theme::Px(18), width = theme::Px(32);
        const auto pos = ImGui::GetCursorScreenPos();
        const auto labelSize = ImGui::CalcTextSize(label, nullptr, true);
        const float rowHeight = std::max(height, ImGui::GetFrameHeight());
        ImGui::BeginDisabled(!enabled);
        const bool pressed = ImGui::InvisibleButton(label, {width + theme::Px(10) + labelSize.x, rowHeight}, ImGuiButtonFlags_EnableNav);
        const bool hovered = ImGui::IsItemHovered();
        ImGui::EndDisabled();
        auto* list = ImGui::GetWindowDrawList();
        const float top = pos.y + (rowHeight - height) / 2;
        const auto track = on ? theme::Accent : (hovered ? theme::Border : theme::Input);
        list->AddRectFilled({pos.x, top}, {pos.x + width, top + height}, enabled ? track : theme::Input, Pill(height));
        list->AddRect({pos.x, top}, {pos.x + width, top + height}, on ? theme::Accent : theme::Border, Pill(height));
        const float knob = height / 2 - theme::Px(3);
        list->AddCircleFilled({on ? pos.x + width - height / 2 : pos.x + height / 2, top + height / 2}, knob,
                              on ? theme::OnAccent : theme::Muted);
        const char* end = ImGui::FindRenderedTextEnd(label);
        list->AddText({pos.x + width + theme::Px(10), pos.y + (rowHeight - labelSize.y) / 2}, enabled ? theme::Text : theme::Faint, label, end);
        return pressed && enabled;
    }

    int Segmented(const char* id, std::span<const char* const> labels, int current) {
        int clicked = -1;
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {theme::Px(6), ImGui::GetStyle().ItemSpacing.y});
        for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
            if (i > 0) ImGui::SameLine();
            ImGui::PushID(i);
            const bool selected = i == current;
            if (selected ? Primary(labels[static_cast<std::size_t>(i)]) : Button(labels[static_cast<std::size_t>(i)])) clicked = i;
            ImGui::PopID();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
        return clicked;
    }

    bool Search(const char* id, char* buffer, std::size_t size, const char* hint, float width) {
        const auto pos = ImGui::GetCursorScreenPos();
        const float iconSpace = theme::Px(28);
        ImGui::SetNextItemWidth(width);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {iconSpace, ImGui::GetStyle().FramePadding.y});
        const bool changed = ImGui::InputTextWithHint(id, hint, buffer, size);
        ImGui::PopStyleVar();
        const float height = ImGui::GetItemRectSize().y;
        Glyph(ImGui::GetWindowDrawList(), icon::Search, {pos.x + iconSpace / 2, pos.y + height / 2}, ImGui::GetFontSize(), theme::Muted);
        return changed;
    }

    void SliderLimits(ImVec2 frameMin, ImVec2 frameMax, const char* minText, const char* maxText, const char* valueText) {
        auto* font = ImGui::GetFont();
        const float size = ImGui::GetFontSize() * 0.85F;
        const auto width = [&](const char* text) { return font->CalcTextSizeA(size, FLT_MAX, 0.0F, text).x; };
        const float valueWidth = ImGui::CalcTextSize(valueText).x;
        const float padding = ImGui::GetStyle().FramePadding.x;
        if (!layout::SliderLimitsFit(frameMax.x - frameMin.x, width(minText), width(maxText), valueWidth, padding)) {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetItemTooltip("%s to %s", minText, maxText);
            return;
        }
        auto* list = ImGui::GetWindowDrawList();
        const float y = (frameMin.y + frameMax.y - size) / 2;
        list->AddText(font, size, {frameMin.x + padding, y}, theme::Muted, minText);
        list->AddText(font, size, {frameMax.x - padding - width(maxText), y}, theme::Muted, maxText);
    }

    bool SliderFloat(const char* id, float& value, float min, float max, const char* format, ImGuiSliderFlags flags) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool changed = ImGui::SliderFloat(id, &value, min, max, format, flags);
        char low[32], high[32], current[32];
        ImFormatString(low, sizeof(low), format, min);
        ImFormatString(high, sizeof(high), format, max);
        ImFormatString(current, sizeof(current), format, value);
        SliderLimits(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), low, high, current);
        return changed;
    }

    bool SliderInt(const char* id, int& value, int min, int max, const char* format, ImGuiSliderFlags flags) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool changed = ImGui::SliderInt(id, &value, min, max, format, flags);
        char low[32], high[32], current[32];
        ImFormatString(low, sizeof(low), format, min);
        ImFormatString(high, sizeof(high), format, max);
        ImFormatString(current, sizeof(current), format, value);
        SliderLimits(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), low, high, current);
        return changed;
    }

    int Pager(const char* id, int page, int pageCount) {
        int delta = 0;
        ImGui::PushID(id);
        if (Icon("prev", icon::Left, "Previous page", false, page > 0)) delta = -1;
        ImGui::SameLine(0, theme::Px(4));
        const auto label = std::format("{} / {}", page + 1, std::max(1, pageCount));
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(0, theme::Px(4));
        if (Icon("next", icon::Right, "Next page", false, page + 1 < pageCount)) delta = 1;
        ImGui::PopID();
        return delta;
    }

    void Tooltip(const char* text) {
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) return;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {theme::Px(8), theme::Px(6)});
        ImGui::SetTooltip("%s", text);
        ImGui::PopStyleVar();
    }

    bool ColorChooser(const char* id, std::array<float, 3>& rgb, const std::array<float, 3>& fallback, bool offerHud) {
        struct Preset {
            const char* name;
            std::array<float, 3> rgb;
        };
        static constexpr std::array kPresets{
            Preset{"Green (FO4 / FO76 HUD)", {18 / 255.0F, 1.0F, 21 / 255.0F}},
            Preset{"Amber", {1.0F, 0.71F, 0.26F}},
            Preset{"Blue", {0.18F, 0.81F, 1.0F}},   Preset{"White", {0.86F, 0.92F, 1.0F}},
            Preset{"Red", {1.0F, 0.31F, 0.31F}},    Preset{"Vault-Tec", {1.0F, 0.84F, 0.16F}},
            Preset{"FO76 Pip-Boy default", {0.1019608F, 0.5019608F, 1.0F}},
            Preset{"FO76 Quick-Boy / power armor default", {0.4823529F, 0.6666667F, 0.8509804F}},
            Preset{"FO76 mod menu", {0.49F, 0.99F, 0.42F}},
            Preset{"FO76 power armor mod menu", {1.0F, 0.82F, 0.41F}},
        };
        const auto same = [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            for (std::size_t i = 0; i < a.size(); ++i)
                if (std::abs(a[i] - b[i]) > 0.003F) return false;
            return true;
        };
        const auto hudColor = ImGui::ColorConvertU32ToFloat4(common::HudColor());
        const std::array hud{hudColor.x, hudColor.y, hudColor.z};
        const auto before = rgb;

        ImGui::PushID(id);
        ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, theme::Px(300)));
        ImGui::ColorEdit3("##edit", rgb.data(), ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
        const float swatch = theme::Px(24);
        for (std::size_t i = 0; i < kPresets.size(); ++i) {
            if (i) ImGui::SameLine(0, theme::Px(6));
            const auto& preset = kPresets[i];
            if (ImGui::ColorButton(preset.name, {preset.rgb[0], preset.rgb[1], preset.rgb[2], 1.0F}, ImGuiColorEditFlags_NoTooltip, {swatch, swatch}))
                rgb = preset.rgb;
            Tooltip(preset.name);
        }
        if (offerHud) {
            ImGui::SameLine(0, theme::Px(6));
            if (ImGui::ColorButton("HUD color", {hud[0], hud[1], hud[2], 1.0F}, ImGuiColorEditFlags_NoTooltip, {swatch, swatch})) rgb = hud;
            Tooltip("HUD color");
            if (Button("Match HUD", !same(rgb, hud))) rgb = hud;
            ImGui::SameLine();
        }
        if (Button("Reset", !same(rgb, fallback))) rgb = fallback;
        Tooltip(std::format("Default #{:02X}{:02X}{:02X}", std::lround(fallback[0] * 255), std::lround(fallback[1] * 255), std::lround(fallback[2] * 255))
                    .c_str());
        ImGui::PopID();
        return rgb != before;
    }

    bool BeginProperties(const char* id) {
        if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) return false;
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch, 0.42F);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.58F);
        return true;
    }

    void Property(std::string_view label, std::string_view value, bool mono, ImU32 color) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
        ImGui::TextUnformatted(label.data(), label.data() + label.size());
        ImGui::PopStyleColor();
        ImGui::TableSetColumnIndex(1);
        const std::string_view shown = value.empty() ? std::string_view("—") : value;
        if (mono) {
            Mono(shown, color);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::PushTextWrapPos(0.0F);
            ImGui::TextUnformatted(shown.data(), shown.data() + shown.size());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
    }

    void EndProperties() { ImGui::EndTable(); }

    bool AppearanceSettings(bool familyControlled) {
        auto appearance = theme::CurrentAppearance();
        bool changed = false, commit = false;
        if (!familyControlled) {
            ImGui::TextUnformatted("Theme");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##theme", theme::FamilyLabel(appearance.family).data())) {
                for (const auto& family : theme::Families) {
                    if (ImGui::Selectable(family.label.data(), family.family == appearance.family)) {
                        theme::SetFamily(family.family);
                        appearance.family = family.family;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            Muted(theme::FamilyDescription(appearance.family));
            if (Button("Use this theme for all B21 windows", true, {-1, 0})) theme::SetFamily(appearance.family, true);
            Tooltip("Every B21 window follows it, including ones that had their own theme.");
        }
        const auto packPicker = [&](const char* label, std::string& choice, bool cursor) {
            const auto name = [&](std::string_view key) -> std::string_view {
                if (key == "theme") return "Follow theme";
                if (key == "default") return cursor ? "Mod default" : "Font Awesome";
                if (key == "simple") return "Simple";
                for (const auto& family : theme::Families)
                    if (family.pack == key) return family.label;
                return key;
            };
            ImGui::TextUnformatted(label);
            ImGui::SetNextItemWidth(-1);
            ImGui::PushID(label);
            if (ImGui::BeginCombo("##pack", name(choice).data())) {
                const auto option = [&](std::string_view key) {
                    if (ImGui::Selectable(name(key).data(), choice == key)) {
                        choice = key;
                        changed = commit = true;
                    }
                };
                option("theme");
                option("default");
                if (cursor) option("simple");
                for (const auto& pack : art::Cursors)
                    if (pack.pack != "fo3" && icons::HasArt(pack.key)) option(pack.pack);
                ImGui::EndCombo();
            }
            ImGui::PopID();
        };
        packPicker("Cursor", appearance.cursor, true);
        packPicker("Icons", appearance.iconSet, false);
        const auto slider = [&](const char* label, float& value, float low, float high, float display, const char* format) {
            ImGui::TextUnformatted(label);
            float shown = value * display;
            if (SliderFloat(std::format("##{}", label).c_str(), shown, low * display, high * display, format)) {
                value = shown / display;
                changed = true;
            }
            commit |= ImGui::IsItemDeactivatedAfterEdit();
        };
        slider("Background opacity", appearance.backgroundOpacity, theme::MinBackgroundOpacity, 1.0F, 100.0F, "%.0f%%");
        slider("Text size", appearance.textScale, theme::MinTextScale, theme::MaxTextScale, 100.0F, "%.0f%%");
        slider("Table text size", appearance.tableTextSize, theme::MinTableTextSize, theme::MaxTableTextSize, 1.0F, "%.0f px");
        if (Button("Reset appearance", true, {-1, 0})) {
            const auto family = appearance.family;
            appearance = theme::DefaultAppearance();
            appearance.family = family;
            changed = commit = true;
        }
        if (changed || commit) theme::SetAppearance(appearance, commit);
        return changed;
    }

    bool CrosshairPicker(std::string& choice) {
        bool changed = false;
        std::string label = choice == "theme" ? "Follow theme" : choice == "none" ? "None" : "Unavailable (using theme)";
        for (const auto& entry : crosshairs::Entries())
            if (entry.key == choice) label = entry.game + " / " + entry.label;
        ImGui::TextUnformatted("Crosshair");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##crosshair", label.c_str())) {
            for (const auto* option : {"theme", "none"}) {
                if (ImGui::Selectable(std::string_view(option) == "theme" ? "Follow theme" : "None", choice == option)) {
                    choice = option;
                    changed = true;
                }
            }
            for (const auto& entry : crosshairs::Entries()) {
                ImGui::PushID(entry.key.c_str());
                if (ImGui::Selectable((entry.game + " / " + entry.label).c_str(), entry.key == choice)) {
                    choice = entry.key;
                    changed = true;
                }
                Tooltip(entry.key.c_str());
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        const auto pos = ImGui::GetCursorScreenPos();
        const ImVec2 size{ImGui::GetContentRegionAvail().x, theme::Px(58)};
        auto* list = ImGui::GetWindowDrawList();
        list->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, theme::Surface, theme::Round(4));
        if (!crosshairs::Draw(list, {pos.x + size.x / 2, pos.y + size.y / 2}, theme::Px(36), 0, choice) && choice != "none")
            list->AddText({pos.x + theme::Px(10), pos.y + theme::Px(18)}, theme::Muted, "No art installed for this selection");
        ImGui::Dummy(size);
        struct FolderState { std::string error; };
        auto& folderError = common::ContextSlot<FolderState>().error;
        if (Button("Open crosshair folder")) folderError = crosshairs::OpenDirectory() ? "" : "Could not open the crosshair folder.";
        ImGui::SameLine();
        if (Button("Refresh")) crosshairs::Reload();
        Wrapped("Add/remove white transparent PNG or DDS files in a game folder, then Refresh. default.png overrides Follow theme.", theme::Muted);
        if (!folderError.empty()) Wrapped(folderError, theme::Warn);
        if (!crosshairs::Diagnostics().empty()) {
            Wrapped(std::format("Crosshair warnings ({})", crosshairs::Diagnostics().size()), theme::Warn);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                for (const auto& warning : crosshairs::Diagnostics()) ImGui::TextUnformatted(warning.c_str());
                ImGui::EndTooltip();
            }
        }
        return changed;
    }

    bool BeginAppWindow(const char* id, ImVec2 display, bool fullscreen, ImVec2 maxSize, ImVec2& size) {
        ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, display, theme::Translucent(theme::Scrim));
        size = display;
        if (!fullscreen) {
            size = {display.x * 0.92F, display.y * 0.90F};
            if (maxSize.x > 0) size.x = std::min(size.x, maxSize.x);
            if (maxSize.y > 0) size.y = std::min(size.y, maxSize.y);
            size = {std::floor(size.x), std::floor(size.y)};
        }
        ImGui::SetNextWindowPos({std::floor((display.x - size.x) / 2), std::floor((display.y - size.y) / 2)});
        ImGui::SetNextWindowSize(size);
        const auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fullscreen ? 0.0F : style.WindowRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, fullscreen ? 0.0F : style.WindowBorderSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;
        const bool open = ImGui::Begin(id, nullptr, flags);
        ImGui::PopStyleVar(4);
        return open;
    }

    bool FullscreenToggle(bool& fullscreen) {
        const bool shortcut = ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
        if (Icon("fullscreen", fullscreen ? icon::Compress : icon::Expand,
                 fullscreen ? "Exit full screen (Ctrl+Enter)" : "Full screen (Ctrl+Enter)") ||
            shortcut) {
            fullscreen = !fullscreen;
            return true;
        }
        return false;
    }

    // ---- title-bar kit

    namespace {
        struct ResizableWindow {
            bool placed = false;
            ImVec2 display{};
            std::string saved;
        };

        std::unordered_map<std::string, ResizableWindow>& ResizableWindows() {
            static std::unordered_map<std::string, ResizableWindow> windows;
            return windows;
        }

        std::unordered_map<std::string, bool>& SidebarChoices() {
            static std::unordered_map<std::string, bool> choices;
            return choices;
        }

        bool& SidebarChoice(std::string_view saveName) {
            auto& choices = SidebarChoices();
            const std::string key(saveName);
            auto found = choices.find(key);
            if (found == choices.end()) found = choices.emplace(key, theme::SavedWindowFlag(saveName, "SidebarCompact")).first;
            return found->second;
        }

        void FadeDrawList(ImDrawList* list, float opacity) {
            for (auto& vertex : list->VtxBuffer) vertex.col = FadeColor(vertex.col, opacity);
        }

        void FadeTree(ImGuiWindow* window, float opacity) {
            FadeDrawList(window->DrawList, opacity);
            for (auto* child : window->DC.ChildWindows)
                if (child->Active) FadeTree(child, opacity);
        }
    }

    bool BeginResizableAppWindow(const char* id, std::string_view saveName, ImVec2 display, ImVec2 minSize, ImVec2 defaultSize,
                                 ImVec2& size) {
        ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, display, theme::Translucent(theme::Scrim));
        auto& state = ResizableWindows()[std::string(saveName)];
        const ImVec2 minimum{std::min(minSize.x, display.x), std::min(minSize.y, display.y)};
        if (!state.placed || state.display.x != display.x || state.display.y != display.y) {
            layout::WindowRect rect;
            if (const auto stored = layout::ParseRect(state.placed ? state.saved : theme::SavedWindowText(saveName, "Rect"))) {
                rect = *stored;
            } else {
                rect.width = std::min(defaultSize.x, display.x * 0.92F);
                rect.height = std::min(defaultSize.y, display.y * 0.90F);
                rect.x = (display.x - rect.width) / 2;
                rect.y = (display.y - rect.height) / 2;
            }
            rect = layout::FitRect(rect, display.x, display.y, minimum.x, minimum.y);
            ImGui::SetNextWindowPos({std::floor(rect.x), std::floor(rect.y)});
            ImGui::SetNextWindowSize({std::floor(rect.width), std::floor(rect.height)});
            state.placed = true;
            state.display = display;
        }
        ImGui::SetNextWindowSizeConstraints(minimum, display);
        const auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, style.WindowBorderSize);
        constexpr auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;
        const bool open = ImGui::Begin(id, nullptr, flags);
        ImGui::PopStyleVar(3);
        const auto pos = ImGui::GetWindowPos();
        size = ImGui::GetWindowSize();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            auto text = layout::FormatRect({pos.x, pos.y, size.x, size.y});
            if (text != state.saved) {
                const bool first = state.saved.empty();
                state.saved = std::move(text);
                // The first frame only records where the window opened; a save waits for a real move or resize.
                if (!first) theme::SaveWindowText(saveName, "Rect", state.saved);
            }
        }
        return open;
    }

    bool QuickSettings(const char* id, QuickSettingsState& state, const std::function<void()>& rows, const char* tooltip, float width) {
        ImGui::PushID(id);
        // ImGui owns the popup's open state (outside clicks and Esc close it); state.open only mirrors it. A cog click
        // while the popup is open already closed it on mouse-down, so its release must not reopen it.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) state.openAtClick = state.open;
        if (Icon("cog", icon::Gear, tooltip, state.open) && !state.openAtClick) state.toggleRequested = true;
        if (state.toggleRequested && !ImGui::IsPopupOpen("##quick")) {
            state.toggleRequested = false;
            ImGui::OpenPopup("##quick");
        }
        const auto* viewport = ImGui::GetMainViewport();
        const float popoverWidth = std::min(theme::Px(width), viewport->WorkSize.x - theme::Px(16));
        PositionPopover("##quick", popoverWidth);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {theme::Px(16), theme::Px(14)});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {theme::Px(8), theme::Px(8)});
        state.open = ImGui::BeginPopup("##quick", ImGuiWindowFlags_NoSavedSettings);
        if (state.open) {
            if (std::exchange(state.toggleRequested, false)) ImGui::CloseCurrentPopup();
            Title("Quick settings");
            if (rows) {
                rows();
                ImGui::Separator();
            }
            Heading("Appearance");
            AppearanceSettings(state.familyControlled);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
        return state.open;
    }

    bool PeekButton(const char* id, Peek& peek) {
        const bool active = peek.Active();
        Icon(id, active ? icon::EyeSlash : icon::Eye,
             active ? "Show the window (click)" : "Peek through the window: click to toggle, hold to peek", active);
        if (ImGui::IsItemActivated()) peek.Press();
        if (ImGui::IsItemDeactivated()) peek.Release(ImGui::GetIO().MouseDownDurationPrev[ImGuiMouseButton_Left]);
        return peek.Active();
    }

    void FadeWindow(const char* windowName, float opacity) {
        if (opacity >= 1.0F) return;
        if (auto* window = ImGui::FindWindowByName(windowName); window && window->Active) FadeTree(window, opacity);
        FadeDrawList(ImGui::GetBackgroundDrawList(), opacity);
    }

    bool SidebarCompact(std::string_view saveName, float windowWidth) {
        return layout::EffectiveCompactSidebar(SidebarChoice(saveName), windowWidth / theme::Scale());
    }

    float SidebarWidth(bool compact) { return theme::Px(compact ? 60.0F : 236.0F); }

    int Sidebar(std::string_view saveName, std::span<const NavItem> items, int active, float windowWidth) {
        const auto& fonts = theme::CurrentFonts();
        bool& choice = SidebarChoice(saveName);
        const bool forced = layout::CompactSidebar(windowWidth / theme::Scale());
        const bool compact = choice || forced;
        auto* list = ImGui::GetWindowDrawList();

        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(compact ? std::floor((ImGui::GetWindowWidth() - ImGui::GetFrameHeight()) / 2) : theme::Px(14));
        if (Icon("burger", icon::Bars,
                 forced ? "Labels come back when the window is wider" : compact ? "Show page names" : "Icons only")) {
            choice = !choice;
            theme::SaveWindowFlag(saveName, "SidebarCompact", choice);
        }
        ImGui::Dummy({0, theme::Px(4)});

        int clicked = -1;
        const char* group = nullptr;
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const auto& item = items[static_cast<std::size_t>(i)];
            if (item.group && (!group || std::strcmp(group, item.group) != 0)) {
                const bool first = group == nullptr;
                group = item.group;
                if (compact) {
                    ImGui::Dummy({0, theme::Px(first ? 2.0F : 6.0F)});
                    if (!first) {
                        const auto at = ImGui::GetCursorScreenPos();
                        list->AddLine({at.x + theme::Px(14), at.y}, {at.x + width - theme::Px(14), at.y}, theme::Border);
                        ImGui::Dummy({0, theme::Px(6)});
                    }
                } else {
                    ImGui::Dummy({0, theme::Px(first ? 2.0F : 8.0F)});
                    ImGui::SetCursorPosX(theme::Px(18));
                    FontScope heading(fonts.bold, 12);
                    ImGui::PushStyleColor(ImGuiCol_Text, theme::Faint);
                    ImGui::TextUnformatted(group);
                    ImGui::PopStyleColor();
                }
            }
            const bool selected = i == active;
            ImGui::PushID(item.id);
            const auto pos = ImGui::GetCursorScreenPos();
            const float height = theme::Px(34);
            ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));
            if (ImGui::Selectable("##item", selected, ImGuiSelectableFlags_None, {width, height})) clicked = i;
            ImGui::PopStyleColor(3);
            const bool hovered = ImGui::IsItemHovered();
            const ImVec2 min{pos.x + theme::Px(8), pos.y}, max{pos.x + width - theme::Px(8), pos.y + height};
            if (selected) {
                Selection(list, min, max);
            } else if (hovered) {
                list->AddRectFilled(min, max, IM_COL32(255, 255, 255, 12), theme::Round(6));
            }
            const float textSize = theme::Px(theme::BodySize);
            const float y = pos.y + (height - textSize) / 2;
            const char* glyph = item.icon ? item.icon : "";
            if (compact) {
                Glyph(list, glyph, {(min.x + max.x) / 2, pos.y + height / 2}, textSize,
                      selected ? theme::SelectionText() : hovered ? theme::Text : theme::Faint);
                if (!item.count.empty()) {
                    const float countSize = theme::Px(10.0F);
                    const auto extent = fonts.body->CalcTextSizeA(countSize, FLT_MAX, 0, item.count.c_str());
                    if (extent.x <= (max.x - min.x) / 2 - theme::Px(2))
                        list->AddText(fonts.body, countSize, {max.x - theme::Px(3) - extent.x, min.y + theme::Px(2)},
                                      selected ? theme::SelectionText() : theme::Faint,
                                      item.count.c_str());
                }
                if (hovered) {
                    const auto tip = item.count.empty() ? std::string(item.label) : std::format("{}  ({})", item.label, item.count);
                    Tooltip(tip.c_str());
                }
            } else {
                Glyph(list, glyph, {min.x + theme::Px(20), pos.y + height / 2}, textSize,
                      selected ? theme::SelectionText() : theme::Faint);
                list->AddText(fonts.body, textSize, {min.x + theme::Px(40), y}, selected ? theme::SelectionText() : theme::Muted, item.label);
                if (!item.count.empty()) {
                    const float countSize = theme::Px(12.5F);
                    const auto extent = fonts.body->CalcTextSizeA(countSize, FLT_MAX, 0, item.count.c_str());
                    list->AddText(fonts.body, countSize, {max.x - theme::Px(10) - extent.x, pos.y + (height - countSize) / 2},
                                  selected ? theme::SelectionText() : theme::Faint,
                                  item.count.c_str());
                }
            }
            ImGui::PopID();
        }
        return clicked;
    }

    bool WindowLauncher(Client& self, std::string_view selfId) {
        const auto windows = InstalledWindows(selfId);
        if (windows.empty()) return false;
        if (Icon("windows", icon::Launch, "Open another B21 window")) {
            ImGui::OpenPopup("##windows");
        }
        bool switched = false;
        PositionPopover("##windows", std::min(theme::Px(236), ImGui::GetMainViewport()->WorkSize.x - theme::Px(16)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {theme::Px(8), theme::Px(8)});
        if (ImGui::BeginPopup("##windows")) {
            for (const auto* window : windows) {
                const auto label = std::format("{}  {}", window->glyph, window->label);
                if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, {0, theme::Px(30)}))
                    switched = SwitchToWindow(self, window->id);
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        return switched;
    }

    TableEvents Table(const char* id, std::span<const Column> columns, const nlohmann::json& rows, TableState& table,
                      const char* rowKey, float trailingWidth, const std::function<void(const nlohmann::json&, int)>& trailing,
                      float height) {
        TableEvents events;
        const int count = static_cast<int>(columns.size()) + (trailing ? 1 : 0);
        constexpr auto flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH |
                               ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_PadOuterX;
        if (!ImGui::BeginTable(id, count, flags, {0, height})) return events;
        ImGui::TableSetupScrollFreeze(0, 1);
        for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
            const auto& column = columns[static_cast<std::size_t>(i)];
            ImGuiTableColumnFlags columnFlags = column.width > 0 ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch;
            if (!column.sortable) columnFlags |= ImGuiTableColumnFlags_NoSort;
            if (column.sortable && table.sortKey == column.key) {
                columnFlags |= ImGuiTableColumnFlags_DefaultSort;
                if (!table.ascending) columnFlags |= ImGuiTableColumnFlags_PreferSortDescending;
            }
            // Stretch columns: readable text (names) gets more of the width than mono IDs.
            ImGui::TableSetupColumn(column.label, columnFlags, column.width > 0 ? theme::Px(column.width) : column.mono ? 0.7F : 1.3F);
        }
        if (trailing)
            ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoResize,
                                    theme::Px(trailingWidth));
        const float textSize = theme::TableTextSize();
        const float monoSize = textSize * theme::MonoSize / theme::BodySize;
        {
            FontScope font(theme::CurrentFonts().bold, textSize * theme::SmallSize / theme::BodySize);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();
        }

        if (auto* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsDirty) {
            if (specs->SpecsCount > 0) {
                const auto& spec = specs->Specs[0];
                const auto& column = columns[static_cast<std::size_t>(spec.ColumnIndex)];
                const bool ascending = spec.SortDirection != ImGuiSortDirection_Descending;
                if (table.sortKey != column.key || table.ascending != ascending) {
                    table.sortKey = column.key;
                    table.ascending = ascending;
                    events.sortChanged = true;
                }
            }
            specs->SpecsDirty = false;
        }

        ImGui::PushFont(theme::CurrentFonts().body, theme::Px(textSize));  // popped before EndTable: rows live in the table's child
        const float rowHeight = ImGui::GetTextLineHeight() + theme::Px(10);
        int index = 0;
        for (const auto& row : rows.is_array() ? rows : nlohmann::json::array()) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
            ImGui::PushID(index);
            const auto key = Str(row, rowKey);
            const bool selected = !key.empty() && key == table.selected;
            for (int c = 0; c < static_cast<int>(columns.size()); ++c) {
                const auto& column = columns[static_cast<std::size_t>(c)];
                ImGui::TableSetColumnIndex(c);
                auto text = column.text ? column.text(row) : Str(row, column.key, "—");
                if (c == 0) {
                    ImGui::AlignTextToFramePadding();
                    const auto label = std::format("{}##row", text.empty() ? "(unnamed)" : text);
                    // The Selectable only covers the cell content rect (inside CellPadding), so its own
                    // highlight left bands at the row's top and bottom; the row background fills the row.
                    const ImU32 selectedColor = ImGui::GetColorU32(ImGuiCol_Header);
                    const ImU32 hoveredColor = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
                    const ImU32 activeColor = ImGui::GetColorU32(ImGuiCol_HeaderActive);
                    ImGui::PushStyleColor(ImGuiCol_Header, 0u);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, 0u);
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, 0u);
                    const bool pressed = ImGui::Selectable(label.c_str(), selected,
                                                           ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap |
                                                               ImGuiSelectableFlags_AllowDoubleClick,
                                                           {0, rowHeight - ImGui::GetStyle().CellPadding.y * 2});
                    ImGui::PopStyleColor(3);
                    const bool navFocused = ImGui::IsItemFocused() && GImGui->NavCursorVisible;
                    if (ImGui::IsItemActive()) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, activeColor);
                    else if (ImGui::IsItemHovered() || navFocused) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, hoveredColor);
                    else if (selected) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, selectedColor);
                    // Controller/keyboard: the focused row is the selection (details follow the D-pad),
                    // and A/Enter on it is the double click.
                    if (navFocused && !key.empty() && key != table.selected) {
                        table.selected = key;
                        events.clicked = index;
                    }
                    if (pressed) {
                        table.selected = key;
                        events.clicked = index;
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) || GImGui->NavActivatePressedId == ImGui::GetItemID())
                            events.activated = index;
                    }
                    if (ImGui::IsItemHovered() && ImGui::GetCurrentContext()->HoveredIdTimer > 0.6F) {
                        if (ImGui::CalcTextSize(text.c_str()).x > ImGui::GetColumnWidth()) Tooltip(text.c_str());
                    }
                    continue;
                }
                ImGui::AlignTextToFramePadding();
                if (column.mono) {
                    FontScope mono(theme::CurrentFonts().mono, monoSize);
                    ImGui::PushStyleColor(ImGuiCol_Text, text == "—" ? theme::Faint : theme::Muted);
                    ImGui::TextUnformatted(text.c_str());
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, text == "—" ? theme::Faint : theme::Text);
                    ImGui::TextUnformatted(text.c_str());
                    ImGui::PopStyleColor();
                }
            }
            if (trailing) {
                ImGui::TableSetColumnIndex(static_cast<int>(columns.size()));
                trailing(row, index);
            }
            ImGui::PopID();
            ++index;
        }
        ImGui::PopFont();
        ImGui::EndTable();
        return events;
    }
}
