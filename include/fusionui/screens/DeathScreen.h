#pragma once
#include <imgui.h>
#include "core/DeathScreen.h"

#include <string>

namespace fusionui::screens::death {
    struct Model {
        int seconds = 0;  // respawn countdown; 0 = the RESPAWN hint
        std::string title = "RESPAWN", hint = "Respawn";
        ImU32 titleColor = IM_COL32(0xF5, 0xCB, 0x5B, 0xFF);  // RESPAWN title (FO76 default; drawn under the HUD colour)
        ImU32 ink = IM_COL32(0xFF, 0xFF, 0xCB, 0xFF);         // seconds, hint bar
    };

    void Draw(const Model& model, float alpha = 1.0F);
}
