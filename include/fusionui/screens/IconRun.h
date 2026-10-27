#pragma once

namespace fusionui::screens::hud {
    // One row of a rendered item silhouette on a square grid (alpha 0..255), as Tales' weapon icons are cut.
    struct IconRun {
        int x = 0, y = 0, length = 0, alpha = 255;
    };
}
