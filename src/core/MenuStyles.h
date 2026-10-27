#pragma once
#include "fusionui/modern/AppearanceCodec.h"
#include "core/MainMenuLayout.h"
#include <array>

namespace fusionui::core::menus {
    using modern::theme::Family;
    // Row boxes start 12 left of their text (the common renderer's padding), so the text sits on mainmenu::kListLeft
    // (a 1920-wide stage) in this 1280-wide one.
    inline constexpr float kListX = mainmenu::kListLeft / 1.5F - 12;
    struct ListMetrics { float x, top, width, pitch, font; };
    inline ListMetrics MainMetrics(Family family, bool title, int count, float) {
        switch (family) {
        case Family::Fallout4: return {kListX, 302, 250, 29.2F, 19};
        case Family::Skyrim: return title ? ListMetrics{kListX, 330, 200, 40, 24} : ListMetrics{kListX, 159.3F, 220, 40, 27};
        case Family::Starfield: return {kListX, 380.9F / 1.5F, 243.306F / 1.5F, 35.397F / 1.5F, 22.0F / 1.5F};
        default: {
            const float pitch = title ? 48.0F : 60.0F;
            return {kListX, (720 - std::min(count, title ? 12 : 10) * pitch) / 2, 280, pitch, 19};
        }
        }
    }

    inline std::vector<mainmenu::Box> MainBoxes(Family family, bool title, int count, int selected, float width) {
        const auto metrics = MainMetrics(family, title, count, width);
        const int visible = std::max(1, static_cast<int>((670 - metrics.top) / metrics.pitch));
        const int first = mainmenu::ScrollFor(std::max(0, selected), 0, count, visible);
        std::vector<mainmenu::Box> boxes;
        for (int i = 0; i < count; ++i) {
            const float y = metrics.top + (i - first) * metrics.pitch;
            boxes.push_back({metrics.x, y, metrics.x + metrics.width, y + metrics.pitch});
        }
        return boxes;
    }

    inline constexpr std::array<std::pair<float, float>, 12> kStarfieldCrossCenters{{
        {-235.375F, 0.0F}, {-160.15F, 0.0F}, {-83.775F, 0.0F}, {83.75F, 0.0F}, {160.1F, 0.0F}, {235.4F, 0.0F},
        {0.0F, -235.325F}, {0.0F, -160.075F}, {0.0F, -83.725F}, {0.0F, 83.775F}, {0.0F, 160.1F}, {0.0F, 235.375F}}};

    inline int StarfieldCrossSlot(float x, float y) {
        if (!std::isfinite(x) || !std::isfinite(y) || std::max(std::abs(x), std::abs(y)) < 40.0F / 276) return -1;
        int selected = -1;
        float distance = INFINITY;
        for (std::size_t i = 0; i < kStarfieldCrossCenters.size(); ++i) {
            const auto [cx, cy] = kStarfieldCrossCenters[i];
            const float next = (x * 276 - cx) * (x * 276 - cx) + (y * 276 - cy) * (y * 276 - cy);
            if (next < distance) { distance = next; selected = static_cast<int>(i); }
        }
        return selected;
    }

    inline int StarfieldCrossStep(int selected, int dx, int dy) {
        constexpr std::array up{8, 8, 8, 8, 8, 8, 6, 6, 7, 8, 9, 10, 8};
        constexpr std::array down{9, 9, 9, 9, 9, 9, 7, 8, 9, 10, 11, 11, 9};
        constexpr std::array left{0, 0, 1, 2, 3, 4, 2, 2, 2, 2, 2, 2, 2};
        constexpr std::array right{1, 2, 3, 4, 5, 5, 3, 3, 3, 3, 3, 3, 3};
        const auto at = static_cast<std::size_t>(selected >= 0 && selected < 12 ? selected : 12);
        return dy < 0 ? up[at] : dy > 0 ? down[at] : dx < 0 ? left[at] : dx > 0 ? right[at] : selected;
    }

    // Fallout 4's FavoritesCross (FavoritesMenu.swf): slots 0-2 left (outermost first), 3-5 right, 6-8 up, 9-11
    // down. kFo4CrossCells are the 57-unit entries' top-left corners; the empty centre cell's middle is kFo4CrossMid.
    inline constexpr std::array<std::pair<float, float>, 12> kFo4CrossCells{{
        {0.0F, 180.0F}, {60.0F, 180.0F}, {120.0F, 180.0F}, {240.0F, 180.0F}, {300.0F, 180.0F}, {360.0F, 180.0F},
        {179.75F, 0.0F}, {179.75F, 60.0F}, {179.75F, 120.0F}, {179.75F, 240.0F}, {179.75F, 300.0F}, {179.75F, 360.0F}}};
    inline constexpr float kFo4CrossEntry = 57, kFo4CrossMid = 179.75F + kFo4CrossEntry / 2;
    // _OutDirectory / _InDirectory: the next slot further out / toward the centre on the same arm.
    inline constexpr std::array<int, 12> kFo4CrossOut{0, 0, 1, 4, 5, 5, 6, 6, 7, 10, 11, 11};
    inline constexpr std::array<int, 12> kFo4CrossIn{1, 2, 2, 3, 3, 4, 7, 8, 8, 9, 9, 10};

    // hideEmptySlots: the four inner slots always show; an outer one shows while it or a slot beyond it is filled.
    inline bool Fo4CrossHidden(const std::array<bool, 12>& occupied, int slot) {
        if (slot < 0 || slot >= 12) return true;
        const auto at = [](int i) { return static_cast<std::size_t>(i); };
        const int out = kFo4CrossOut[at(slot)];
        return !occupied[at(slot)] && !occupied[at(out)] && !occupied[at(kFo4CrossOut[at(out)])] && slot != 2 && slot != 3 && slot != 8 &&
               slot != 9;
    }

    // ClampSelection: a hidden slot steps inward (twice at most: inner slots never hide); -1 stays -1.
    inline int Fo4CrossClamp(const std::array<bool, 12>& occupied, int slot) {
        if (slot < 0 || slot >= 12) return -1;
        for (int step = 0; step < 2 && Fo4CrossHidden(occupied, slot); ++step) slot = kFo4CrossIn[static_cast<std::size_t>(slot)];
        return slot;
    }

    // onKeyUp: FO4's direction tables (index 12 = nothing selected), then ClampSelection.
    inline int Fo4CrossStep(const std::array<bool, 12>& occupied, int selected, int dx, int dy) {
        constexpr std::array up{8, 8, 8, 8, 8, 8, 6, 6, 7, 8, 9, 10, 8};
        constexpr std::array down{9, 9, 9, 9, 9, 9, 7, 8, 9, 10, 11, 11, 9};
        constexpr std::array left{0, 0, 1, 2, 3, 4, 2, 2, 2, 2, 2, 2, 2};
        constexpr std::array right{1, 2, 3, 4, 5, 5, 3, 3, 3, 3, 3, 3, 3};
        const auto at = static_cast<std::size_t>(selected >= 0 && selected < 12 ? selected : 12);
        const int next = dy < 0 ? up[at] : dy > 0 ? down[at] : dx < 0 ? left[at] : dx > 0 ? right[at] : selected;
        return Fo4CrossClamp(occupied, next);
    }

    // The shown cell nearest a pointer direction (x, y in -1..1, y down; 1 reaches the outermost cell), -1 in
    // the centre cell. FO4 selects by hovering an entry; hidden entries take no hover.
    inline int Fo4CrossSlot(const std::array<bool, 12>& occupied, float x, float y) {
        constexpr float kReach = 180;  // centre to the outermost cell's middle
        if (!std::isfinite(x) || !std::isfinite(y) || std::max(std::abs(x), std::abs(y)) * kReach < kFo4CrossEntry / 2) return -1;
        int selected = -1;
        float distance = INFINITY;
        for (int i = 0; i < 12; ++i) {
            if (Fo4CrossHidden(occupied, i)) continue;
            const auto [cx, cy] = kFo4CrossCells[static_cast<std::size_t>(i)];
            const float dx = x * kReach - (cx + kFo4CrossEntry / 2 - kFo4CrossMid), dy = y * kReach - (cy + kFo4CrossEntry / 2 - kFo4CrossMid);
            if (const float next = dx * dx + dy * dy; next < distance) { distance = next; selected = i; }
        }
        return selected;
    }

    inline std::optional<mainmenu::Cover> PhotoFit(float frameWidth, float frameHeight, float width, float height) {
        if (!(frameWidth > 0 && frameHeight > 0 && width > 0 && height > 0) || !std::isfinite(frameWidth) || !std::isfinite(frameHeight) || !std::isfinite(width) || !std::isfinite(height)) return std::nullopt;
        const float scale = std::min({frameWidth / width, frameHeight / height, 1.0F});
        return mainmenu::Cover{(frameWidth - width * scale) / 2, (frameHeight - height * scale) / 2, scale};
    }
}
