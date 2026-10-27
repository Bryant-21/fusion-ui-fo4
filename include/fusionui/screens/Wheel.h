#pragma once
#include "fusionui/screens/IconRun.h"
#include "fusionui/screens/Style.h"
#include "core/FavoritesWheel.h"

#include <array>
#include <string>
#include <vector>

namespace fusionui::screens::wheel {
    using core::wheel::Row;
    using core::wheel::State;

    inline bool UsesList(modern::theme::Family family) {
        using modern::theme::Family;
        return family == Family::Skyrim || family == Family::Fallout3 || family == Family::NewVegas;
    }

    struct Slot {
        std::string name;  // empty: an empty slot
        int count = 0;     // "(N)" after the name when > 1
        std::string icon;  // fusionui icon key (item.type.*), used when `runs` is empty
        std::vector<hud::IconRun> runs;  // the item's own silhouette; fitted to the icon box
        bool equipped = false;
        std::string ammo;   // "10mm", shown as "10mm (120)"
        int ammoCount = 0;
        float condition = -1.0F;  // 0..2 of the CND meter; < 0 hides it
    };

    struct Palette {
        ImU32 ink = IM_COL32(0xFF, 0xFF, 0xCB, 0xFF);     // icons, hotkeys, ammo, CND (FO76 cream at its default HUD colour; drawn under the HUD colour)
        ImU32 accent = IM_COL32(0xF5, 0xCB, 0x5B, 0xFF);  // title, selection outline and glow, tab
    };

    struct Model {
        std::array<Slot, core::wheel::kSlots> slots;
        State state;                   // selection, list, fades
        std::vector<Row> rows;         // quick inventory, sorted (core::wheel::SortRows)
        bool tent = false;             // survival tent hint
        bool change = true, inspect = true;
        std::string tab = "FAVORITES", listTitle = "QUICK INVENTORY", noItems = "NO ITEMS";
        Palette palette;
    };

    // The facts core::wheel::Step needs, read off the model (its rows span stays valid while `model` lives).
    core::wheel::Facts FactsOf(const Model& model);

    // Draws the whole wheel; `alpha` fades it as a whole.
    void Draw(const Model& model, float alpha = 1.0F);
}
