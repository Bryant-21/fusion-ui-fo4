#pragma once
#include "fusionui/fo4/Style.h"
#include "core/InventoryRows.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fusionui::screens {
    // A modal's outcome this frame: `choice` 0 accepted, 1 cancelled, -1 still open.
    struct ModalResult {
        int choice = -1;
        bool changed = false;  // the quantity moved
    };

    // ModalSetQuantity_mc: header, value box and slider. Arrows / wheel step by one, bumpers and
    // Page Up / Down by Quantity::Step(), a click or drag on the track jumps.
    struct QuantityModal {
        std::string_view header = "SET QUANTITY";
        const char* accept = "ACCEPT";  // hint labels (C strings: the hint bar keeps the pointers)
        const char* cancel = "CANCEL";
    };
    ModalResult Quantity(const QuantityModal& modal, core::inventory::Quantity& quantity, const fo4::Nav& nav);

    // ModalConfirmPurchase_mc: header ("CONFIRM PURCHASE" / "CONFIRM SALE"), the item, the price with its
    // currency icon, an optional warning and a button list (OK / CANCEL). Disabled buttons can't be chosen.
    struct ConfirmModal {
        std::string_view header;
        std::string_view item;
        std::string_view value;  // empty hides the price line
        std::string_view currency = "currency.caps";
        std::string_view warning;
        std::span<const char* const> buttons;
        int disabled = -1;  // index of a button that is shown greyed and can't be chosen
    };
    // Returns the chosen button, `buttons.size() - 1` on cancel, or -1 while open.
    int Confirm(const ConfirmModal& modal, int& selected, const fo4::Nav& nav);

    // The repair confirm (examinemenu ConfirmPanel, the panel FO76's workbench repair confirms with): a #111111
    // 75 % panel at least 400 wide (content + 32), the question in 24 bold, the condition now and after the
    // selected repair, the repair options, the selected option's components ("Steel (3)", dimmed when short), a
    // notice and CONFIRM / CANCEL hints (Space / A, Tab / B), centred on the stage.
    struct RepairOption {
        std::string label;   // "BASIC REPAIR KIT (3)"
        std::string target;  // "100%"
        float after = 1.0F;  // condition after this repair, 0..2
        bool enabled = true;
        std::vector<std::string> costs;
        bool costsShort = false;  // the player lacks some of them
    };
    struct RepairModal {
        std::string_view header;  // "REPAIR 10MM PISTOL"
        float before = 1.0F;
        std::span<const RepairOption> options;
        std::string_view notice;
        const char* confirm = "CONFIRM";
        const char* cancel = "CANCEL";
    };
    // Returns the chosen option, `options.size()` on cancel, or -1 while open.
    int Repair(const RepairModal& modal, int& selected, const fo4::Nav& nav);
}
