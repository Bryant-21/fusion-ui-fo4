#include "styles/fo76/InventoryModals.h"
#include "fusionui/fo76/Theme.h"
#include "fusionui/Common.h"
#include "fusionui/Icons.h"

#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace fusionui::fo76 {
    ImU32 theme::Hud(ImU32 fo76) { return HudTint(fo76, common::HudColor()); }

    using fo4::Align;
    using fo4::Canvas;
    using fo4::Px;
    using fo4::Stage;
    using fo4::TextWidth;

    namespace {
        ImU32 WithAlpha(ImU32 color, float alpha) {
            const float a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(alpha, 0.0F, 1.0F);
            return (color & ~IM_COL32_A_MASK) | static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT;
        }
        float AlphaOf(ImU32 color) { return static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F; }

        ImU32 Lerp(ImU32 a, ImU32 b, float t) {
            const auto ch = [&](int shift) {
                const float x = static_cast<float>((a >> shift) & 0xFF), y = static_cast<float>((b >> shift) & 0xFF);
                return static_cast<ImU32>(x + (y - x) * t + 0.5F) << shift;
            };
            return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | ch(IM_COL32_A_SHIFT);
        }

        void Rect(float x0, float y0, float x1, float y1, ImU32 color) { Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), color); }

        std::string Upper(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }

        bool MouseMoved() {
            const auto d = ImGui::GetIO().MouseDelta;
            return d.x != 0.0F || d.y != 0.0F;
        }

        // Black 85 % box with a faint ink outline (BackgroundBoxContainer_mc + BackgroundBoxStroke_mc).
        // The menu fades behind an open modal (SecureTrade's modalFadeOut), then the box.
        void ModalBox(float x0, float y0, float x1, float y1) {
            Canvas()->AddRectFilled({0, 0}, ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 140));
            Rect(x0, y0, x1, y1, theme::MenuPanel);
            Canvas()->AddRect(Stage(x0, y0), Stage(x1, y1), WithAlpha(theme::Hud(theme::Ink), 0.4F), 0.0F, 0, Px(1.0F));
        }
    }

    void ShadowText(float x, float y, float size, std::string_view text, ImU32 color, bool bold, float spacing, Align align,
                    const DropShadow& shadow) {
        if (text.empty()) return;
        const float unit = Px(1.0F);
        ShadowLayers(shadow, AlphaOf(color), [&](ImVec2 offset, ImU32 tint) {
            fo4::Text(x + offset.x / unit, y + offset.y / unit, size, text, tint, bold, spacing, align);
        });
        fo4::Text(x, y, size, text, color, bold, spacing, align);
    }

    void ShadowIcon(std::string_view key, float cx, float cy, float size, ImU32 tint, const DropShadow& shadow, float angle) {
        if (key.empty()) return;
        const auto centre = Stage(cx, cy);
        ShadowLayers(shadow, AlphaOf(tint), [&](ImVec2 offset, ImU32 color) {
            icons::Draw(Canvas(), key, {centre.x + offset.x, centre.y + offset.y}, Px(size), color, 1.0F, false, angle);
        });
        icons::Draw(Canvas(), key, centre, Px(size), tint, 1.0F, false, angle);
    }

    void RadialRect(float x0, float y0, float x1, float y1, ImU32 inner, ImU32 outer) {
        // A fan from the centre to points around the rectangle; each rim point takes the gradient colour at
        // its distance on the inscribed ellipse (Flash's radial box), clamped to the outer stop.
        const float cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, rx = (x1 - x0) / 2, ry = (y1 - y0) / 2;
        constexpr int kPerEdge = 6;
        std::vector<ImVec2> rim;
        rim.reserve(4 * kPerEdge);
        const std::array<ImVec2, 5> corners{{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}}};
        for (int e = 0; e < 4; ++e)
            for (int i = 0; i < kPerEdge; ++i) {
                const float t = static_cast<float>(i) / kPerEdge;
                rim.push_back({corners[e].x + (corners[e + 1].x - corners[e].x) * t, corners[e].y + (corners[e + 1].y - corners[e].y) * t});
            }
        auto* list = Canvas();
        const int n = static_cast<int>(rim.size());
        list->PrimReserve(n * 3, n + 1);
        const auto uv = list->_Data->TexUvWhitePixel;
        const auto base = static_cast<ImDrawIdx>(list->_VtxCurrentIdx);
        list->PrimWriteVtx(Stage(cx, cy), uv, inner);
        for (const auto& p : rim) {
            const float d = std::hypot((p.x - cx) / std::max(rx, 0.01F), (p.y - cy) / std::max(ry, 0.01F));
            list->PrimWriteVtx(Stage(p.x, p.y), uv, Lerp(inner, outer, std::min(d, 1.0F)));
        }
        for (int i = 0; i < n; ++i) {
            list->PrimWriteIdx(base);
            list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 1 + i));
            list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 1 + (i + 1) % n));
        }
    }

    void SelectionPlate(float x0, float y0, float x1, float y1, float glow) {
        if (glow > 0.0F) {
            const DropShadow halo{theme::Hud(theme::Gold), 0.0F, 0.0F, 16.0F, glow};
            const float unit = Px(1.0F);
            ShadowLayers(halo, 1.0F, [&](ImVec2 offset, ImU32 tint) {
                Rect(x0 + offset.x / unit, y0 + offset.y / unit, x1 + offset.x / unit, y1 + offset.y / unit, tint);
            });
        }
        RadialRect(x0, y0, x1, y1, theme::Hud(theme::GoldLight), theme::Hud(theme::Gold));
    }

    void CornerPlate(std::string_view label, const CornerPlateLook& look) {
        if (label.empty()) return;
        auto* list = Canvas();
        if (look.wedge) {
            // Shape 360 through its matrix: a 45 degree triangle off the corner, alpha 0 on the hypotenuse and
            // 229 at (-2.8, 1108.8) on the gradient axis, so 216 at the screen corner.
            const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            const ImVec2 points[3]{Stage(0.0F, 551.5F), Stage(529.6F, 1081.0F), Stage(0.0F, 1081.0F)};
            const ImU32 colors[3]{IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 216)};
            list->PrimReserve(3, 3);
            const auto base = static_cast<ImDrawIdx>(list->_VtxCurrentIdx);
            for (int i = 0; i < 3; ++i) list->PrimWriteVtx(points[i], uv, colors[i]);
            for (int i = 0; i < 3; ++i) list->PrimWriteIdx(static_cast<ImDrawIdx>(base + i));
        }
        constexpr float kHeight = 40.55F, kSlant = 54.8F;
        const float x0 = look.left, x1 = look.left + look.width, y0 = look.top, y1 = look.top + kHeight;
        const ImVec2 plate[4]{Stage(x0, y0), Stage(x1, y0), Stage(x1 - kSlant, y1), Stage(x0, y1)};
        const int first = list->VtxBuffer.Size;
        list->AddConvexPolyFilled(plate, 4, IM_COL32_WHITE);
        ImGui::ShadeVertsLinearColorGradientKeepAlpha(list, first, list->VtxBuffer.Size, Stage(look.gradientX0, y0), Stage(x1, y0),
                                                      theme::Hud(theme::PlateLight), theme::Hud(theme::Gold));
        const auto text = Upper(label);
        const float width = TextWidth(26.0F, text, true);
        const float size = width > look.textWidth - 4.0F ? 26.0F * (look.textWidth - 4.0F) / width : 26.0F;
        fo4::Text(look.textX + 2.0F, look.textY + 1.4F + (26.0F - size) / 2.0F, size, text, theme::DarkOnGold, true);
    }

    int HintBar(std::span<const fo4::Hint> hints, float y, float centerX, ImU32 baseInk, float scale, bool boldLabels) {
        // BSButtonHintBar: the strip starts BAR_Y_OFFSET (5) below the bar's origin and reaches BACKGROUND_PAD
        // (8) past the buttons, half on each side; buttons sit BUTTON_SPACING (20) apart.
        const float kOffset = 5.0F * scale, kPad = 8.0F * scale, kSpacing = 20.0F * scale, kHeight = 38.0F * scale;
        const float kKeySize = 26.0F * scale, kLabelSize = 26.0F * scale, kKeyGap = 6.0F * scale, kGlyph = 13.5F * scale;
        const float kTextDy = 4.0F * scale;
        if (hints.empty()) return -1;
        const bool pad = common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD;
        std::vector<std::string> keys, labels;
        std::vector<float> keyWidths, widths;
        float total = 0.0F;
        for (const auto& hint : hints) {
            keys.push_back(std::string(hint.key) + ")");
            labels.push_back(Upper(hint.label));
            keyWidths.push_back(pad ? common::PadGlyphWidth(Px(kGlyph), hint.padButton) / Px(1.0F) : TextWidth(kKeySize, keys.back()));
            widths.push_back(keyWidths.back() + kKeyGap + TextWidth(kLabelSize, labels.back(), boldLabels));
            total += widths.back();
        }
        total += kSpacing * static_cast<float>(hints.size() - 1);
        const float left = centerX - total / 2, top = y + kOffset;
        Rect(left - kPad / 2, top, left + total + kPad / 2, top + kHeight, IM_COL32(0, 0, 0, 102));

        int clicked = -1;
        float x = left;
        for (std::size_t i = 0; i < hints.size(); ++i) {
            const bool enabled = hints[i].enabled;
            const bool hover = enabled && ImGui::IsMouseHoveringRect(Stage(x, top), Stage(x + widths[i], top + kHeight));
            if (hover) {
                Rect(x - 2.0F * scale, top, x + widths[i] + 2.0F * scale, top + kHeight, theme::Hud(theme::Gold));
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) clicked = static_cast<int>(i);
            }
            const ImU32 ink = hover ? IM_COL32(0, 0, 0, 255) : WithAlpha(baseInk, enabled ? 1.0F : 0.5F);
            if (pad) common::PadGlyph(Canvas(), Stage(x + keyWidths[i] / 2, top + kHeight / 2), Px(kGlyph), hints[i].padButton, ink);
            else if (hover) fo4::Text(x, top + kTextDy, kKeySize, keys[i], ink);
            else ShadowText(x, top + kTextDy, kKeySize, keys[i], ink);
            const float lx = x + keyWidths[i] + kKeyGap;
            if (hover) fo4::Text(lx, top + kTextDy, kLabelSize, labels[i], ink, boldLabels);
            else ShadowText(lx, top + kTextDy, kLabelSize, labels[i], ink, boldLabels);
            x += widths[i] + kSpacing;
        }
        return clicked;
    }

    ModalResult Quantity(const QuantityModal& m, core::inventory::Quantity& q, const fo4::Nav& nav) {
        // ModalSetQuantity_mc, securetrade.swf: box 700..1220 x 414..708, header 28 bold gold at 428 (its tooltip
        // line is left empty), value box 888..1029 x 492..533.5 (32 bold), slider track 757..1162 x 551.5..597
        // in a cream frame with arrows, thumb 17 x 78, hint bar at 651.9.
        constexpr float kX0 = 700.0F, kX1 = 1220.0F, kY0 = 414.0F, kY1 = 708.0F;
        constexpr float kTrack0 = 757.0F, kTrack1 = 1162.0F, kTrackY0 = 551.5F, kTrackY1 = 597.0F, kThumbW = 17.0F;
        ModalResult result;
        ModalBox(kX0, kY0, kX1, kY1);
        ShadowText(960.0F, 428.0F, 28.0F, Upper(m.header), theme::Hud(theme::Gold), true, 0.0F, Align::Center);

        Rect(888.4F, 492.0F, 1028.9F, 533.5F, IM_COL32(0, 0, 0, 255));
        const auto value = std::to_string(q.value);
        ShadowText(958.6F, 492.5F, 32.0F, value, theme::Hud(theme::Ink), true, 0.0F, Align::Center);

        // Slider: cream frame, dark track, arrow heads in the frame's ends, thumb at the value.
        auto* list = Canvas();
        list->AddRect(Stage(717.6F, 543.9F), Stage(1202.2F, 605.0F), theme::Hud(theme::Ink), 0.0F, 0, Px(2.0F));
        Rect(kTrack0, kTrackY0, kTrack1, kTrackY1, IM_COL32(0x11, 0x14, 0x14, 255));
        const float mid = (kTrackY0 + kTrackY1) / 2;
        list->AddTriangleFilled(Stage(728.0F, mid), Stage(747.0F, kTrackY0 + 4.0F), Stage(747.0F, kTrackY1 - 4.0F), theme::Hud(theme::Ink));
        list->AddTriangleFilled(Stage(1191.0F, mid), Stage(1172.0F, kTrackY0 + 4.0F), Stage(1172.0F, kTrackY1 - 4.0F), theme::Hud(theme::Ink));
        const float thumbX = kTrack0 + (kTrack1 - kTrack0 - kThumbW) * q.Fraction();
        const DropShadow thumbGlow{theme::CardHeader | IM_COL32_A_MASK, 0.0F, 0.0F, 4.0F, 0.75F};
        const float unit = Px(1.0F);
        ShadowLayers(thumbGlow, 1.0F, [&](ImVec2 o, ImU32 tint) {
            Rect(thumbX + o.x / unit, 535.3F + o.y / unit, thumbX + kThumbW + o.x / unit, 613.6F + o.y / unit, tint);
        });
        Rect(thumbX, 535.3F, thumbX + kThumbW, 613.6F, theme::Hud(theme::Ink));

        int delta = nav.dx;
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1) || ImGui::IsKeyPressed(ImGuiKey_PageUp)) delta += q.Step();
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1) || ImGui::IsKeyPressed(ImGuiKey_PageDown)) delta -= q.Step();
        if (const float wheel = ImGui::GetIO().MouseWheel; wheel != 0.0F) delta += wheel > 0.0F ? 1 : -1;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(Stage(kTrack0, 530.0F), Stage(kTrack1, 620.0F))) {
            const float fx = (ImGui::GetIO().MousePos.x - Stage(kTrack0, 0.0F).x) / Px(kTrack1 - kTrack0);
            const int target = 1 + static_cast<int>(std::lround(std::clamp(fx, 0.0F, 1.0F) * static_cast<float>(q.maximum - 1)));
            delta += target - q.value;
        }
        if (delta != 0) result.changed = q.Adjust(delta);

        const std::array<fo4::Hint, 2> hints{{{FUSIONUI_PAD_A, "E", m.accept}, {FUSIONUI_PAD_B, "TAB", m.cancel}}};
        const int clicked = fo76::HintBar(hints, 651.9F, 959.0F);
        if (nav.accept || clicked == 0) result.choice = 0;
        else if (nav.cancel || clicked == 1) result.choice = 1;
        return result;
    }

    int Confirm(const ConfirmModal& m, int& selected, const fo4::Nav& nav) {
        // ModalConfirmPurchase_mc: box 714.5..1205.5 x 392..686, header 28 bold gold at 400.3, item line 28 at
        // 439.8, price 32 bold with its currency icon at 474, button list from 559.5.
        constexpr float kX0 = 714.5F, kX1 = 1205.5F, kY0 = 392.1F, kY1 = 686.2F;
        constexpr float kButtonW = 300.0F, kButtonH = 35.0F, kButtonPitch = 41.5F, kListTop = 559.5F;
        ModalBox(kX0, kY0, kX1, kY1);
        ShadowText(960.0F, 400.3F, 28.0F, Upper(m.header), theme::Hud(theme::Gold), true, 0.0F, Align::Center);
        float lineY = 439.8F;
        if (!m.warning.empty()) {
            ShadowText(960.0F, lineY, 22.0F, m.warning, theme::Hud(theme::Ink), false, 0.0F, Align::Center);
            lineY += 30.0F;
        }
        ShadowText(960.0F, lineY, 28.0F, m.item, theme::Hud(theme::Ink), false, 0.0F, Align::Center);
        if (!m.value.empty()) {
            const float vw = TextWidth(32.0F, m.value, true), w = 26.0F + 6.0F + vw;
            const float x = 960.0F - w / 2;
            ShadowIcon(m.currency, x + 13.0F, 496.0F, 26.0F, theme::Hud(theme::Ink));
            ShadowText(x + 32.0F, 474.3F, 32.0F, m.value, theme::Hud(theme::Ink), true);
        }

        const int count = static_cast<int>(m.buttons.size());
        const auto usable = [&](int i) { return i != m.disabled; };
        selected = std::clamp(selected, 0, std::max(0, count - 1));
        if (!usable(selected)) selected = (selected + 1) % std::max(count, 1);
        if (nav.dy != 0 && count > 0) {
            int next = selected;
            for (int step = 0; step < count; ++step) {
                next = (next + nav.dy + count) % count;
                if (usable(next)) break;
            }
            selected = next;
        }
        int chosen = nav.accept && usable(selected) ? selected : nav.cancel ? count - 1 : -1;
        for (int i = 0; i < count; ++i) {
            const float x0 = 960.0F - kButtonW / 2, y0 = kListTop + static_cast<float>(i) * kButtonPitch;
            const bool hover = ImGui::IsMouseHoveringRect(Stage(x0, y0), Stage(x0 + kButtonW, y0 + kButtonH));
            if (hover && usable(i)) {
                if (MouseMoved()) selected = i;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) chosen = i;
            }
            const auto label = Upper(m.buttons[static_cast<std::size_t>(i)]);
            if (i == selected) {
                SelectionPlate(x0, y0, x0 + kButtonW, y0 + kButtonH, 0.65F);
                fo4::Text(960.0F, y0 + 2.0F, 26.0F, label, IM_COL32(0, 0, 0, 255), false, 0.0F, Align::Center);
            } else {
                ShadowText(960.0F, y0 + 2.0F, 26.0F, label, usable(i) ? theme::Hud(theme::Ink) : theme::Unavailable, false, 0.0F, Align::Center);
            }
        }
        return chosen;
    }

    void ConditionMeter(float x0, float y0, float condition) {
        Rect(x0, y0, x0 + 160.0F, y0 + 24.0F, theme::MeterBack);
        const auto fill = core::inventory::ConditionBar(condition);
        const ImU32 over = theme::Hud(theme::Ink);
        if (fill.dim <= 0.0F) {
            Rect(x0 + 0.8F, y0 + 0.5F, x0 + 159.8F, y0 + 23.7F, over);
            return;
        }
        constexpr float kL = 7.1F, kR = 154.4F;
        Rect(x0 + kR - (kR - kL) * fill.dim, y0 + 4.7F, x0 + kR, y0 + 19.5F, theme::Hud(theme::Gold));
        if (fill.bright > 0.0F) Rect(x0 + kR - (kR - kL) * fill.bright, y0 + 9.2F, x0 + kR, y0 + 15.0F, over);
    }

    int Repair(const RepairModal& m, int& selected, const fo4::Nav& nav) {
        // ConfirmPanel.Build: MinWidthC 400, WidthPaddingC 32, 20 after the question and before / after the
        // buttons. The options use the confirm modal's 300 x 35 list (pitch 41.5); components are 22 pt, 17.25 in.
        constexpr float kRowH = 35.0F, kPitch = 41.5F, kCostPitch = 27.0F, kGap = 20.0F, kHints = 38.0F;
        constexpr float kMeterW = 160.0F, kArrow = 34.0F, kCnd = 50.0F;
        const auto header = Upper(m.header);
        const int count = static_cast<int>(m.options.size());
        const auto usable = [&](int i) { return m.options[static_cast<std::size_t>(i)].enabled; };
        selected = std::clamp(selected, 0, std::max(0, count - 1));
        if (count > 0 && !usable(selected))
            for (int i = 0; i < count; ++i)
                if (usable(i)) { selected = i; break; }
        if (nav.dy != 0 && count > 0) {
            int next = selected;
            for (int step = 0; step < count; ++step) {
                next = (next + nav.dy + count) % count;
                if (usable(next)) break;
            }
            if (usable(next)) selected = next;
        }
        const RepairOption* current = count > 0 ? &m.options[static_cast<std::size_t>(selected)] : nullptr;
        const bool ready = current && current->enabled;

        float rowW = 300.0F;
        for (const auto& option : m.options)
            rowW = std::max(rowW, TextWidth(26.0F, Upper(option.label)) + TextWidth(26.0F, option.target) + 40.0F);
        const float content = std::max({TextWidth(24.0F, header, true), rowW, kCnd + kMeterW * 2 + kArrow});
        const float width = std::max(400.0F, content + 32.0F);
        const std::size_t costs = current ? current->costs.size() : 0;
        const float height = 7.0F + 30.0F + kGap + 24.0F + kGap + static_cast<float>(count) * kPitch +
                             static_cast<float>(costs) * kCostPitch + (m.notice.empty() ? 0.0F : 30.0F) + kGap + kHints + kGap;
        const float x0 = 960.0F - width / 2, y0 = 540.0F - height / 2;
        Canvas()->AddRectFilled({0, 0}, ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 140));
        Rect(x0, y0, x0 + width, y0 + height, IM_COL32(0x11, 0x11, 0x11, 191));  // BGRect_mc: #111111 at 75 %

        const ImU32 ink = theme::Hud(theme::Ink);
        float y = y0 + 7.0F;
        ShadowText(960.0F, y, 24.0F, header, ink, true, 0.0F, Align::Center);
        y += 30.0F + kGap;
        const float meterX = 960.0F - (kCnd + kMeterW * 2 + kArrow) / 2;
        ShadowText(meterX, y - 1.0F, 22.0F, "CND", ink, true);
        ConditionMeter(meterX + kCnd, y, m.before);
        ShadowText(meterX + kCnd + kMeterW + kArrow / 2, y - 3.0F, 24.0F, ">", ink, true, 0.0F, Align::Center);
        ConditionMeter(meterX + kCnd + kMeterW + kArrow, y, ready ? current->after : m.before);
        y += 24.0F + kGap;

        int chosen = nav.accept && ready ? selected : nav.cancel ? count : -1;
        const float listX = 960.0F - rowW / 2;
        for (int i = 0; i < count; ++i) {
            const auto& option = m.options[static_cast<std::size_t>(i)];
            const float top = y + static_cast<float>(i) * kPitch;
            const bool hover = ImGui::IsMouseHoveringRect(Stage(listX, top), Stage(listX + rowW, top + kRowH));
            if (hover && option.enabled) {
                if (MouseMoved()) selected = i;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) chosen = i;
            }
            const auto label = Upper(option.label);
            if (i == selected && option.enabled) {
                SelectionPlate(listX, top, listX + rowW, top + kRowH, 0.65F);
                fo4::Text(listX + 10.0F, top + 2.0F, 26.0F, label, IM_COL32(0, 0, 0, 255));
                fo4::Text(listX + rowW - 10.0F, top + 2.0F, 26.0F, option.target, IM_COL32(0, 0, 0, 255), false, 0.0F, Align::Right);
            } else {
                const ImU32 color = option.enabled ? ink : theme::Unavailable;
                ShadowText(listX + 10.0F, top + 2.0F, 26.0F, label, color);
                ShadowText(listX + rowW - 10.0F, top + 2.0F, 26.0F, option.target, color, false, 0.0F, Align::Right);
            }
        }
        y += static_cast<float>(count) * kPitch;
        if (current) {
            // ConfirmPanelComponentEntry: components the player is short of show at DIMMED_ALPHA.
            const ImU32 color = current->costsShort ? WithAlpha(ink, 0.5F) : ink;
            for (const auto& cost : current->costs) {
                ShadowText(listX + 17.25F, y, 22.0F, cost, color);
                y += kCostPitch;
            }
        }
        if (!m.notice.empty()) {
            ShadowText(960.0F, y, 22.0F, m.notice, ink, false, 0.0F, Align::Center);
            y += 30.0F;
        }
        const std::array<fo4::Hint, 2> hints{{{FUSIONUI_PAD_A, "SPACE", m.confirm, ready}, {FUSIONUI_PAD_B, "TAB", m.cancel}}};
        const int clicked = fo76::HintBar(hints, y + kGap - 5.0F, 960.0F);
        if ((clicked == 0 || ImGui::IsKeyPressed(ImGuiKey_Space, false)) && ready) chosen = selected;
        else if (clicked == 1) chosen = count;
        return chosen;
    }
}
