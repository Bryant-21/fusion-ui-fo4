#pragma once
// Flash filters as FO76's movies set them, for every fo76 screen (HUD, menus, C.A.M.P.). Values are the
// SWF's own: angle in degrees (45 = down-right), distance and blur in stage units, strength as Flash's
// multiplier. Draw the shadow layers first, then the element itself.
#include "fusionui/fo4/Style.h"

#include <algorithm>
#include <cmath>

namespace fusionui::fo76 {
    struct DropShadow {
        ImU32 color = IM_COL32(0, 0, 0, 255);
        float angle = 45.0F;
        float distance = 2.0F;
        float blur = 2.0F;  // 0 = hard copy
        float strength = 1.0F;
    };

    // The common FO76 text/icon shadow (black, 45 degrees, distance 2, blur 2, strength 1).
    inline constexpr DropShadow kShadow{};
    // Text-field halo: #181c1d up-left, faint.
    inline constexpr DropShadow kHalo{IM_COL32(24, 28, 29, 255), 225.0F, 1.0F, 4.0F, 0.5F};
    // Hard copies (stealth brackets, revive timer, floating labels).
    inline constexpr DropShadow kHard{IM_COL32(0, 0, 0, 255), 45.0F, 2.0F, 0.0F, 1.0F};
    // The shadow the game adds behind HUD text and icons beyond the SWF filters. Measured on 4K in-game captures
    // (quest tracker over sky): a near-black copy ~3.5 px down-right with a 1 px soft edge, no rim on the top-left
    // edges. Strength 2.5 brings ShadowLayers' 0.4 core to full black.
    inline constexpr DropShadow kGlow{IM_COL32(0, 0, 0, 255), 45.0F, 2.5F, 1.0F, 2.5F};

    // XP meter texts (black, distance 3, blur 4) and the LEVEL UP banner (#181c1d, distance 3, blur 8, strength 2).
    inline constexpr DropShadow kXp{IM_COL32(0, 0, 0, 255), 45.0F, 3.0F, 4.0F, 1.0F};
    inline constexpr DropShadow kLevelUp{IM_COL32(24, 28, 29, 255), 45.0F, 3.0F, 8.0F, 2.0F};

    // Calls draw(offset in screen px, tint) once per shadow tap. A blurred shadow is approximated by a core
    // at the offset plus one 8-tap ring per stage unit out to blur/2 (Flash's 3-pass box blur spreads
    // ~blur/2 each side), outer rings fainter. Tuned on FO76's blur-2 text: ~0.55 dark 1 px under the
    // glyph edge, background again 3 px out.
    template <class Draw>
    void ShadowLayers(const DropShadow& shadow, float alpha, Draw&& draw) {
        constexpr float kPi = 3.14159265F;
        const float rad = shadow.angle * kPi / 180.0F;
        const ImVec2 base{fo4::Px(shadow.distance * std::cos(rad)), fo4::Px(shadow.distance * std::sin(rad))};
        const auto tint = [&](float weight) {
            const float a = static_cast<float>((shadow.color >> IM_COL32_A_SHIFT) & 0xFF) * alpha * weight * shadow.strength;
            return (shadow.color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(std::fmin(a, 255.0F) + 0.5F) << IM_COL32_A_SHIFT);
        };
        if (shadow.blur <= 0.0F) {
            draw(base, tint(1.0F));
            return;
        }
        draw(base, tint(0.4F));
        const int rings = std::max(1, static_cast<int>(shadow.blur / 2.0F + 0.5F));
        for (int ring = 1; ring <= rings; ++ring) {
            const float r = fo4::Px(static_cast<float>(ring));
            const float weight = 0.05F * static_cast<float>(rings - ring + 1) / static_cast<float>(rings);
            for (int t = 0; t < 8; ++t) {
                const float a = kPi * (static_cast<float>(t) + (ring % 2 == 0 ? 0.5F : 0.0F)) / 4.0F;  // stagger rings
                draw({base.x + r * std::cos(a), base.y + r * std::sin(a)}, tint(weight));
            }
        }
    }
}
