#pragma once
#include "fusionui/modern/Theme.h"
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace fusionui::core::inventory::styles {
    struct Metrics { float pitch, selected, font; };
    inline Metrics Rows(modern::theme::Family family) {
        using modern::theme::Family;
        switch (family) {
        case Family::Skyrim: return {35, 65, 20};
        case Family::Starfield: return {83.0F / 1.5F, 83.0F / 1.5F, 20.0F / 1.5F};
        case Family::Fallout3:
        case Family::NewVegas: return {26, 26, 19};
        default: return {26.2F, 26.2F, 18};
        }
    }

    inline int VisibleRows(Metrics metrics, float height) {
        return std::max(1, static_cast<int>(std::floor((height - metrics.selected + metrics.pitch) / metrics.pitch)));
    }

    struct Row { int index; float top, height; };
    inline std::vector<Row> RowBoxes(Metrics metrics, int first, int count, int selected, float top, float height) {
        std::vector<Row> result;
        const float bottom = top + height;
        for (int i = first; i < count; ++i) {
            const float size = i == selected ? metrics.selected : metrics.pitch;
            if (top + size > bottom + 0.001F) break;
            result.push_back({i, top, size});
            top += size;
        }
        return result;
    }

    inline int NextTab(int selected, int direction, int count, std::span<const bool> enabled) {
        if (count <= 0 || direction == 0) return -1;
        int index = std::clamp(selected, 0, count - 1);
        for (int step = 0; step < count; ++step) {
            index = (index + (direction > 0 ? 1 : count - 1)) % count;
            if ((enabled.empty() || (index < static_cast<int>(enabled.size()) && enabled[static_cast<std::size_t>(index)])) && index != selected) return index;
        }
        return -1;
    }
}
