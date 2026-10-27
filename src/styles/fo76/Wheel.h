#pragma once
// Fallout 76's favorites wheel (radialmenu.swf, favorites mode), drawn natively from a plain view model:
// the twelve quickkey slots on the ring, the selected item's centre info (icon, name, ammo, condition),
// the FAVORITES tab, the hint bar and the QUICK INVENTORY list (FO76's "radialInventory" frame).
// Positions are FO76's own (1920 x 1080 stage). Draw between fo4::hud::Begin(id, kFo76Stage) and End.
#include "fusionui/fo4/Hud.h"
#include "fusionui/fo76/Stage.h"
#include "core/FavoritesWheel.h"

#include <array>
#include <string>
#include <vector>

#include "fusionui/screens/Wheel.h"

namespace fusionui::fo76::wheel {
    using fusionui::screens::wheel::Row;
    using fusionui::screens::wheel::State;
    using fusionui::screens::wheel::Slot;
    using fusionui::screens::wheel::Palette;
    using fusionui::screens::wheel::Model;
    core::wheel::Facts FactsOf(const Model& model);
    void Draw(const Model& model, float alpha = 1.0F);
}
