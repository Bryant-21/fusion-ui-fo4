#include "fusionui/screens/Style.h"
#include "screens/Fallback.h"
#include "core/ScreenStyles.h"
#include "styles/common/Menus.h"
#include "styles/common/CampBuild.h"

#include <iostream>

namespace fusionui::screens {
    namespace {
        FallbackLogger fallbackLogger{};
        core::screens::Fallbacks reported;
    }

    namespace detail {
        bool NativeGameStyle() {
            const auto family = modern::theme::CurrentAppearance().family;
            return family == modern::theme::Family::Fallout4 || family == modern::theme::Family::Fallout3 ||
                   family == modern::theme::Family::NewVegas || family == modern::theme::Family::Skyrim ||
                   family == modern::theme::Family::Starfield;
        }

        void Fallback(std::string_view screen) {
            const auto requested = modern::theme::CurrentAppearance().family;
            if (!core::screens::FirstFallback(reported, screen, requested, modern::theme::Family::Fallout76)) return;
            const auto style = modern::appearance::FamilyName(requested);
            if (fallbackLogger) fallbackLogger(screen, style);
            else std::clog << "fusionui: " << screen << " has no " << style << " renderer; using fo76\n";
        }
    }

    void SetFallbackLogger(FallbackLogger logger) { fallbackLogger = logger; }
}

#include "styles/fo76/InventoryModals.h"
#include "styles/common/Inventory.h"
#include "styles/fo76/ButtonHints.h"
#include "styles/fo76/CampBuild.h"
#include "styles/fo76/DeathScreen.h"
#include "styles/fo76/DailyOps.h"
#include "styles/fo76/Wheel.h"
#include "styles/fo76/Inventory.h"
#include "styles/fo76/MainMenu.h"
#include "styles/fo76/Keypad.h"
#include "styles/fo76/PhotoMode.h"

namespace fusionui::screens {
    using detail::Fallback;
    using detail::NativeGameStyle;

    void ButtonHintBar(std::span<const ButtonHint> hints, float centerX, float plateTop, ImU32 ink) {
        if (NativeGameStyle()) return styles::inventory::ButtonHintBar(hints, centerX, plateTop, ink);
        Fallback("Button hints");
        fusionui::fo76::ButtonHintBar(hints, centerX, plateTop, ink);
    }

    ListResult InventoryPanel(Side side, const Panel& panel, std::span<const Item> items, ListModel& model, bool focused, const Nav& nav, bool prices) {
        if (NativeGameStyle()) return styles::inventory::InventoryPanel(side, panel, items, model, focused, nav, prices);
        Fallback("Inventory");
        return fusionui::fo76::InventoryPanel(side, panel, items, model, focused, nav, prices);
    }

    int TabBar(std::span<const std::string> tabs, int selected, std::span<const bool> enabled) {
        if (NativeGameStyle()) return styles::inventory::TabBar(tabs, selected, enabled);
        Fallback("Inventory");
        return fusionui::fo76::TabBar(tabs, selected, enabled);
    }

    float ItemCard(const Item& item, float centerX, float bottom, CardLook look) {
        if (NativeGameStyle()) return styles::inventory::ItemCard(item, centerX, bottom, look);
        Fallback("Item card");
        return fusionui::fo76::ItemCard(item, centerX, bottom, look);
    }

    void EffectLines(std::span<const Effect> effects, float centerX, float bottom) {
        if (NativeGameStyle()) return styles::inventory::EffectLines(effects, centerX, bottom);
        Fallback("Item card");
        fusionui::fo76::EffectLines(effects, centerX, bottom);
    }

    TradeResult TradeMenu(const TradeView& view, const Nav& nav) {
        if (NativeGameStyle()) return styles::inventory::TradeMenu(view, nav);
        Fallback("Trade");
        return fusionui::fo76::TradeMenu(view, nav);
    }

    int InspectCard(const Item& item, std::span<const Hint> hints) {
        if (NativeGameStyle()) return styles::inventory::InspectCard(item, hints);
        Fallback("Inspect");
        return fusionui::fo76::InspectCard(item, hints);
    }

    void WorkbenchItems(const WorkbenchList& list) {
        if (NativeGameStyle()) return styles::inventory::WorkbenchItems(list);
        Fallback("Workbench");
        fusionui::fo76::WorkbenchItems(list);
    }

    PipBoyResult InventoryList(const PipBoyView& view, const Nav& nav) {
        if (NativeGameStyle()) return styles::inventory::InventoryList(view, nav);
        Fallback("Inventory");
        return fusionui::fo76::InventoryList(view, nav);
    }

    ModalResult PipBoyQuantity(std::string_view header, core::inventory::Quantity& quantity, const Nav& nav, ImU32 tint) {
        if (NativeGameStyle()) return styles::inventory::PipBoyQuantity(header, quantity, nav, tint);
        Fallback("Modals");
        return fusionui::fo76::PipBoyQuantity(header, quantity, nav, tint);
    }

    KeypadOutcome Keypad(KeypadModel& model) {
        if (NativeGameStyle()) return styles::menus::Keypad(model);
        Fallback("Keypad");
        return fusionui::fo76::Keypad(model);
    }

    void PhotoMode(const PhotoModeView& view) {
        if (NativeGameStyle()) return styles::menus::PhotoMode(view);
        Fallback("Photo mode");
        fusionui::fo76::PhotoMode(view);
    }

    ModalResult Quantity(const QuantityModal& modal, core::inventory::Quantity& quantity, const fo4::Nav& nav) {
        if (NativeGameStyle()) return styles::inventory::Quantity(modal, quantity, nav);
        Fallback("Modals");
        return fusionui::fo76::Quantity(modal, quantity, nav);
    }

    int Confirm(const ConfirmModal& modal, int& selected, const fo4::Nav& nav) {
        if (NativeGameStyle()) return styles::inventory::Confirm(modal, selected, nav);
        Fallback("Modals");
        return fusionui::fo76::Confirm(modal, selected, nav);
    }

    int Repair(const RepairModal& modal, int& selected, const fo4::Nav& nav) {
        if (NativeGameStyle()) return styles::inventory::Repair(modal, selected, nav);
        Fallback("Modals");
        return fusionui::fo76::Repair(modal, selected, nav);
    }
}

namespace fusionui::screens::camp {
    Palette Tinted(const Palette& fo76, ImU32 hud) {
        return fusionui::fo76::camp::Tinted(fo76, hud);
    }

    Input ReadInput() {
        return fusionui::fo76::camp::ReadInput();
    }

    Result Menu(const Model& model, const Input& input) {
        if (NativeGameStyle()) return styles::camp::Menu(model, input);
        Fallback("C.A.M.P.");
        return fusionui::fo76::camp::Menu(model, input);
    }

    core::camp::NavShape ShapeOf(const Model& model) {
        if (NativeGameStyle()) return styles::camp::ShapeOf(model);
        return fusionui::fo76::camp::ShapeOf(model);
    }

    Layout LayoutFor(float width, float height) {
        if (NativeGameStyle()) return styles::camp::LayoutFor(width, height);
        return fusionui::fo76::camp::LayoutFor(width, height);
    }

    float CardBottom(const Card& card, bool expanded) {
        if (NativeGameStyle()) return styles::camp::CardBottom(card, expanded);
        return fusionui::fo76::camp::CardBottom(card, expanded);
    }

    std::pair<float, float> CardSpan(const Layout& layout, bool expanded) {
        if (NativeGameStyle()) return styles::camp::CardSpan(layout, expanded);
        return fusionui::fo76::camp::CardSpan(layout, expanded);
    }

    bool CardExpandable(const Card& card) {
        if (NativeGameStyle()) return styles::camp::CardExpandable(card);
        return fusionui::fo76::camp::CardExpandable(card);
    }
    float CardScrollLimit(const Card& card) {
        if (NativeGameStyle()) return styles::camp::CardScrollLimit(card);
        return fusionui::fo76::camp::CardScrollLimit(card);
    }
}

namespace fusionui::screens::death {
    void Draw(const Model& model, float alpha) {
        if (NativeGameStyle()) return styles::menus::DrawDeath(model, alpha);
        Fallback("Death");
        fusionui::fo76::death::Draw(model, alpha);
    }
}

namespace fusionui::screens::hud {
    int DailyOpsReportScreen(const DailyOpsReport& report, const Palette& palette, float alpha) {
        if (NativeGameStyle()) return styles::menus::DrawDailyOps(report, palette, alpha);
        Fallback("Daily Ops");
        return fusionui::fo76::dailyops::Draw(report, palette, alpha);
    }
}

namespace fusionui::screens::wheel {
    core::wheel::Facts FactsOf(const Model& model) {
        return fusionui::fo76::wheel::FactsOf(model);
    }

    void Draw(const Model& model, float alpha) {
        if (NativeGameStyle()) return styles::menus::DrawWheel(model, alpha);
        Fallback("Favorites");
        fusionui::fo76::wheel::Draw(model, alpha);
    }
}

namespace fusionui::screens::mainmenu {
    std::vector<core::mainmenu::Box> RowBoxes(const List& list) {
        if (NativeGameStyle()) return styles::menus::RowBoxes(list);
        return fusionui::fo76::mainmenu::RowBoxes(list);
    }

    ListResult DrawList(const List& list) {
        if (NativeGameStyle()) return styles::menus::DrawList(list);
        Fallback("Main menu");
        return fusionui::fo76::mainmenu::DrawList(list);
    }

    SettingsResult DrawSettings(const SettingsPage& page, const fo4::Nav& nav) {
        if (NativeGameStyle()) return styles::menus::DrawSettings(page, nav);
        Fallback("Settings");
        return fusionui::fo76::mainmenu::DrawSettings(page, nav);
    }

    void DrawLoading(const Loading& loading) {
        if (NativeGameStyle()) return styles::menus::DrawLoading(loading);
        Fallback("Loading");
        fusionui::fo76::mainmenu::DrawLoading(loading);
    }
}
