#pragma once
#include "fusionui/screens/InventoryModals.h"
#include "core/MainMenuLayout.h"

#include <string>
#include <vector>

namespace fusionui::screens::mainmenu {
    // A picture the selected large title row shows, placed as FO76 places its Vault Boy: x from the row's label
    // left edge, y from the top of its banner, stage units.
    struct RowArt {
        ImTextureID texture{};
        float x = 0, y = 0, width = 0, height = 0;
        ImVec2 uv0{0, 0}, uv1{1, 1};  // the current frame's cell of an animated sheet
    };

    struct Row {
        std::string label;  // already translated
        bool disabled = false;
        bool minor = false;  // title menu's minor rows (SETTINGS, HELP, CREW, QUIT)
        bool busy = false;   // FO4's waitingForLoad spinner
        std::string image;   // fusionui icon key beside the selected large title row (mainmenu.play / mainmenu.character)
        RowArt art;          // drawn instead of `image` when it has a texture
    };

    enum class Kind { Title, Pause };

    struct List {
        Kind kind = Kind::Pause;
        std::vector<Row> rows;
        int selected = -1;
    };

    struct ListResult {
        int hovered = -1;  // row under the mouse this frame
        int clicked = -1;  // row the mouse clicked
    };

    // The row boxes the list draws and hit-tests (stage units from the left edge).
    std::vector<core::mainmenu::Box> RowBoxes(const List& list);
    ListResult DrawList(const List& list);

    struct Setting {
        // Info: read-only, shows options[value] where a stepper shows its value (a save's play time).
        enum class Kind { Stepper, Toggle, Slider, Info };
        std::string label;
        Kind kind = Kind::Stepper;
        std::vector<std::string> options;  // stepper values
        int value = 0;                     // option index, or 0 / 1 for a toggle
        float fraction = 0.0F;             // slider position 0..1
        bool disabled = false;
    };

    struct SettingsPage {
        std::vector<std::string> categories;  // the left column, drawn like the pause list
        int category = 0;                     // -1: a fixed-height details box (a save page), `categories` its lines
        ImTextureID preview{};                // the details box's picture (the selected save's screenshot)
        float previewWidth = 0, previewHeight = 0;
        std::string prompt;                   // a confirmation question ("Load this game?") above the hints
        std::vector<Setting> rows;
        int selected = 0;
        int scroll = 0;
        std::string onLabel = "ON", offLabel = "OFF";
        std::vector<fo4::Hint> hints;
    };

    struct SettingsResult {
        int selected = 0, scroll = 0;
        int changed = -1;  // row whose value moved
        int value = 0;     // its new option index / toggle state
        bool back = false;
        int hint = -1;  // clicked hint
        int pressed = -1;  // Info row the mouse clicked
    };

    // Up / down move the selection (wrapping, disabled rows skipped), left / right step the selected value,
    // accept flips a toggle, cancel reports back. The mouse selects on hover and clicks the arrows.
    SettingsResult DrawSettings(const SettingsPage& page, const fo4::Nav& nav);

    struct Loading {
        ImTextureID image{};  // full-screen photo / background; none leaves the game's own backdrop
        float imageWidth = 0, imageHeight = 0;
        std::string tip;
        std::string loadingLabel = "LOADING";
        std::string level;      // "LEVEL 25"; empty hides it
        bool minimal = false;   // FO4's SetMinimalMode: no strip, black backdrop
        bool spinnerOnly = false;  // FO4's SpinnerOnly: only the logo
        float seconds = 0;      // drives the logo's turn
    };
    void DrawLoading(const Loading& loading);
}
