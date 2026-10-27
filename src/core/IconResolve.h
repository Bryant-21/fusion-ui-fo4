#pragma once
#include "fusionui/IconKeys.h"
#include "fusionui/GameIcons.h"

#include <cstddef>
#include <string_view>

namespace fusionui::core::icons {
    struct Resolved {
        int pack = -1;                // index of the first pack with art for the key, or -1
        const char* glyph = nullptr;  // Font Awesome fallback (or the placeholder) when pack < 0
        std::string_view art;
    };

    // Normal packs in registration order, then fallback packs in registration order, then the key's Font
    // Awesome glyph, then the placeholder glyph.
    template <class HasArt, class IsFallback>
    Resolved Resolve(std::string_view key, std::size_t packCount, HasArt hasArt, IsFallback isFallback) {
        for (const bool fallbackPass : {false, true})
            for (std::size_t i = 0; i < packCount; ++i)
                if (isFallback(i) == fallbackPass && hasArt(i, key)) return {static_cast<int>(i), nullptr, key};
        if (const auto* glyph = fusionui::icons::FallbackGlyph(key)) return {-1, glyph, {}};
        return {-1, fusionui::icons::kPlaceholderGlyph, {}};
    }

    template <class HasArt, class IsFallback, class IsOriginal>
    Resolved ResolveGame(std::string_view key, std::string_view game, std::size_t packCount,
                         HasArt hasArt, IsFallback isFallback, IsOriginal isOriginal) {
        if (!game.empty()) {
            const auto mapped = fusionui::icons::GameKey(game, key);
            if (!mapped.empty()) {
                const auto result = Resolve(mapped, packCount, hasArt, isFallback);
                if (result.pack >= 0) return {result.pack, nullptr, mapped};
            }
            for (const bool fallback : {false, true})
                for (std::size_t i = 0; i < packCount; ++i)
                    if (isOriginal(i) && isFallback(i) == fallback && hasArt(i, key))
                        return {static_cast<int>(i), nullptr, key};
        }
        const auto* glyph = fusionui::icons::FallbackGlyph(key);
        return {-1, glyph ? glyph : fusionui::icons::kPlaceholderGlyph, {}};
    }
}
