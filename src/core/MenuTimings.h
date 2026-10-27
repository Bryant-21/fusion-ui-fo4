#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace fusionui::core::menus {
inline constexpr std::array<int, 7> SkyrimFavoritesOpen{0, 41, 84, 128, 169, 212, 256};
inline constexpr std::array<float, 7> SkyrimFavoritesSlide{-30.0F, -25.0F, -20.0F, -15.0F, -10.0F, -5.05F, 0.0F};
inline constexpr std::array<std::array<int, 13>, 12> StarfieldFavoritesOpen{{
    {0, 0, 0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256},
    {0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256},
    {0, 0, 0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256},
    {0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 0, 0, 256, 0, 256, 0, 256, 256, 256, 256},
}};
template <class T, std::size_t N>
inline T MenuFrame(const std::array<T, N>& track, float age) {
    const auto frame = std::clamp(static_cast<int>(std::floor(age * 30 + 0.0001F)), 0, static_cast<int>(N) - 1);
    return track[static_cast<std::size_t>(frame)];
}
}
