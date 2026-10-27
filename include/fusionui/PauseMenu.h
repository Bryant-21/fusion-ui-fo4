#pragma once
// Rows a plugin adds to the vanilla pause menu's own list (MainPanel_mc.List_mc.entryList), where FO4 and
// other mods put theirs: the row is real list data, so the stock menu, menu replacers and Tales' FO76 pause
// menu (which draws the live list) all show it. fusionui's MOD SETTINGS row is one of these.
#include <functional>
#include <string>

namespace fusionui::pause {
    struct Entry {
        std::string id;     // unique within the plugin, e.g. "tales.photomode"
        std::string label;  // shown as is (translate before adding)
        std::function<bool()> shown;     // null: always; false keeps the row out of the list
        std::function<bool()> enabled;   // null: always; false shows the row disabled
        std::function<void()> activate;  // the row was pressed (UI thread)
    };

    // Adds the row after SETTINGS (or after rows added before it). Call from the game thread at or after
    // kGameDataReady; the first call installs this plugin's pause-menu hooks.
    void AddEntry(Entry entry);
}
