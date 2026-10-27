#pragma once
#include <imgui.h>

namespace fusionui::screens::hud {
    // HUD inks; menus that sit on the HUD layer (wheel, death screen) take them too.
    struct Palette {
        ImU32 hud = IM_COL32(255, 219, 119, 255);     // player HUD colour (FO76 default cream)
        ImU32 header = IM_COL32(244, 174, 54, 255);   // quest titles, tracker icons
        ImU32 hostile = IM_COL32(245, 113, 89, 255);  // rads, enemies, negative effects
        ImU32 friendly = IM_COL32(247, 204, 93, 255);
    };
}
