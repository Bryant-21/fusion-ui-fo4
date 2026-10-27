#pragma once
// Fallout 76's C.A.M.P. build menu (workshop build menu), drawn natively. The menu is a plain view model
// in, a result out: the owner (Tales' CampBuildMenu) fills the model from its catalog each frame, keeps
// the returned navigation state, and acts on activate / back / mode / category changes. Positions are
// FO76's (1920 x 1080 stage, from workshopmenunew.swf and its library), including the movie's per-aspect
// frames (16:9 / 16:10 / 21:9 / 32:9). Draw inside fo4::BeginMenu(id, 0, kFo76Stage) / EndMenu.
#include "fusionui/Abi.h"
#include "styles/fo76/ButtonHints.h"
#include "fusionui/fo76/Stage.h"
#include "core/CampBuildNav.h"

#include <cstdint>
#include <string>
#include <vector>

#include "fusionui/screens/CampBuild.h"

namespace fusionui::fo76::camp {
    using fusionui::screens::camp::Focus;
    using fusionui::screens::camp::NavState;
    using fusionui::screens::camp::kNoPad;
    using fusionui::screens::camp::Palette;
    using fusionui::screens::camp::Category;
    using fusionui::screens::camp::TileState;
    using fusionui::screens::camp::Premium;
    using fusionui::screens::camp::Tile;
    using fusionui::screens::camp::CardRow;
    using fusionui::screens::camp::CardSection;
    using fusionui::screens::camp::Card;
    using fusionui::screens::camp::Hint;
    using fusionui::screens::camp::QuickOption;
    using fusionui::screens::camp::QuickMenu;
    using fusionui::screens::camp::BlueprintList;
    using fusionui::screens::camp::Model;
    using fusionui::screens::camp::Input;
    using fusionui::screens::camp::Result;
    using fusionui::screens::camp::Layout;
    Palette Tinted(const Palette& fo76, ImU32 hud);
    Input ReadInput();
    Result Menu(const Model& model, const Input& input);
    core::camp::NavShape ShapeOf(const Model& model);
    Layout LayoutFor(float width, float height);
    float CardBottom(const Card& card, bool expanded = false);
    std::pair<float, float> CardSpan(const Layout& layout, bool expanded = false);
    bool CardExpandable(const Card& card);
    float CardScrollLimit(const Card& card);
}
