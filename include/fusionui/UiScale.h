#pragma once
#include <algorithm>
#include <cstdint>

// How big FusionUI draws: Auto follows the Windows display scale of the game's monitor (dpi / 96, at
// least 1; 4K at 150% = 1.5) and falls back to display height / 1080 while the DPI is unknown. A
// client may pin a manual scale instead.
namespace fusionui::uiscale {
    inline constexpr float kAuto = 0.0F;
    inline constexpr float kMin = 0.5F;
    inline constexpr float kMax = 3.0F;
    inline constexpr std::uint32_t kBaseDpi = 96;

    // Anything at or below 0, or NaN, is Auto; a manual scale is clamped to [kMin, kMax].
    [[nodiscard]] inline float Sanitize(float raw) noexcept { return raw > 0.0F ? std::clamp(raw, kMin, kMax) : kAuto; }

    // A DPI of 0 means unknown.
    [[nodiscard]] inline float AutoScale(float displayHeight, std::uint32_t dpi) noexcept {
        const float scale = dpi > 0 ? static_cast<float>(dpi) / static_cast<float>(kBaseDpi) : displayHeight / 1080.0F;
        return std::clamp(scale, 1.0F, kMax);
    }

    [[nodiscard]] inline float Resolve(float manual, float displayHeight, std::uint32_t dpi) noexcept {
        const float setting = Sanitize(manual);
        return setting == kAuto ? AutoScale(displayHeight, dpi) : setting;
    }
}
