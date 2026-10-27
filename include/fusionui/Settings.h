#pragma once
#include "fusionui/Abi.h"

#include <string_view>
#include <vector>

namespace fusionui {
    std::vector<FUSIONUI_SettingsPanel> SettingsPanels();
    // Zero reopens the last selected panel, or the first registered panel.
    bool OpenSettings(FUSIONUI_ClientId id = 0);
    const char* SettingsCategory(FUSIONUI_ClientId id);
    // The MOD SETTINGS search box, shared by every panel: a panel draws only the rows whose label or help
    // matches (case-insensitive), and a section heading only when one of its rows does.
    std::string_view SettingsSearch();
    bool SettingsMatches(std::string_view label, std::string_view help = {});
}
