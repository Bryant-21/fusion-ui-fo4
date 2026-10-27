#pragma once
#include "styles/HudDraw.h"

namespace fusionui::styles::frames {
    using namespace hudDraw;
    using modern::theme::Family;
    inline bool ClassicFrame() {
        const auto family = modern::theme::CurrentAppearance().family;
        return family == Family::Fallout3 || family == Family::NewVegas;
    }

    inline void Diamond(float x, float y, float radius, ImU32 ink) {
        kit::Canvas()->AddQuad(At(x, y - radius), At(x + radius, y), At(x, y + radius), At(x - radius, y), ink, Px(1));
    }

    inline void Panel(float x, float y, float width, float height, ImU32 ink) {
        const bool skyrim = modern::theme::CurrentAppearance().family == Family::Skyrim;
        const auto glass = modern::theme::CurrentAppearance().family == Family::Starfield ? IM_COL32(16, 27, 39, 225) : IM_COL32(0, 0, 0, skyrim ? 190 : 115);
        Box(x, y, width, height, Fade(glass, static_cast<float>(ink >> IM_COL32_A_SHIFT) / 255));
        if (modern::theme::CurrentAppearance().family == Family::Starfield) {
            kit::Canvas()->AddRect(At(x, y), At(x + width, y + height), Fade(ink, 0.65F), 0, 0, Px(1));
            Box(x, y, 3, height, ink);
        } else if (skyrim) {
            for (const float inset : {0.0F, 3.0F}) {
                Line(x + 8, y + inset, x + width - 8, y + inset, Fade(ink, 0.6F));
                Line(x + 8, y + height - inset, x + width - 8, y + height - inset, Fade(ink, 0.6F));
            }
            Diamond(x, y + height / 2, 4, ink);
            Diamond(x + width, y + height / 2, 4, ink);
        } else {
            const float cap = ClassicFrame() ? 8.0F : 5.0F;
            for (const float side : {x, x + width}) {
                const float inner = side == x ? side + cap : side - cap;
                Line(side, y, side, y + height, ink, ClassicFrame() ? 2.0F : 1.0F);
                Line(side, y, inner, y, ink);
                Line(side, y + height, inner, y + height, ink);
            }
        }
    }

    inline void Bar(float x, float y, float width, float height, float fill, ImU32 ink) {
        fill = std::clamp(fill, 0.0F, 1.0F);
        if (ClassicFrame()) {
            for (float offset = 0; offset < width; offset += 6.4F)
                Box(x + offset, y, std::min(4.8F, width - offset), height, Fade(ink, offset < width * fill ? 1.0F : 0.18F));
        } else if (modern::theme::CurrentAppearance().family == Family::Skyrim) {
            Line(x, y - 2, x + width, y - 2, ink);
            Line(x, y + height + 2, x + width, y + height + 2, ink);
            Diamond(x - 4, y + height / 2, 4, ink);
            Diamond(x + width + 4, y + height / 2, 4, ink);
            Meter(x, y, width, height, fill, ink);
        } else {
            Meter(x, y, width, height, fill, ink);
        }
    }

}
