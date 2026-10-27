#pragma once
// FO76's menu look, shared by the trade, container, inspect and Pip-Boy screens: the palette
// (GlobalFunc.as and the securetrade / examinemenu art, measured at FO76's default HUD colour), the gold
// selection plate, the button-hint bar (BSButtonHintBar) and the quantity and confirm modals
// (securetrade.swf ModalSetQuantity_mc / ModalConfirmPurchase_mc). Coordinates are FO76's 1920 x 1080
// stage; draw inside fo4::BeginMenu(id, dim, kFo76Stage).
#include "fusionui/fo76/Filters.h"
#include "fusionui/fo76/Stage.h"
#include "core/InventoryRows.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fusionui::fo76 {
    namespace theme {
        // The coloured entries below (Ink, Gold, GoldLight, PlateLight) are FO76's values at its default HUD
        // colour; menus draw them through Hud(), which follows the player's HUD colour. Dark ink, greys, fills
        // and the rarity / difference colours stay fixed.
        inline constexpr ImU32 kFo76DefaultHud = IM_COL32(255, 219, 119, 255);
        inline constexpr ImU32 Ink = IM_COL32(0xFF, 0xFF, 0xCB, 0xFF);         // body text
        inline constexpr ImU32 Gold = IM_COL32(0xF5, 0xCB, 0x5B, 0xFF);        // headers; selection edge
        inline constexpr ImU32 DarkOnGold = IM_COL32(0x18, 0x1C, 0x1D, 0xFF);  // text on a gold plate
        inline constexpr ImU32 Unavailable = IM_COL32(0x56, 0x61, 0x67, 0xFF);
        inline constexpr ImU32 GoldLight = IM_COL32(0xF9, 0xE1, 0x8C, 0xFF);   // selection centre
        inline constexpr ImU32 PlateLight = IM_COL32(0xFF, 0xFE, 0xC9, 0xFF);  // corner plate gradient start
        inline constexpr ImU32 MenuPanel = IM_COL32(0x11, 0x14, 0x14, 216);
        inline constexpr ImU32 TabBack = IM_COL32(0x11, 0x11, 0x14, 191);
        inline constexpr ImU32 CardBox = IM_COL32(0x33, 0x33, 0x33, 191);
        inline constexpr ImU32 CardHeader = IM_COL32(0x31, 0x39, 0x3B, 216);
        inline constexpr ImU32 CardRow = IM_COL32(0x07, 0x07, 0x07, 51);
        inline constexpr ImU32 InspectBox = IM_COL32(0x18, 0x1C, 0x1D, 191);
        inline constexpr ImU32 MeterBack = IM_COL32(0x18, 0x1C, 0x1D, 89);
        inline constexpr ImU32 Difference = IM_COL32(0x27, 0xC3, 0xB7, 0xFF);  // inspect difference numbers
        inline constexpr ImU32 Common = IM_COL32(0x89, 0xFF, 0x5B, 0xFF);
        inline constexpr ImU32 Rare = IM_COL32(0x4B, 0xAF, 0xD6, 0xFF);
        inline constexpr ImU32 Epic = IM_COL32(0xA4, 0x3D, 0xFA, 0xFF);
        inline constexpr ImU32 Legendary = IM_COL32(0xE5, 0x97, 0x51, 0xFF);

        // FO76's HUD-colour family: its cream/gold/amber inks (hue 30-70 degrees, not grey). Meaning colours
        // (hostile red, blueprint blue, rarity) and neutral greys are outside it.
        constexpr bool HudFamily(ImU32 c) {
            const int r = (c >> IM_COL32_R_SHIFT) & 0xFF, g = (c >> IM_COL32_G_SHIFT) & 0xFF, b = (c >> IM_COL32_B_SHIFT) & 0xFF;
            const int hi = r > g ? (r > b ? r : b) : (g > b ? g : b), lo = r < g ? (r < b ? r : b) : (g < b ? g : b);
            if (hi == 0 || (hi - lo) * 100 < hi * 15 || r != hi || b != lo) return false;  // grey, or hue outside red..yellow
            return (g - b) * 100 >= (hi - lo) * 50;  // hue >= 30 degrees (60 * (g - b) / (hi - lo))
        }
        // User rule: everything is the player's HUD colour. A HUD-family ink becomes `hud` at the ink's own
        // brightness (cream and FO76's default draw as exactly `hud`); any other colour is returned unchanged.
        constexpr ImU32 HudTint(ImU32 fo76, ImU32 hud) {
            if (!HudFamily(fo76)) return fo76;
            const int r = (fo76 >> IM_COL32_R_SHIFT) & 0xFF, g = (fo76 >> IM_COL32_G_SHIFT) & 0xFF;
            const ImU32 value = static_cast<ImU32>(r > g ? r : g);  // HudFamily guarantees blue is the lowest
            const auto ch = [&](int shift) { return ((((hud >> shift) & 0xFF) * value + 127) / 255) << shift; };
            return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | (fo76 & IM_COL32_A_MASK);
        }
        // HudTint with the player's HUD colour (common::HudColor()).
        ImU32 Hud(ImU32 fo76);
    }

    // The text-field shadow securetrade sets on rows, card text, headers and the footer (black, 45 degrees,
    // distance 2, no blur) and the icon shadow (35 degrees, blur 1).
    inline constexpr DropShadow kTextShadow{IM_COL32(0, 0, 0, 255), 45.0F, 2.0F, 0.0F, 1.0F};
    inline constexpr DropShadow kIconShadow{IM_COL32(0, 0, 0, 255), 35.0F, 2.0F, 1.0F, 1.0F};

    // fo4::Text / icons::Draw with a Flash drop shadow under them.
    void ShadowText(float x, float y, float size, std::string_view text, ImU32 color, bool bold = false, float spacing = 0.0F,
                    fo4::Align align = fo4::Align::Left, const DropShadow& shadow = kTextShadow);
    void ShadowIcon(std::string_view key, float cx, float cy, float size, ImU32 tint, const DropShadow& shadow = kIconShadow,
                    float angle = 0.0F);

    // FO76's gold selection: radial #F9E18C -> #F5CB5B with a gold glow (blur 16) of `glow` strength.
    void SelectionPlate(float x0, float y0, float x1, float y1, float glow = 1.25F);
    // Flash-style radial gradient filling a rectangle: `inner` at the centre, `outer` on the inscribed ellipse.
    void RadialRect(float x0, float y0, float x1, float y1, ImU32 inner, ImU32 outer);

    // A page's Header_mc corner plate (securetrade shape 362 / examinemenu shape 436): a 40.55 tall plate on the
    // left edge whose right side slants 54.8 inward toward the bottom, filled with a horizontal #FFFEC9 -> #F5CB5B
    // gradient (HUD-tinted), and its 26 bold #181C1D label, shrunk to fit its text field as FO76's
    // TEXTAUTOSZ_SHRINK does. FO76 gives each movie a fixed plate; securetrade adds a dark corner wedge.
    struct CornerPlateLook {
        float left, top, width;  // the plate's top edge
        float gradientX0;        // where the gradient starts (#FFFEC9)
        float textX, textY, textWidth;  // HeaderText_tf
        bool wedge;              // securetrade's black wedge (shape 360): the corner fades to 90 % black
    };
    inline constexpr CornerPlateLook kTradeCorner{-0.25F, 1015.7F, 240.45F, 25.0F, 14.0F, 1018.3F, 182.0F, true};
    inline constexpr CornerPlateLook kInspectCorner{0.0F, 967.2F, 215.2F, 0.0F, 20.2F, 969.6F, 182.0F, false};
    void CornerPlate(std::string_view label, const CornerPlateLook& look = kTradeCorner);

    // BSButtonHintBar: "KEY) LABEL" groups 20 apart on a black 40 % strip (padding 8), no brackets, text
    // #FFFFCB, disabled hints at 50 %. `y` is the bar's registration point (trade 1011.5, inspect 994.4).
    // Returns the clicked hint or -1.
    // `ink` and `scale` let the Pip-Boy page draw it in its own colour and units. `boldLabels` is the
    // stand-alone bsbuttonhintbar.swf look (labels in its $MAIN_Font_Bold field), which photo mode loads.
    int HintBar(std::span<const fo4::Hint> hints, float y, float centerX = 960.0F, ImU32 ink = theme::Hud(theme::Ink), float scale = 1.0F,
                bool boldLabels = false);

    // colorConditionMeter / MeterClip at (x0, y0): 160 x 24 backing at 35 %, a gold bar inset 7 shrinking toward
    // the right end, a cream overcharge line over it above 100 %, a full cream block when broken. `condition` 0..2.
    void ConditionMeter(float x0, float y0, float condition);

}
