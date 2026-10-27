#pragma once
#include "fusionui/screens/Keypad.h"
#include "fusionui/screens/PhotoMode.h"
#include "fusionui/screens/MainMenu.h"
#include "fusionui/screens/DeathScreen.h"
#include "fusionui/screens/Wheel.h"
#include "fusionui/screens/DailyOps.h"

namespace fusionui::styles::menus {
    screens::KeypadOutcome Keypad(screens::KeypadModel& model);
    void PhotoMode(const screens::PhotoModeView& view);
    std::vector<core::mainmenu::Box> RowBoxes(const screens::mainmenu::List& list);
    screens::mainmenu::ListResult DrawList(const screens::mainmenu::List& list);
    screens::mainmenu::SettingsResult DrawSettings(const screens::mainmenu::SettingsPage& page, const fo4::Nav& nav);
    void DrawLoading(const screens::mainmenu::Loading& loading);
    void DrawDeath(const screens::death::Model& model, float alpha);
    void DrawWheel(const screens::wheel::Model& model, float alpha);
    int DrawDailyOps(const screens::hud::DailyOpsReport& report, const screens::hud::Palette& palette, float alpha);
}
