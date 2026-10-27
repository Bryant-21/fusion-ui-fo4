#include "styles/fo76/PhotoMode.h"
#include "styles/PhotoOverlays.h"
#include "fusionui/Common.h"
#include "fusionui/Icons.h"
#include "fusionui/fo76/Theme.h"

#include <algorithm>
#include <array>

namespace fusionui::fo76 {
    namespace {
        using fo4::Align;
        using fo4::Canvas;
        using fo4::Px;
        using fo4::Stage;
        using theme::Hud;

        // selfiemenu.swf, FO76 stage units. PanelBackground_mc (1166.3, 601.3) holds MessageBackground_mc
        // (shape 71 at +454.15, +157.5, 90 % alpha): a #31393B tab strip over a #181C1D body, 1-unit #181C1D
        // outlines. Panels sit at PanelBackground + TabSelector - 23 / + 20 with rows 45 apart from +5,
        // each control 75 in (Panel.CONTROL_OFFSET_X / CONTROL_SPACING / CONTROL_SPACING_OFFSET).
        constexpr float kLeft = 1358.55F, kRight = 1885.55F, kTop = 613.1F, kStrip = 672.1F, kBottom = 995.65F;
        constexpr ImU32 kBody = IM_COL32(0x18, 0x1C, 0x1D, 229), kStripFill = IM_COL32(0x31, 0x39, 0x3B, 229);
        constexpr float kRowX = 1418.3F, kRowY = 678.2F, kRowStep = 45.0F;
        // Within a control: Selector_mc (shape 18, 519.4 x 32.25) at +203, +13.9; Slider_mc / Stepper_mc at
        // +329.4, +15.45; label text from -328 (the text field's -330 box plus its 2-unit gutter; sub rows set x 41
        // further in), value box 190.8 wide centred.
        constexpr float kSelectorDx = 203.0F, kSelectorDy = 13.9F, kSelectorHalfW = 259.7F, kSelectorHalfH = 16.1F;
        constexpr float kWidgetDx = 329.4F, kWidgetDy = 15.45F, kLabelDx = -328.0F, kSubIndent = 41.0F;
        constexpr float kArrowDx = 103.5F, kValueWidth = 186.8F;
        // TabSelector_mc at PanelBackground + (200, 51.9): Icon0..4_mc and CategorySelector_mc (57 x 57),
        // which SetPanel moves by Icon1 - Icon0 per tab.
        constexpr float kTabX = 1366.3F, kTabY = 653.2F, kTabSelectorX = 124.45F, kTabSelectorY = -9.9F, kTabStep = 65.5F,
                        kTabHalf = 28.5F, kTabIcon = 44.0F;
        constexpr std::array<ImVec2, core::photo::kPanelCount> kTabIcons{
            {{123.6F, -10.35F}, {189.1F, -10.1F}, {255.7F, -10.15F}, {321.7F, -10.05F}, {386.95F, -9.95F}}};
        // The bumper arrows (shape 21 in sprites 101 / 102) and ButtonHintPrev / Next_mc: one-hint
        // BSButtonHintBars without a background at PanelBackground + (274.85 / 649.4, 18.15), the key centred
        // 6 left of the origin (its empty label's gap), text 5 + 4 below it. ButtonBackground_mc (the main hint bar) is at (960, 1011.5).
        constexpr ImVec2 kPrevArrow{1407.45F, 642.5F}, kNextArrow{1834.95F, 642.7F};
        constexpr float kPrevKeyX = 1435.0F, kNextKeyX = 1810.0F, kBumperBarY = 619.45F, kHintBarY = 1011.5F;
        // The stand-alone hint bar draws 42 tall where securetrade's is 38.
        constexpr float kHintScale = 1.1F;

        // FO76's art at its default HUD colour; drawn through Hud().
        constexpr ImU32 kLabel = IM_COL32(0xFF, 0xFF, 0xCA, 0xFF);   // SliderLabel_tf / StepperLabel_tf
        constexpr ImU32 kCream = IM_COL32(0xFE, 0xFF, 0xC9, 0xFF);   // track, bar, value, arrows
        constexpr ImU32 kPlate = IM_COL32(0xF5, 0xCB, 0x5B, 0xFF);   // row selector, tab selector
        constexpr ImU32 kTabArt = IM_COL32(0xF4, 0xCA, 0x5D, 0xFF);  // unselected tab icons
        constexpr ImU32 kTabEdge = IM_COL32(0xF5, 0xE4, 0x68, 0xFF), kTabGlow = IM_COL32(0xF5, 0xF1, 0x62, 0xFF);
        constexpr ImU32 kBlack = IM_COL32(0, 0, 0, 255);

        ImU32 Faded(ImU32 color, float alpha) {
            const float a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * alpha;
            return (color & ~IM_COL32_A_MASK) | static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT;
        }

        void Rect(float x0, float y0, float x1, float y1, ImU32 color) { Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), color); }

        // Shape 21: the stepper / bumper chevron, pointing left; `flip` is the -1, -1 copy.
        void Chevron(ImVec2 c, bool flip, ImU32 color) {
            constexpr std::array<ImVec2, 6> kShape{{{3.05F, -7.8F}, {4.75F, -6.1F}, {-1.35F, 0.0F}, {4.75F, 6.1F}, {3.05F, 7.8F}, {-4.75F, 0.0F}}};
            std::array<ImVec2, 6> points{};
            const float s = flip ? -1.0F : 1.0F;
            for (std::size_t i = 0; i < kShape.size(); ++i) points[i] = Stage(c.x + kShape[i].x * s, c.y + kShape[i].y * s);
            Canvas()->AddConcavePolyFilled(points.data(), static_cast<int>(points.size()), color);
        }

        // Label text fields: a 2.5 blur DropShadow at distance 0 (a halo), black on cream text, cream on black.
        DropShadow Halo(ImU32 color) { return {color, 45.0F, 0.0F, 2.5F, 1.0F}; }

        void Background() {
            Rect(kLeft, kStrip, kRight, kBottom, kBody);
            Rect(kLeft, kTop, kRight, kStrip, kStripFill);
            const ImU32 edge = IM_COL32(0x18, 0x1C, 0x1D, 229);
            Canvas()->AddRect(Stage(kLeft - 0.5F, kTop - 0.5F), Stage(kRight + 0.5F, kBottom + 0.5F), edge, 0.0F, 0, Px(1.0F));
            Canvas()->AddLine(Stage(kLeft, kStrip), Stage(kRight, kStrip), edge, Px(1.0F));
        }

        void Tabs(int selected) {
            const float sx = kTabX + kTabSelectorX + kTabStep * static_cast<float>(selected), sy = kTabY + kTabSelectorY;
            const float unit = Px(1.0F);
            ShadowLayers({Hud(kTabGlow), 0.0F, 0.0F, 4.0F, 1.0F}, 1.0F, [&](ImVec2 o, ImU32 tint) {
                Rect(sx - kTabHalf + o.x / unit, sy - kTabHalf + o.y / unit, sx + kTabHalf + o.x / unit, sy + kTabHalf + o.y / unit, tint);
            });
            Rect(sx - kTabHalf - 0.5F, sy - kTabHalf - 0.5F, sx + kTabHalf + 0.5F, sy + kTabHalf + 0.5F, Hud(kTabEdge));
            Rect(sx - kTabHalf, sy - kTabHalf, sx + kTabHalf, sy + kTabHalf, Hud(kPlate));
            for (int i = 0; i < core::photo::kPanelCount; ++i) {
                const auto at = kTabIcons[static_cast<std::size_t>(i)];
                icons::Draw(Canvas(), kPhotoTabIcons[static_cast<std::size_t>(i)], Stage(kTabX + at.x, kTabY + at.y), Px(kTabIcon),
                            i == selected ? kBlack : Hud(kTabArt));
            }

            // Bumper hints: chevrons on the strip's ends with the Z / C keys (LB / RB glyphs on a pad).
            const ImU32 key = Hud(theme::Ink);
            Chevron(kPrevArrow, false, Hud(kCream));
            Chevron(kNextArrow, true, Hud(kCream));
            if (common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD) {
                common::PadGlyph(Canvas(), Stage(kPrevKeyX, kBumperBarY + 24.0F), Px(13.5F), FUSIONUI_PAD_LB, key);
                common::PadGlyph(Canvas(), Stage(kNextKeyX, kBumperBarY + 24.0F), Px(13.5F), FUSIONUI_PAD_RB, key);
            } else {
                ShadowText(kPrevKeyX, kBumperBarY + 9.0F, 26.0F, "Z)", key, false, 0.0F, Align::Center);
                ShadowText(kNextKeyX, kBumperBarY + 9.0F, 26.0F, "C)", key, false, 0.0F, Align::Center);
            }
        }

        void Row(const core::photo::Control& c, int index, bool selected) {
            const float x = kRowX, y = kRowY + kRowStep * static_cast<float>(index);
            const float alpha = c.active ? 1.0F : 0.25F;
            const float cx = x + kSelectorDx, cy = y + kSelectorDy;
            if (selected) Rect(cx - kSelectorHalfW, cy - kSelectorHalfH, cx + kSelectorHalfW, cy + kSelectorHalfH, Faded(Hud(kPlate), alpha));

            const ImU32 label = Faded(selected ? kBlack : Hud(kLabel), alpha);
            const ImU32 ink = Faded(selected ? kBlack : Hud(kCream), alpha);
            const ImU32 halo = Faded(selected ? Hud(kLabel) : kBlack, alpha);
            const float wx = x + kWidgetDx, wy = y + kWidgetDy;
            ShadowText(wx + kLabelDx + (c.sub ? kSubIndent : 0.0F), y, 22.0F, c.label, label, true, 0.0F, Align::Left, Halo(halo));

            if (c.slider) {
                constexpr float kHalfTrack = core::photo::kSliderTrack / 2, kTrackHalfH = 2.25F;
                Rect(wx - kHalfTrack, wy - kTrackHalfH, wx + kHalfTrack, wy + kTrackHalfH, ink);
                // Slider_Bar_*_mc: 6 x 14 with a blur-2 glow in the row's background colour, which only shows
                // as a faint edge where the bar crosses the track.
                const float bx = wx - kHalfTrack + std::clamp(c.Fraction(), 0.0F, 1.0F) * core::photo::kSliderTrack;
                const ImU32 edge = Faded(selected ? Hud(kPlate) : kBlack, alpha * 0.6F);
                Rect(bx - 3.5F, wy - 7.5F, bx + 3.5F, wy + 7.5F, edge);
                Rect(bx - 3.0F, wy - 7.0F, bx + 3.0F, wy + 7.0F, ink);
                return;
            }
            // StepperValue_tf: 20 bold centred, shrunk to fit (TEXTAUTOSZ_SHRINK).
            const auto shown = c.Shown();
            const float width = fo4::TextWidth(20.0F, shown, true);
            const float size = width > kValueWidth ? 20.0F * kValueWidth / width : 20.0F;
            fo4::Text(wx, wy - 13.0F + (20.0F - size) / 2, size, shown, ink, true, 0.0F, Align::Center);
            if (c.Arrows()) {
                Chevron({wx - kArrowDx, wy}, false, ink);
                Chevron({wx + kArrowDx, wy}, true, ink);
            }
        }
    }

    void PhotoMode(const PhotoModeView& view) {
        fusionui::styles::PhotoOverlays(view);
        const auto& m = view.model;
        if (!m.visible) return;

        Background();
        Tabs(m.panel);
        const auto& panel = m.Current();
        for (int i = 0; i < static_cast<int>(panel.controls.size()); ++i)
            Row(panel.controls[static_cast<std::size_t>(i)], i, i == panel.selected);

        const std::array<fo4::Hint, 4> hints{{{FUSIONUI_PAD_A, "Space", view.hints[0].c_str()},
                                              {FUSIONUI_PAD_X, "T", view.hints[1].c_str()},
                                              {FUSIONUI_PAD_Y, "R", view.hints[2].c_str()},
                                              {FUSIONUI_PAD_B, "Tab", view.hints[3].c_str()}}};
        HintBar(hints, kHintBarY, 960.0F, Hud(theme::Ink), kHintScale, true);
    }
}
