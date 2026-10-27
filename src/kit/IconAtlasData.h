#pragma once
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fusionui::kit {
    struct IconRect {
        float u0{}, v0{}, u1{}, v1{};
    };

    struct IconEntry {
        bool tintable{};
        bool mask{};                        // white single-colour shape, drawn in the bright tint
        bool monochrome{};
        bool alphaMask{};
        bool shadow = true;
        float aspect = 1.0F;
        float aperture{};
        bool luminanceMask{};
        IconRect clip{0, 0, 1, 1};
        struct Layer { IconRect shape, bounds; };
        std::vector<Layer> layers;
        std::array<IconRect, 4> bands{};   // shadow, dim, mid, bright
        IconRect shape{};
    };

    struct IconAtlasData {
        std::string texture;
        int width{};
        int height{};
        std::unordered_map<std::string, IconEntry> icons;
    };

    std::optional<IconAtlasData> ParseIconAtlas(std::string_view json);
}
