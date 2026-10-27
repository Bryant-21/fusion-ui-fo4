#pragma once
#include "fusionui/Abi.h"
#include "fusionui/screens/ButtonHints.h"
#include "fusionui/fo4/Style.h"
#include "core/CampBuildNav.h"

#include <cstdint>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace fusionui::screens::camp {
    using core::camp::Focus;
    using core::camp::NavState;

    inline constexpr std::uint32_t kNoPad = UINT32_MAX;

    // FO76's art at its default HUD colour (theme::kFo76DefaultHud).
    struct Palette {
        ImU32 text = IM_COL32(254, 254, 201, 255);    // labels, values, budget fill (FO76's HUD colour)
        ImU32 accent = IM_COL32(244, 202, 91, 255);   // titles, icons, selection
        ImU32 hostile = IM_COL32(250, 109, 65, 255);  // missing requirements, tile badges
        ImU32 stored = IM_COL32(224, 97, 57, 255);    // the budget's stored meter
        ImU32 blueprint = IM_COL32(114, 208, 244, 255);  // the blueprint budget bar's frame
        ImU32 friendly = IM_COL32(96, 220, 96, 255);  // public workshop materials
    };

    // The palette at the player's HUD colour (theme::HudTint on each entry).
    // Menu applies it with common::HudColor(); neutral fills (panels, plates, the hint strip) stay untinted.
    Palette Tinted(const Palette& fo76, ImU32 hud);

    struct Category {
        std::string icon;  // fusionui icon key (camp.category.*)
        std::string label;
        std::vector<std::string> subs;  // sub-categories, listed under the selected category
        bool enabled = true;
    };

    // FO76's BUILDABLE_STATE: which corner badge the tile shows.
    enum class TileState { Buildable, MissingMaterials, Unavailable, Locked };
    // FO76's PREMIUM_STATE: the Atomic Shop or Fallout 1st icon, gold when owned, red otherwise.
    enum class Premium { None, AtomicShop, Fallout1st };

    struct Tile {
        std::string name;
        ImTextureID thumbnail{};  // none draws FO76's missing-image glyph
        TileState state = TileState::Buildable;
        Premium premium = Premium::None;
        bool entitled = true;
        bool hasVariants = false;  // FO76's variant indicator
    };

    struct CardRow {
        std::string icon;  // optional fusionui icon key before the label (camp.category.power, currency.*)
        std::string label;
        std::string value;     // "12/4"; empty for perk rows
        bool missing = false;  // red row
        bool publicMaterials = false;  // green: a public workshop's materials are used
        bool tagged = false;           // Tag for Search mark beside the row
        bool transferLocked = false;   // the materials are transfer-locked
    };
    struct CardSection {
        std::string title;  // "Build Requirements:", "Input Requirements:", "Produces:"
        std::vector<CardRow> rows;
        bool fits = false;  // Build Requirements: cut to the card's height with a "More" row when collapsed
    };
    struct Card {
        std::string name;    // empty hides the card
        std::string banner;  // "--ACCESS DENIED--", "--RESERVED--", "--NEEDS FOUNDATION--", "FREE!"
        std::string built;   // build count, "3" or "1/5"; empty hides
        int stored = 0;      // shown when > 0
        std::string builtLabel = "BUILT:", storedLabel = "STORED:";
        std::vector<CardSection> sections;
        std::string description;
        std::string moreLabel = "More";
    };

    using Hint = ButtonHint;

    // FO76's QuickMenuPanel (Options, Store/Scrap): every option listed, the widest sets the width.
    struct QuickOption {
        std::string label;
        bool enabled = true;
    };
    struct QuickMenu {
        std::vector<QuickOption> options;  // empty: closed
        int selected = 0;
    };

    // FO76's blueprint save list: one column, seven rows visible.
    struct BlueprintList {
        std::string title = "MY BLUEPRINTS";
        std::vector<std::string> rows;  // empty: closed
        int selected = 0;
    };

    struct Model {
        bool modify = false;  // Modify mode is the active tab
        std::string buildLabel = "BUILD", modifyLabel = "MODIFY", modeKey = "Q)";
        std::uint32_t modePad = FUSIONUI_PAD_RB;
        bool gamepad = false;      // hints and the mode key show pad glyphs
        bool keepExpanded = false;  // Modify's Edit / Move / Blueprint keep the category panel open
        std::vector<Category> categories;
        std::vector<Tile> items;  // of the selected category / sub-category
        // The selected tile's variants (its own recipe first) and the group's name; fewer than 2 has no grid.
        std::vector<Tile> variants;
        std::string variantGroup;
        bool variantsMinimized = false;  // Toggle Variants: the grid shows one row
        NavState nav;
        float budget = 0.0F;           // 0..1 built
        float budgetStored = 0.0F;     // 0..1 stored, the meter from the bar's right end
        float blueprintBudget = -1.0F;  // 0..1 while placing a blueprint (replaces the budget bar); < 0 hides
        std::string budgetLabel = "BUDGET", storedLabel = "STORED]", blueprintBudgetLabel = "BLUEPRINT BUDGET";
        // FO76's "$VariantCount" ({1}/{2}), "$VariantNone" and "$None".
        std::string variantCount = "Variants: {1}/{2}", variantNone = "Variants: None", none = "None";
        std::string message;  // workshop message under the budget
        Card card;
        bool cardExpanded = false;  // Expand Info Card
        float cardScroll = 0.0F;    // expanded card's scroll, stage units
        QuickMenu quickMenu;
        BlueprintList blueprints;
        std::vector<Hint> hints;
        Palette palette;
        std::vector<std::string> cameraHelp;
        bool cameraDot = false;
    };

    struct Input {
        fo4::Nav nav;
        int cycle = 0;  // Z / C, LB: previous / next category
        bool toggleMode = false;  // Q, RB
    };
    // Keyboard / pad, read once per frame.
    Input ReadInput();

    struct Result {
        NavState nav;  // keep it and pass it back in the next model
        bool categoryChanged = false, itemChanged = false, variantChanged = false, focusChanged = false;
        bool activate = false;    // build the selected item (or variant)
        bool back = false;        // cancel on the category list: leave the menu
        bool toggleMode = false;  // Build <-> Modify
    };

    Result Menu(const Model& model, const Input& input);

    // The model's navigation shape (what Menu steps with). Render thread: sub-category heights are measured.
    core::camp::NavShape ShapeOf(const Model& model);

    // FO76's per-aspect frame for a display: where the side panels sit on the stage.
    struct Layout {
        float left = 0;  // LeftContainer offset from its 16:9 place (x 50)
        float card = 0;  // item card offset from x 1580
        ImVec2 quickMenu{990, 434};
        float hintTop = 1027;
        std::optional<std::array<float, 4>> preview;
    };
    Layout LayoutFor(float width, float height);

    // Stage y (FO76's 1920 x 1080) where the item card ends, and the card's stage x span.
    float CardBottom(const Card& card, bool expanded = false);
    std::pair<float, float> CardSpan(const Layout& layout, bool expanded = false);
    // Whether the card can expand (FO76 offers Expand Info Card only then): rows or text past its height.
    bool CardExpandable(const Card& card);
    float CardScrollLimit(const Card& card);
}
