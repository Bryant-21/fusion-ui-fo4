#pragma once

#include "fusionui/Crosshairs.h"

#include <nlohmann/json.hpp>

#include <array>
#include <vector>

namespace fusionui::core::crosshairs {
    inline constexpr std::array<std::string_view, 6> Games{"fo4", "fo76", "skyrim", "starfield", "fo3", "fnv"};
    inline constexpr std::size_t MaxKeyLength = 96;

    bool ValidGame(std::string_view game);
    bool ValidKey(std::string_view key);
    std::string CustomKey(std::string_view game, std::string_view filename);
    struct Catalog {
        std::vector<fusionui::crosshairs::Entry> entries;
        std::vector<std::string> warnings;
    };
    Catalog Parse(const nlohmann::json& document);
    const fusionui::crosshairs::Entry* Resolve(std::span<const fusionui::crosshairs::Entry> entries,
                                          std::string_view choice, std::string_view game);
}
