#pragma once
// Run-cut item silhouettes (hud::IconRun, as Tales' rendered item icons are cut) fitted into an icon box.
#include "fusionui/screens/IconRun.h"

#include <imgui.h>

#include <algorithm>
#include <limits>
#include <optional>
#include <span>

namespace fusionui::core::iconruns {
    struct Fit {
        int x0 = 0, y0 = 0;          // the drawn pixels' top-left on the run grid
        float unit = 0, left = 0, top = 0;
    };

    // Fits the drawn pixels (not the whole grid) into a `box` square centred on (cx, cy), keeping their aspect.
    inline std::optional<Fit> FitRuns(std::span<const screens::hud::IconRun> runs, float cx, float cy, float box) {
        int x0 = std::numeric_limits<int>::max(), y0 = x0, x1 = std::numeric_limits<int>::min(), y1 = x1;
        for (const auto& r : runs) {
            x0 = std::min(x0, r.x);
            x1 = std::max(x1, r.x + r.length);
            y0 = std::min(y0, r.y);
            y1 = std::max(y1, r.y + 1);
        }
        if (x1 <= x0 || y1 <= y0) return std::nullopt;
        const float unit = box / static_cast<float>(std::max(x1 - x0, y1 - y0));
        return Fit{x0, y0, unit, cx - unit * static_cast<float>(x1 - x0) / 2, cy - unit * static_cast<float>(y1 - y0) / 2};
    }

    // `to` maps the style's own coordinates to screen points (fo4::Stage, hudDraw::At); each run's alpha scales tint's.
    template <class ToScreen>
    void DrawRuns(ImDrawList* list, std::span<const screens::hud::IconRun> runs, float cx, float cy, float box, ImU32 tint, ToScreen to) {
        const auto fit = FitRuns(runs, cx, cy, box);
        if (!fit) return;
        const float alpha = static_cast<float>((tint >> IM_COL32_A_SHIFT) & 0xFF);
        for (const auto& r : runs) {
            const auto a = static_cast<ImU32>(alpha * static_cast<float>(std::clamp(r.alpha, 0, 255)) / 255.0F + 0.5F);
            const float x = fit->left + fit->unit * static_cast<float>(r.x - fit->x0), y = fit->top + fit->unit * static_cast<float>(r.y - fit->y0);
            list->AddRectFilled(to(x, y), to(x + fit->unit * static_cast<float>(r.length), y + fit->unit),
                                (tint & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT));
        }
    }
}
