#pragma once
// Drawing helpers shared by the fo76 HUD sources (stage units, FO76 1920 x 1080).
#include "fusionui/Common.h"
#include "fusionui/Icons.h"
#include "fusionui/fo4/Hud.h"
#include "fusionui/fo4/Style.h"
#include "fusionui/fo76/Filters.h"
#include "fusionui/fo76/Stage.h"
#include "fusionui/screens/HudPalette.h"
#include "fusionui/screens/IconRun.h"
#include "fusionui/fo76/Theme.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace fusionui::fo76::hud::draw {
    using fusionui::screens::hud::Palette;
    using fo4::Align;

    // Roboto Condensed: cap height 1456 and ascender 1900 of 2048 units.
    inline constexpr float kCapEm = 1456.0F / 2048.0F, kCapTopEm = (1900.0F - 1456.0F) / 2048.0F;
    inline constexpr ImU32 kTrack = IM_COL32(24, 28, 29, 115);  // #181c1d at 45%
    inline constexpr ImU32 kInk = IM_COL32(24, 22, 16, 255);

    // The user rule: HUD-family inks draw in the player's HUD colour (theme::HudTint); other colours pass through.
    inline ImU32 Tint(ImU32 color) { return theme::Hud(color); }

    inline ImU32 Fade(ImU32 color, float alpha) {
        const auto a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(alpha, 0.0F, 1.0F);
        return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT);
    }

    inline std::string Upper(std::string_view text) {
        std::string out(text);
        for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return out;
    }

    // Groups FO76 moves as a whole (the power-armor layout) draw their usual numbers under a shift.
    // Thread-local, kept in FusionUI.dll so every module drawing a group sees the same shift.
    ImVec2& GroupShift();
    struct Shifted {
        Shifted(float dx, float dy) { GroupShift() = {dx, dy}; }
        ~Shifted() { GroupShift() = {}; }
    };

    // The Flash filters an element carries: `outer` on the clip, `inner` on its text field (FO76 nests a
    // black kShadow clip round a kHalo text field). Meters, cards, the compass band and dividers carry none.
    struct Fx {
        const DropShadow* outer = nullptr;
        const DropShadow* inner = nullptr;
    };
    inline constexpr Fx kText{&kShadow, &kHalo};
    inline constexpr Fx kDrop{&kShadow};
    inline constexpr Fx kHaloOnly{&kHalo};
    inline constexpr Fx kGlowOnly{&kGlow};
    inline constexpr Fx kGlowHalo{&kGlow, &kHalo};
    // Nameplate / target card detail row: 1 unit at 45 degrees, blur 2.
    inline constexpr DropShadow kDetailShadow{IM_COL32(0, 0, 0, 255), 45.0F, 1.0F, 2.0F, 1.0F};

    // Calls draw(dx, dy, color) in stage units under each of fx's shadows, then once with `color` on top.
    template <class F>
    void Filtered(Fx fx, ImU32 color, F&& draw) {
        const float a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F;
        if (a > 0)
            for (const auto* shadow : {fx.outer, fx.inner})
                if (shadow)
                    ShadowLayers(*shadow, a, [&](ImVec2 offset, ImU32 tint) {
                        draw(offset.x / fo4::Px(1.0F), offset.y / fo4::Px(1.0F), tint);
                    });
        draw(0.0F, 0.0F, color);
    }

    // Text placed by its cap top and cap height, as FO76's layout is measured.
    inline void Caps(float x, float capTop, float capHeight, std::string_view text, ImU32 color, bool bold = false,
                     Align align = Align::Left, float spacing = 0.0F, Fx fx = {}) {
        const float size = capHeight / kCapEm;
        Filtered(fx, color, [&](float dx, float dy, ImU32 c) {
            fo4::Text(x + dx + GroupShift().x, capTop + dy + GroupShift().y - kCapTopEm * size, size, text, c, bold, spacing, align);
        });
    }
    inline float CapsWidth(float capHeight, std::string_view text, bool bold = false, float spacing = 0.0F) {
        return fo4::TextWidth(capHeight / kCapEm, text, bold, spacing);
    }
    // Caps under one of the SWF's own DropShadow filters (Filters.h).
    inline void FilteredCaps(const DropShadow& shadow, float x, float capTop, float capHeight, std::string_view text, ImU32 color,
                             float alpha, bool bold = false, Align align = Align::Left) {
        const float px = fo4::Px(1.0F);
        ShadowLayers(shadow, alpha, [&](ImVec2 offset, ImU32 tint) {
            Caps(x + offset.x / px, capTop + offset.y / px, capHeight, text, tint, bold, align);
        });
        Caps(x, capTop, capHeight, text, Fade(color, alpha), bold, align);
    }

    // Inclusive pixel boxes, as measured.
    inline void Box(float x0, float y0, float x1, float y1, ImU32 color) {
        fo4::Canvas()->AddRectFilled(fo4::Stage(x0 + GroupShift().x, y0 + GroupShift().y),
                                     fo4::Stage(x1 + 1 + GroupShift().x, y1 + 1 + GroupShift().y), color);
    }
    inline void Panel(float x0, float y0, float x1, float y1, ImU32 fill, ImU32 border) {
        Box(x0, y0, x1, y0, border);
        Box(x0, y1, x1, y1, border);
        Box(x0, y0 + 1, x0, y1 - 1, border);
        Box(x1, y0 + 1, x1, y1 - 1, border);
        Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, fill);
    }

    // Icons take the HUD tint; full-colour pack art (currencies) ignores it. Shadow taps tint the icon's
    // shape, so a full-colour icon wants icons::Draw's own hard shadow instead.
    // `angle` turns the art clockwise (radians), `mirror` flips it left to right.
    inline void Icon(std::string_view key, float cx, float cy, float size, ImU32 tint, float alpha, Fx fx = {}, float angle = 0.0F,
                     bool mirror = false) {
        Filtered(fx, Fade(tint, alpha), [&](float dx, float dy, ImU32 c) {
            const float a = static_cast<float>((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F;
            icons::Draw(fo4::Canvas(), key, fo4::Stage(cx + dx + GroupShift().x, cy + dy + GroupShift().y), fo4::Px(size), c | IM_COL32_A_MASK, a,
                        false, angle, mirror);
        });
    }

    inline ImU32 Mix(ImU32 a, ImU32 b, float t) {
        const auto ch = [&](int shift) {
            const float x = static_cast<float>((a >> shift) & 0xFF), y = static_cast<float>((b >> shift) & 0xFF);
            return static_cast<ImU32>(x + (y - x) * t + 0.5F) << shift;
        };
        return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | ch(IM_COL32_A_SHIFT);
    }

    // A meter fill with FO76's 1-unit darker rim (top (212,158,60) on the cream fill, (206,66,52) on red).
    inline void Fill(float x0, float y0, float x1, float y1, ImU32 color, float alpha, bool rim = true) {
        if (x1 < x0) return;
        Box(x0, y0, x1, y1, Fade(color, alpha));
        if (!rim) return;
        const auto edge = Fade(Mix(color, IM_COL32(150, 40, 0, 255), 0.41F), alpha);
        Box(x0, y0, x1, y0, edge);
        Box(x0, y1, x1, y1, edge);
        Box(x0, y0 + 1, x0, y1 - 1, edge);
        Box(x1, y0 + 1, x1, y1 - 1, edge);
    }

    // Fill inset from the frame per meter (FO76 varies it), the rim, and the frame's own opacity.
    struct Inset {
        float side = 3, top = 3, bottom = 3;
        bool rim = true;
        float frame = 1.0F;    // track and outline opacity
        float outline = 1.0F;  // outline opacity on top of `frame`
    };
    // FO76 meter: 1-unit black outline round a #181c1d 45% track, then the fill. `fromRight` empties from the left.
    // `trail` (> fill) is recent damage: EnemyHealthOptionalBar (Optional_mc) behind the fill, 78.7% of its height
    // and at half brightness (ColorTransform 0.5, 0.5, 0.5).
    inline void Meter(float x0, float y0, float x1, float y1, float fill, ImU32 color, float alpha, bool fromRight = false,
                      Inset inset = {}, float trail = -1.0F) {
        Panel(x0, y0, x1, y1, Fade(kTrack, alpha * inset.frame), Fade(IM_COL32(0, 0, 0, 255), alpha * inset.frame * inset.outline));
        fill = std::clamp(fill, 0.0F, 1.0F);
        const float inner0 = x0 + inset.side, inner1 = x1 - inset.side, width = (inner1 - inner0) * fill;
        const float top = y0 + inset.top, bottom = y1 - inset.bottom;
        if (trail = std::min(trail, 1.0F); trail > fill) {
            const float trailWidth = (inner1 - inner0) * trail, pad = (bottom - top) * (1.0F - 0.787F) / 2;
            const ImU32 dim = IM_COL32(((color >> IM_COL32_R_SHIFT) & 0xFF) / 2, ((color >> IM_COL32_G_SHIFT) & 0xFF) / 2,
                                       ((color >> IM_COL32_B_SHIFT) & 0xFF) / 2, (color >> IM_COL32_A_SHIFT) & 0xFF);
            if (fromRight) Fill(inner1 - trailWidth, top + pad, inner1 - width, bottom - pad, dim, alpha, false);
            else Fill(inner0 + width, top + pad, inner0 + trailWidth, bottom - pad, dim, alpha, false);
        }
        if (fill <= 0.0F) return;
        if (fromRight) Fill(inner1 - width, top, inner1, bottom, color, alpha, inset.rim);
        else Fill(inner0, top, inner0 + width, bottom, color, alpha, inset.rim);
    }

    // Longest prefix of `text` (plus "...") that fits `width` at `capHeight`.
    inline std::string Ellipsize(std::string_view text, float capHeight, float width, bool bold = false) {
        if (CapsWidth(capHeight, text, bold) <= width) return std::string(text);
        std::string out(text);
        while (!out.empty() && CapsWidth(capHeight, out + "...", bold) > width)
            while (!out.empty()) {  // drop one UTF-8 code point: continuation bytes, then the lead byte
                const auto c = static_cast<unsigned char>(out.back());
                out.pop_back();
                if ((c & 0xC0) != 0x80) break;
            }
        return out + "...";
    }

    // Greedy word wrap at `width`. With `maxLines`, the last line takes the rest and ends in "..." when the
    // text does not fit.
    inline std::vector<std::string> Wrap(std::string_view text, float capHeight, float width, bool bold = false,
                                         std::size_t maxLines = SIZE_MAX) {
        std::vector<std::string> lines;
        std::string line;
        for (std::size_t i = 0; i < text.size();) {
            const auto end = std::min(text.find(' ', i), text.size());
            const auto word = std::string(text.substr(i, end - i));
            const auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && CapsWidth(capHeight, candidate, bold) > width) {
                lines.push_back(line);
                line = word;
                if (lines.size() == maxLines) break;
            } else {
                line = candidate;
            }
            i = end + 1;
        }
        if (lines.size() < maxLines) {
            if (!line.empty()) lines.push_back(line);
        } else {
            lines.back() = Ellipsize(lines.back() + " " + line + "...", capHeight, width, bold);
        }
        return lines;
    }
}
