#include "styles/fo76/MainMenu.h"
#include "fusionui/Icons.h"
#include "fusionui/fo4/Hud.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>

namespace fusionui::fo76::mainmenu {
    using fo4::Canvas;
    using fo4::Px;
    using fo4::Stage;
    namespace layout = core::mainmenu;

    namespace {
        // ImGui font pixels per fo4::Text size unit (fo4 Base.cpp kEmToImGui), for vertical centring.
        constexpr float kLine = 2400.0F / 2048.0F;

        float TextTop(float top, float height, float size) { return top + (height - size * kLine) / 2.0F; }

        // FO76's selection bar: gold from the screen edge, its right end slanting inward toward the bottom.
        void Bar(float y0, float y1, float topRight, float bottomRight, ImU32 color) {
            const ImVec2 points[]{Stage(-4000.0F, y0), Stage(topRight, y0), Stage(bottomRight, y1), Stage(-4000.0F, y1)};
            ShadowLayers(kGlow, static_cast<float>(color >> IM_COL32_A_SHIFT) / 255.0F, [&](ImVec2 offset, ImU32 shade) {
                ImVec2 shifted[4];
                for (int i = 0; i < 4; ++i) shifted[i] = {points[i].x + offset.x, points[i].y + offset.y};
                Canvas()->AddConvexPolyFilled(shifted, 4, shade);
            });
            Canvas()->AddConvexPolyFilled(points, 4, color);
        }

        // The title menu's banner (menulistcomponent MainMenuEntryLarge Backer_mc): pale yellow at the screen edge
        // shading to gold by x 500, then flat to its slanted end.
        void TitleBanner(float y0, float y1, const layout::ListMetrics& m, bool disabled) {
            auto* list = Canvas();
            const int first = list->VtxBuffer.Size;
            Bar(y0, y1, m.barTopRight, m.barBottomRight, disabled ? theme::Unavailable : IM_COL32(0xFE, 0xD7, 0x63, 0xFF));
            if (!disabled)
                ImGui::ShadeVertsLinearColorGradientKeepAlpha(list, first, list->VtxBuffer.Size, Stage(0.0F, y0), Stage(500.0F, y0),
                                                             IM_COL32(0xFE, 0xFE, 0x83, 0xFF), IM_COL32(0xFE, 0xD7, 0x63, 0xFF));
        }

        bool Hovered(const layout::Box& b) { return ImGui::IsMouseHoveringRect(Stage(b.x0 - 4000.0F, b.y0), Stage(b.x1, b.y1)); }

        ImU32 Alpha(ImU32 color, float alpha) {
            const auto a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * alpha;
            return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT);
        }

        void Spinner(float cx, float cy, float radius, ImU32 color) {
            const float t = static_cast<float>(ImGui::GetTime());
            for (int i = 0; i < 8; ++i) {
                const float a = t * 6.0F + static_cast<float>(i) * 0.785F;
                const float fade = static_cast<float>(i + 1) / 8.0F;
                Canvas()->AddLine(Stage(cx + std::cos(a) * radius * 0.45F, cy + std::sin(a) * radius * 0.45F),
                                  Stage(cx + std::cos(a) * radius, cy + std::sin(a) * radius), Alpha(color, fade), Px(2.5F));
            }
        }

        const layout::ListMetrics& MetricsOf(Kind kind) { return kind == Kind::Title ? layout::kTitle : layout::kPause; }

        // Title large rows: bold 60; minor rows: bold 24.5 (underlined when selected); pause rows: 33.
        float SizeOf(Kind kind, bool minor) { return kind == Kind::Title ? (minor ? 24.5F : 60.0F) : 33.0F; }
    }

    std::vector<layout::Box> RowBoxes(const List& list) {
        std::vector<bool> minor;
        for (const auto& row : list.rows) minor.push_back(list.kind == Kind::Title && row.minor);
        return layout::RowBoxes(MetricsOf(list.kind), minor);
    }

    ListResult DrawList(const List& list) {
        fo4::hud::Anchor(fo4::hud::Edge::Left);
        const auto& m = MetricsOf(list.kind);
        const auto boxes = fusionui::fo76::mainmenu::RowBoxes(list);
        // The title rows' text is FO76's fixed cream (menulistcomponent Text_tf #FEFEC8), not the HUD colour.
        const ImU32 ink = list.kind == Kind::Title ? IM_COL32(0xFE, 0xFE, 0xC8, 0xFF) : theme::Hud(theme::Ink);
        const ImU32 gold = theme::Hud(theme::Gold);
        ListResult result;
        for (std::size_t i = 0; i < list.rows.size(); ++i) {
            const auto& row = list.rows[i];
            const auto& box = boxes[i];
            const bool minor = list.kind == Kind::Title && row.minor;
            const bool selected = static_cast<int>(i) == list.selected;
            const float size = SizeOf(list.kind, minor);
            const float y = TextTop(box.y0, box.y1 - box.y0, size);
            const bool bold = list.kind == Kind::Title;
            if (Hovered(box)) {
                result.hovered = static_cast<int>(i);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) result.clicked = static_cast<int>(i);
            }
            if (selected && !minor && list.kind == Kind::Title) {
                TitleBanner(box.y0, box.y1, m, row.disabled);
                // Only FO76's own Vault Boy (the converted sprite sheet); without it the row shows none.
                const auto& art = row.art;
                fo4::Text(m.textX, y, size, row.label, theme::DarkOnGold, bold);
                // FO76 layers the Vault Boy over the label (Image_mc sits above Text_mc).
                if (art.texture)
                    Canvas()->AddImage(art.texture, Stage(m.textX + art.x, box.y0 + art.y),
                                       Stage(m.textX + art.x + art.width, box.y0 + art.y + art.height), art.uv0, art.uv1);
            } else if (selected && !minor) {
                Bar(box.y0, box.y1, m.barTopRight, m.barBottomRight, row.disabled ? theme::Unavailable : gold);
                fo4::Text(m.textX, y, size, row.label, theme::DarkOnGold, bold);
            } else {
                const ImU32 color = row.disabled ? theme::Unavailable : ink;
                ShadowText(m.textX, y, size, row.label, color, bold, 0.0F, fo4::Align::Left, kGlow);
                if (selected) {
                    const float width = fo4::TextWidth(size, row.label, bold);
                    const float under = y + size * kLine - 3.0F;
                    Canvas()->AddRectFilled(Stage(m.textX, under), Stage(m.textX + width, under + 2.0F), color);
                }
            }
            if (row.busy) Spinner(m.textX + fo4::TextWidth(size, row.label, bold) + 26.0F, (box.y0 + box.y1) / 2, 14.0F, ink);
        }
        return result;
    }

    SettingsResult DrawSettings(const SettingsPage& page, const fo4::Nav& nav) {
        fo4::hud::Anchor(fo4::hud::Edge::Left);
        SettingsResult result{page.selected, page.scroll};
        const auto& s = layout::kSettings;
        const int count = static_cast<int>(page.rows.size());

        if (page.category < 0) {
            // A save's details: one fixed-height box, the screenshot over its lines, so moving through saves never
            // resizes or shifts it.
            constexpr float x0 = 150.0F, x1 = 575.0F, y0 = 308.0F, y1 = 778.0F, pad = 12.0F;
            Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), IM_COL32(0x18, 0x1C, 0x1D, 166));
            Canvas()->AddRect(Stage(x0, y0), Stage(x1, y1), Alpha(theme::Hud(theme::Ink), 0.6F), 0.0F, 0, Px(1.5F));
            const float pw = x1 - x0 - 2 * pad, ph = pw * 9.0F / 16.0F;
            if (page.preview && page.previewWidth > 0 && page.previewHeight > 0) {
                const float scale = std::min(pw / page.previewWidth, ph / page.previewHeight);
                const float w = page.previewWidth * scale, h = page.previewHeight * scale;
                const float px = x0 + pad + (pw - w) / 2, py = y0 + pad + (ph - h) / 2;
                Canvas()->AddImage(page.preview, Stage(px, py), Stage(px + w, py + h));
            }
            Canvas()->PushClipRect(Stage(x0, y0), Stage(x1 - pad, y1), true);
            float y = y0 + pad + ph + 14.0F;
            for (const auto& line : page.categories) {
                if (y + 34.0F > y1) break;
                ShadowText(x0 + pad + 4.0F, y, 28.0F, line, theme::Hud(theme::Ink), false, 0.0F, fo4::Align::Left, kGlow);
                y += 38.0F;
            }
            Canvas()->PopClipRect();
        } else {
            // Left column: the categories, as the pause list draws them.
            List categories{Kind::Pause, {}, page.category};
            for (const auto& c : page.categories) categories.rows.push_back({c});
            fusionui::fo76::mainmenu::DrawList(categories);
        }

        std::vector<bool> disabled;
        for (const auto& r : page.rows) disabled.push_back(r.disabled);
        result.selected = layout::StepSelection(page.selected, nav.dy, disabled);
        if (count > 0 && result.selected >= 0 && result.selected < count && !page.rows[static_cast<std::size_t>(result.selected)].disabled) {
            const auto& row = page.rows[static_cast<std::size_t>(result.selected)];
            const int options = row.kind == Setting::Kind::Toggle ? 2 : static_cast<int>(row.options.size());
            int value = row.value;
            if (nav.dx != 0) value = layout::StepValue(value, nav.dx, options);
            if (nav.accept && row.kind == Setting::Kind::Toggle) value = value ? 0 : 1;
            if (value != row.value) {
                result.changed = result.selected;
                result.value = value;
            }
        }
        result.back = nav.cancel;

        const int visible = s.visible;
        result.scroll = layout::ScrollFor(result.selected, page.scroll, count, visible);
        const ImU32 ink = theme::Hud(theme::Ink);
        if (result.scroll > 0) ShadowIcon("ui.chevron.up", s.x0 + 29.0F, s.top - 16.0F, 22.0F, ink);
        if (result.scroll + visible < count)
            ShadowIcon("ui.chevron.down", s.x0 + 29.0F, s.top + s.rowHeight * static_cast<float>(visible) + 16.0F, 22.0F, ink);

        const float valueSize = 33.0F, labelSize = 33.0F;
        for (int i = result.scroll; i < std::min(count, result.scroll + visible); ++i) {
            const auto& row = page.rows[static_cast<std::size_t>(i)];
            const float y0 = s.top + s.rowHeight * static_cast<float>(i - result.scroll), y1 = y0 + s.rowHeight;
            const bool selected = i == result.selected;
            if (ImGui::IsMouseHoveringRect(Stage(s.x0, y0), Stage(s.x1, y1)) && !row.disabled) {
                const auto delta = ImGui::GetIO().MouseDelta;
                if (delta.x != 0.0F || delta.y != 0.0F) result.selected = i;
                if (row.kind == Setting::Kind::Info && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) result.pressed = i;
            }
            if (selected) {
                Canvas()->AddRectFilledMultiColor(Stage(s.x0, y0), Stage(s.x1, y1), theme::Hud(theme::GoldLight),
                                                  theme::Hud(theme::Gold), theme::Hud(theme::Gold), theme::Hud(theme::GoldLight));
            } else {
                Canvas()->AddRectFilled(Stage(s.x0, y0), Stage(s.x1, y1), IM_COL32(0x18, 0x1C, 0x1D, 166));
                Canvas()->AddRectFilled(Stage(s.x0, y1 - 1.0F), Stage(s.x1, y1), IM_COL32(0x31, 0x39, 0x3B, 166));
            }
            const ImU32 color = row.disabled ? theme::Unavailable : selected ? theme::DarkOnGold : ink;
            const float ty = TextTop(y0, s.rowHeight, labelSize);
            if (selected) fo4::Text(s.labelX, ty, labelSize, row.label, color);
            else ShadowText(s.labelX, ty, labelSize, row.label, color, false, 0.0F, fo4::Align::Left, kGlow);

            const float cy = (y0 + y1) / 2;
            if (row.kind == Setting::Kind::Slider) {
                const float x0 = s.valueX - 90.0F, x1 = s.valueX + 90.0F;
                Canvas()->AddRectFilled(Stage(x0, cy - 2.0F), Stage(x1, cy + 2.0F), color);
                const float knob = x0 + (x1 - x0) * std::clamp(row.fraction, 0.0F, 1.0F);
                Canvas()->AddRectFilled(Stage(knob - 3.0F, cy - 10.0F), Stage(knob + 3.0F, cy + 10.0F), color);
                continue;
            }
            const bool toggle = row.kind == Setting::Kind::Toggle;
            const std::string value = toggle ? (row.value ? page.onLabel : page.offLabel)
                                     : row.value >= 0 && row.value < static_cast<int>(row.options.size())
                                         ? row.options[static_cast<std::size_t>(row.value)] : std::string{};
            const float width = fo4::TextWidth(valueSize, value);
            fo4::Text(s.valueX, TextTop(y0, s.rowHeight, valueSize), valueSize, value, color, false, 0.0F, fo4::Align::Center);
            if (row.kind == Setting::Kind::Stepper) {
                const float lx = s.valueX - width / 2 - 14.0F, rx = s.valueX + width / 2 + 14.0F;
                const ImVec2 left[]{Stage(lx + 6.0F, cy - 9.0F), Stage(lx + 6.0F, cy + 9.0F), Stage(lx - 6.0F, cy)};
                const ImVec2 right[]{Stage(rx - 6.0F, cy - 9.0F), Stage(rx + 6.0F, cy), Stage(rx - 6.0F, cy + 9.0F)};
                Canvas()->AddConvexPolyFilled(left, 3, color);
                Canvas()->AddConvexPolyFilled(right, 3, color);
                if (selected && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !row.disabled) {
                    const int options = static_cast<int>(row.options.size());
                    int stepped = row.value;
                    if (ImGui::IsMouseHoveringRect(Stage(lx - 20.0F, y0), Stage(lx + 12.0F, y1))) stepped = layout::StepValue(row.value, -1, options);
                    if (ImGui::IsMouseHoveringRect(Stage(rx - 12.0F, y0), Stage(rx + 20.0F, y1))) stepped = layout::StepValue(row.value, 1, options);
                    if (stepped != row.value) { result.changed = i; result.value = stepped; }
                }
            } else if (toggle && selected && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !row.disabled &&
                       ImGui::IsMouseHoveringRect(Stage(s.x0, y0), Stage(s.x1, y1))) {
                result.changed = i;
                result.value = row.value ? 0 : 1;
            }
        }
        if (!page.prompt.empty()) {
            // The question sits on a plate across the middle of the list; the game's hint bar answers it.
            const float py0 = s.top + s.rowHeight * 5.0F, py1 = py0 + s.rowHeight * 2.0F;
            Canvas()->AddRectFilled(Stage(s.x0, py0), Stage(s.x1, py1), IM_COL32(0, 0, 0, 225));
            Canvas()->AddRect(Stage(s.x0, py0), Stage(s.x1, py1), ink, 0.0F, 0, Px(1.5F));
            ShadowText((s.x0 + s.x1) / 2, TextTop(py0, py1 - py0, 36.0F), 36.0F, page.prompt, ink, true, 0.0F, fo4::Align::Center, kHalo);
        }
        if (!page.hints.empty()) result.hint = HintBar(page.hints, 992.0F, 960.0F);
        return result;
    }

    void DrawLoading(const Loading& l) {
        const auto display = ImGui::GetIO().DisplaySize;
        auto* list = Canvas();
        if (l.minimal) list->AddRectFilled({0, 0}, display, IM_COL32(0, 0, 0, 255));
        else if (l.image && !l.spinnerOnly) {
            if (const auto cover = layout::CoverRect(display.x, display.y, l.imageWidth, l.imageHeight))
                list->AddImage(l.image, {cover->x, cover->y},
                               {cover->x + l.imageWidth * cover->scale, cover->y + l.imageHeight * cover->scale});
        }
        fo4::hud::Anchor(fo4::hud::Edge::Left);
        const float right = display.x / Px(1.0F);  // the screen's right edge in stage units
        const ImU32 ink = theme::Hud(theme::Ink), gold = theme::Hud(theme::Gold);
        const float logoX = right - 92.0F, logoY = 1003.0F;
        if (!l.minimal && !l.spinnerOnly) {
            const float x0 = 48.0F, x1 = right - 48.0F, y0 = 960.0F, y1 = 1047.0F;
            list->AddRectFilled(Stage(x0, y0), Stage(x1, y1), IM_COL32(0, 0, 0, 191));
            list->AddRect(Stage(x0, y0), Stage(x1, y1), Alpha(gold, 0.7F), 0.0F, 0, Px(1.5F));
            list->AddLine(Stage(115.0F, y1), Stage(210.0F, y0), Alpha(gold, 0.7F), Px(1.5F));
            const float split = right - 605.0F;
            list->AddLine(Stage(split, y1), Stage(split + 95.0F, y0), Alpha(gold, 0.7F), Px(1.5F));
            if (icons::HasArt("loading.survivalguide")) ShadowIcon("loading.survivalguide", 108.0F, 1004.0F, 92.0F, gold);
            // The tip wraps between the guide and the LOADING divider: two lines at 24, else three at 20, centred in
            // the box; what still does not fit ends in "...".
            const float tipWidth = split - 214.0F - 24.0F;
            const auto wrap = [&](float size) {
                std::vector<std::string> lines;
                std::string line;
                for (std::size_t at = 0; at < l.tip.size();) {
                    const auto end = std::min(l.tip.find(' ', at), l.tip.size());
                    const auto word = std::string_view(l.tip).substr(at, end - at);
                    auto joined = line.empty() ? std::string(word) : line + " " + std::string(word);
                    if (!line.empty() && fo4::TextWidth(size, joined) > tipWidth) lines.push_back(std::exchange(line, std::string(word)));
                    else line = std::move(joined);
                    at = end + 1;
                }
                if (!line.empty()) lines.push_back(std::move(line));
                return lines;
            };
            float tipSize = 24.0F;
            auto lines = wrap(tipSize);
            if (lines.size() > 2) lines = wrap(tipSize = 20.0F);
            if (lines.size() > 3) {
                lines.resize(3);
                while (!lines[2].empty() && fo4::TextWidth(tipSize, lines[2] + "...") > tipWidth) lines[2].pop_back();
                lines[2] += "...";
            }
            const float pitch = tipSize * 1.15F, tipTop = (y0 + y1) / 2 - pitch * static_cast<float>(lines.size()) / 2;
            for (std::size_t i = 0; i < lines.size(); ++i)
                ShadowText(214.0F, TextTop(tipTop + pitch * static_cast<float>(i), pitch, tipSize), tipSize, lines[i], ink, false, 0.0F,
                           fo4::Align::Left, kHalo);
            ShadowText(split + 103.0F, TextTop(979.0F, 26.0F, 21.0F), 21.0F, l.loadingLabel, ink, true, 0.0F, fo4::Align::Left, kHalo);
            if (!l.level.empty())
                ShadowText(split + 103.0F, TextTop(1006.0F, 26.0F, 21.0F), 21.0F, l.level, ink, true, 0.0F, fo4::Align::Left, kHalo);
        }
        if (!l.minimal) {
            // Only the cog turns; FO76 keeps its "4" upright and dark on top.
            ShadowIcon("loading.vault", logoX, logoY, 94.0F, gold, kIconShadow, layout::VaultCogAngle(l.seconds));
            icons::Draw(Canvas(), "loading.vault.number", Stage(logoX, logoY), Px(94.0F), IM_COL32(0, 0, 0, 255));
        }
    }
}
