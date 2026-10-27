#pragma once
#include "fusionui/screens/IconRun.h"
#include "fusionui/screens/InventoryModals.h"

#include <optional>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace fusionui::screens {
    using fo4::Hint;
    using fo4::ListModel;
    using fo4::Nav;

    // The Pip-Boy page's stage (newpipboymenu.swf).
    inline constexpr fo4::StageSize kPipBoyPage{876.0F, 700.0F};

    struct StatRow {
        std::string label;
        std::string value;
        std::string labelIcon;  // icon key before the label, e.g. "stat.ammo"
        std::string valueIcon;  // icon key before the value, e.g. "damage.ballistic", "currency.caps"
        int compare = 0;        // -3..3: comparison marks beside the row
    };

    // A legendary effect or a mod, with its star count.
    struct Effect {
        std::string text;
        int stars = 0;
        std::string icon;       // drawn after the text and stars, e.g. "item.unique", "item.setbonus.active"
        bool inactive = false;  // the unavailable colour: a set bonus short of its pieces
    };

    struct Item {
        std::string name;
        int count = 1;
        bool equipped = false;
        bool favorite = false;
        bool tagged = false;
        bool quest = false;
        bool setItem = false;
        bool setActive = false;
        bool locked = false;
        bool unique = false;    // FO76's unique gem after the name and stars
        bool disabled = false;  // can't equip / can't trade: drawn in the unavailable colour
        int legendaryStars = 0;
        float condition = -1.0F;  // 0..2 (CND row); below 0 hides the row, 0 is broken
        float rowMeter = -1.0F;   // 0..2: the trade row's condition bar (FO76's fridge / freezer freshness); below 0 hides it
        float weight = -1.0F;     // below 0 hides the Weight row
        int value = -1;           // below 0 hides the Value row; the barter rows' price column
        std::string currency = "currency.caps";
        std::string description;        // ItemCard_DescriptionEntry: wrapped text above the card's rows
        std::vector<StatRow> stats;     // rows between CND and Weight
        std::vector<Effect> effects;    // legendary effect lines over the card
        std::vector<Effect> mods;       // inspect: CURRENT MODS
        std::string typeLine;
        std::string iconKey;
        std::vector<hud::IconRun> iconRuns;  // the item's rendered icon; the list rows draw it, else iconKey
        ImU32 iconTint = 0;                  // the icon's colour on an unselected row; 0 = the row's ink
        std::vector<std::string> cells;      // trade rows: the Panel::headings columns, right-aligned at the row's end
    };

    struct ListResult {
        bool selectionChanged = false;
        int activated = -1;  // accept on the selected row, or a click on a row
        int heading = -1;    // a clicked Panel::headings column
        bool pager = false;  // the Panel::pager label was clicked
    };

    // A trade panel: header title, rows, footer carry weight (empty hides it) and currency amount.
    struct Panel {
        std::string title;
        std::string weight;
        std::string currencyAmount;
        std::string currency = "currency.caps";
        // Optional list header strip over the rows (it takes the first row slot): a search field, a column pager and
        // up to three sortable column headings over Item::cells. All empty / null leaves FO76's plain list.
        std::vector<std::string> headings;
        int sortedHeading = -1;  // draws the sort arrow
        bool descending = false;
        std::string pager;       // e.g. "Columns 1/3 >"; empty hides it
        std::string* search = nullptr;  // the search text; null hides the field. Esc clears it.
        std::string searchHint;         // shown while the field is empty
        bool focusSearch = false;       // put the keyboard into the field this frame
        std::string footnote;           // centred in the footer, e.g. "SHOWN WT 12.5"
    };
    enum class Side { Player, Other };

    // One trade panel at FO76's position for `side`. Both panels show their selection (as FO76's do); only
    // the focused one takes the keyboard / pad; hover selects and a click activates on both. `prices` adds
    // the barter price column (Item::value).
    ListResult InventoryPanel(Side side, const Panel& panel, std::span<const Item> items, ListModel& model, bool focused,
                              const Nav& nav, bool prices = false);

    // Category tab bar (LabelSelector) with its bumper keys and a selector that slides between tabs.
    // `enabled` (empty: all) greys out tabs with nothing in them; bumpers skip those. Returns the newly
    // selected tab or -1 (bumpers Z / C or LB / RB, or a click).
    int TabBar(std::span<const std::string> tabs, int selected, std::span<const bool> enabled = {});

    // Which card: trade / container (#333 box), inspect (#181C1D box).
    enum class CardLook { Trade, Inspect };
    // The item stats card ("ITEM STATS": Legendary, CND, stats, Weight, Value), centred on stage x
    // `centerX` and growing upward from `bottom`. Returns the card's top.
    float ItemCard(const Item& item, float centerX, float bottom, CardLook look = CardLook::Trade);
    // Legendary effect lines (22 bold), centred on `centerX` with their stars, ending above `bottom`.
    void EffectLines(std::span<const Effect> effects, float centerX, float bottom);

    enum class TradeKind { Barter, Container };
    // Everything the trade / container window shows. Spans and models are the caller's.
    struct TradeView {
        TradeKind kind = TradeKind::Barter;
        std::span<const std::string> tabs;
        std::span<const bool> tabEnabled;  // empty: every tab enabled
        int tab = 0;
        Panel player, other;
        std::span<const Item> playerItems, otherItems;
        ListModel* playerList = nullptr;  // required
        ListModel* otherList = nullptr;   // required
        int column = 0;  // 0 player, 1 other: the focused panel
        std::string corner;  // corner plate label; empty hides it
        std::span<const Hint> hints;
        std::string_view status;
        bool interactive = true;
        std::uint64_t session = 0;
    };
    struct PreviewSlot {
        float x = 0.5F, y = 0.4F;
        float size = 0.2F;  // fraction of display height for the host's separate 3D renderer
    };
    struct TradeResult {
        int tab = -1;   // new tab
        int column = 0; // focused panel after this frame
        ListResult player, other;
        int hint = -1;  // clicked hint
        bool inputConsumed = false;  // category navigation must not also transfer or close
        std::optional<PreviewSlot> preview;
    };
    // FO76's trade (barter) and container window: tabs, two panels, effects + card of the focused selection,
    // the corner plate and the hint bar. Buying and selling are per item (the caller's quantity / confirm
    // modals, fo76/Theme.h).
    TradeResult TradeMenu(const TradeView& view, const Nav& nav);

    // Inspect: title + stars, effect lines, ITEM STATS on the left, CURRENT MODS on the right, the INSPECT
    // corner plate and the hint bar. Returns the clicked hint or -1.
    int InspectCard(const Item& item, std::span<const Hint> hints);

    // A workbench (ExamineMenu) inventory row, drawn into the box of the stock row it stands in for.
    struct WorkbenchRow {
        Item item;             // Item::cells sit under WorkbenchList::headings
        std::string subtitle;  // small second line under the name; the fo76 style has none
        float top = 0, height = 0;
        bool selected = false;
    };
    // The workbench's item list, in the current stage's units, drawn over a stock list that keeps every input:
    // nothing here reacts to the mouse or keys.
    struct WorkbenchList {
        float x = 0, width = 0;
        float headingTop = 0;               // the headings' line, just above the first row
        std::string title;                  // over the names; empty with no headings hides the line
        std::vector<std::string> headings;  // over Item::cells, right-aligned
        std::span<const WorkbenchRow> rows;
    };
    void WorkbenchItems(const WorkbenchList& list);

    // The Pip-Boy page's paper doll (NewPaperDoll), shown for apparel: the base body (30 % of the Pip-Boy colour)
    // with the slots the selected piece covers drawn over it, and the per-part resistance of one damage type.
    struct PaperDoll {
        enum class Base { Underwear, Clothes, VaultSuit } base = Base::Underwear;
        bool torso = false, leftArm = false, rightArm = false, leftLeg = false, rightLeg = false;
        bool helmet = false, goggles = false, gasMask = false;
        std::string resistIcon = "resist.ballistic";
        int head = 0, chest = 0, leftArmResist = 0, rightArmResist = 0, leftLegResist = 0, rightLegResist = 0;
    };

    // Pip-Boy inventory page (876 x 700 stage): page header (STAT / INV / DATA / RADIO), the sub-tab
    // carousel, the item list, the selected item's card and the bottom bar (weight, caps, DMG / DR).
    struct PipBoyView {
        std::span<const std::string> tabs;  // sub tabs (Weapons, Apparel, ...)
        int tab = 0;
        int page = 1;  // 0 STAT, 1 INV, 2 DATA, 3 RADIO
        std::span<const Item> items;
        ListModel* list = nullptr;  // required
        std::string weight;          // "323 / 425"
        std::string currencyAmount;  // caps
        std::string damage;          // DMG / DR widget value, e.g. "47"; empty hides it
        std::string damageIcon = "stat.damage";
        ImU32 tint = 0;  // Pip-Boy colour; 0 = the HUD colour
        std::span<const Hint> hints;
        const PaperDoll* doll = nullptr;  // apparel: the paper doll over a shorter card (ItemCardScrollableMini)
        // Component view (Junk): `list` drives the components ("Steel (12)") instead of the items, and the
        // right side lists the items holding the selected component instead of the card.
        bool componentView = false;
        std::span<const Item> components;
        std::span<const std::string> owners;
        bool interactive = true;
    };
    struct PipBoyResult {
        int tab = -1;
        ListResult list;
        int hint = -1;
        bool inputConsumed = false;
    };
    PipBoyResult InventoryList(const PipBoyView& view, const Nav& nav);

    // The Pip-Boy page's quantity modal (QuantityMenuNEW: "DROP", or "DESTROY" when over-encumbered) over a dimmed
    // page: the header breaking the box's top edge, the count in 60 bold, the slider with its arrows and marker.
    // Arrows / wheel step by one, bumpers by a twentieth, triggers by a quarter; accept / cancel close it.
    ModalResult PipBoyQuantity(std::string_view header, core::inventory::Quantity& quantity, const Nav& nav, ImU32 tint = 0);
}
