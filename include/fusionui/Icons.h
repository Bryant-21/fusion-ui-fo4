#pragma once
// Icons by stable key (fusionui/IconKeys.h). Art comes from icon packs registered at runtime, e.g. one a
// converter generated on the player's machine; keys no pack covers draw their Font Awesome glyph.
#include "fusionui/IconAtlas.h"
#include "fusionui/IconKeys.h"

#include <imgui.h>

#include <filesystem>
#include <string_view>

struct ID3D11Device;

namespace fusionui::icons {
    void DrawGame(ImDrawList* list, std::string_view key, ImVec2 center, float size, ImU32 tint, float alpha = 1.0F,
                  bool shadow = true);
    // Fallback packs are consulted only after every Normal pack, so a converted pack always beats them.
    enum class Priority { Normal, Fallback };
    // An IconAtlas JSON (fusionui/IconAtlas.h). Within a priority, earlier registrations win. A missing or
    // unreadable pack is skipped at Load and every key keeps its fallback. Any thread.
    void RegisterPack(const std::filesystem::path& atlasJson, Priority priority = Priority::Normal);
    // Loads packs registered since the last call into `device`. Cheap when nothing is pending; call it
    // from OnContextCreated or at the top of Draw. The first call also registers fusionui's own packs as Fallback
    // packs: AssetDir()/icons/fusion.json, then every other pack FusionUI installs there (fo76, map, fo4, ...).
    void Load(ID3D11Device* device);
    // A pack supplies art for `key`.
    bool HasArt(std::string_view key);
    // Draws `key` centred at `center`, `size` pixels tall, in `tint` (pack art is tinted through its
    // four bands; glyphs are drawn in the tint). `angle` turns pack art clockwise (radians) about its centre
    // and `mirror` flips it left to right; fallback glyphs ignore both. Render thread.
    void Draw(ImDrawList* list, std::string_view key, ImVec2 center, float size, ImU32 tint, float alpha = 1.0F,
              bool shadow = false, float angle = 0.0F, bool mirror = false);
    // Pack art only, each band in its own `palette` colour (map markers). Returns false, drawing nothing, when
    // no pack has `key`. Render thread.
    bool Draw(ImDrawList* list, std::string_view key, ImVec2 center, float size, const Palette4& palette, float alpha = 1.0F,
              bool shadow = true, float angle = 0.0F, bool mirror = false);
    bool DrawRect(ImDrawList* list, std::string_view key, ImVec2 min, ImVec2 max, ImU32 tint, float alpha = 1.0F,
                  bool shadow = false);
    // Drops pack textures after a device loss; the next Load reloads them.
    void ReleaseDeviceObjects();
}
