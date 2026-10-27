#pragma once
// How a modern window's appearance (background opacity, text sizes) is stored: one section per
// client in the shared FusionUI.toml, falling back to the shared [Modern] section older builds wrote,
// then to the client's own defaults. Pure: the TOML read/write lives in Appearance.cpp.

#include "fusionui/modern/Theme.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fusionui::modern::appearance {
    using Lookup = std::function<std::optional<std::string>(std::string_view key)>;

    // "Modern" for the shared section, "Modern.<client>" for a client's own.
    std::string Section(std::string_view client);

    // Stable names from theme::Families; anything else reads as Modern.
    std::string_view FamilyName(theme::Family family);
    theme::Family ParseFamily(std::string_view text);
    std::string_view PackChoice(std::string_view text, bool cursor);
    std::string_view CrosshairChoice(std::string_view text);

    // Theme comes from the client's section, then the shared one; Format leaves it out (SetFamily writes it).
    theme::Appearance Resolve(const Lookup& client, const Lookup& shared, const theme::Appearance& defaults);

    std::vector<std::pair<std::string, std::string>> Format(const theme::Appearance& appearance);
}
