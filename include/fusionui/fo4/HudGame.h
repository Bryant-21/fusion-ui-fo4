#pragma once
// Engine side of fusionui/fo4/Hud.h; defined in plugin builds only (src/game).
#include <imgui.h>

#include <optional>
#include <span>
#include <string_view>

namespace fusionui::fo4::hud {
    // The game's HUD is up: HUDMenu open, a game loaded, no loading screen or main menu, and none of the
    // menus that hide the HUD (Pip-Boy, container, barter, examine, workshop, pause, dialogue, VATS...)
    // nor any of `alsoHiddenBy`. Overlays draw above every vanilla menu, so check this each Draw.
    bool Visible(std::span<const std::string_view> alsoHiddenBy = {});

    // World point -> display pixels through the world camera, or nullopt when it is behind the camera
    // or more than `margin` (fraction of the screen) off screen. Call from Draw so the point matches the
    // frame being drawn.
    std::optional<ImVec2> Project(float x, float y, float z, float margin = 0.0F);
    // The same into an explicit `viewport` (pixels, or {1, 1} for normalized coordinates with y down). It
    // does not touch ImGui, so game-thread code can call it.
    std::optional<ImVec2> Project(float x, float y, float z, ImVec2 viewport, float margin = 0.0F);
}
