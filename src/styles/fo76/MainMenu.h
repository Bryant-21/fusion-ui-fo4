#pragma once
// Fallout 76's title menu, pause menu, settings page and loading screen, drawn natively. Each screen is a
// plain view model in, a result out: the owner (Tales' MainMenuUi) fills the rows from the live vanilla
// list each frame and acts on what the result reports. Positions are FO76's 1920 x 1080 stage
// (core/MainMenuLayout.h); the lists hug the left screen edge. Draw inside fo4::BeginMenu(id, 0, kFo76Stage).
#include "fusionui/fo76/Theme.h"
#include "core/MainMenuLayout.h"

#include <string>
#include <vector>

#include "fusionui/screens/MainMenu.h"

namespace fusionui::fo76::mainmenu {
    using fusionui::screens::mainmenu::Row;
    using fusionui::screens::mainmenu::Kind;
    using fusionui::screens::mainmenu::List;
    using fusionui::screens::mainmenu::ListResult;
    using fusionui::screens::mainmenu::Setting;
    using fusionui::screens::mainmenu::SettingsPage;
    using fusionui::screens::mainmenu::SettingsResult;
    using fusionui::screens::mainmenu::Loading;
    std::vector<core::mainmenu::Box> RowBoxes(const List& list);
    ListResult DrawList(const List& list);
    SettingsResult DrawSettings(const SettingsPage& page, const fo4::Nav& nav);
    void DrawLoading(const Loading& loading);
}
