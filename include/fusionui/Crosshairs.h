#pragma once

#include <imgui.h>

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

struct ID3D11Device;

namespace fusionui::crosshairs {
    struct Entry {
        std::string key, label, game;
        bool isDefault = false;
        bool custom = false;
    };

    // Render thread only. Load is cached; Reload requests a rescan on the next Load.
    void Load(ID3D11Device* device);
    void Reload();
    void ReleaseDeviceObjects();
    // Views remain valid until the next reload/device loss.
    std::span<const Entry> Entries();
    std::span<const std::string> Diagnostics();
    // The real (not MO2-virtual) crosshairs folder, so files added in game show on Refresh.
    std::filesystem::path Directory();
    // Creates the per-game folders and opens Directory() in Explorer; false when it could not.
    bool OpenDirectory();
    // Empty choice uses the current Appearance selection; "theme" follows the game and "none" hides it.
    // Size bounds the transparent source frame, preserving its aim center and aspect ratio.
    // Tint 0 follows HUD colour. Unavailable selections fall back to the theme's default.
    // None or no available default draws nothing and returns false, never a placeholder glyph.
    bool Draw(ImDrawList* list, ImVec2 center, float size = 40.0F, ImU32 tint = 0, std::string_view choice = {});
}
