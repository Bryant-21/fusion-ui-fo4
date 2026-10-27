#pragma once
#include "styles/fo76/InventoryModals.h"
// FO76's inventory screens drawn natively: the trade (barter) and container windows (securetrade), the
// inspect card (examinemenu), the item card they share and the Pip-Boy inventory page
// (newpipboy_invpage + newpipboymenu). Inputs are plain view models, so previews and tests drive them with
// fake data. Trade, container and inspect use FO76's fixed palette (fo76/Theme.h) on the 1920 x 1080
// stage: draw them between fo4::BeginMenu(id, dim, kFo76Stage) and fo4::EndMenu(). The Pip-Boy page is
// its own 876 x 700 stage (kPipBoyPage), its white art tinted with the Pip-Boy colour.
#include "fusionui/fo76/Theme.h"

#include <span>
#include <string>
#include <vector>

#include "fusionui/screens/Inventory.h"

namespace fusionui::fo76 {
    using fusionui::screens::Hint;
    using fusionui::screens::ListModel;
    using fusionui::screens::Nav;
    using fusionui::screens::kPipBoyPage;
    using fusionui::screens::StatRow;
    using fusionui::screens::Effect;
    using fusionui::screens::Item;
    using fusionui::screens::ListResult;
    using fusionui::screens::Panel;
    using fusionui::screens::Side;
    using fusionui::screens::CardLook;
    using fusionui::screens::TradeKind;
    using fusionui::screens::TradeView;
    using fusionui::screens::TradeResult;
    using fusionui::screens::PaperDoll;
    using fusionui::screens::PipBoyView;
    using fusionui::screens::PipBoyResult;
    ListResult InventoryPanel(Side side, const Panel& panel, std::span<const Item> items, ListModel& model, bool focused,
                              const Nav& nav, bool prices = false);
    int TabBar(std::span<const std::string> tabs, int selected, std::span<const bool> enabled = {});
    float ItemCard(const Item& item, float centerX, float bottom, CardLook look = CardLook::Trade);
    void EffectLines(std::span<const Effect> effects, float centerX, float bottom);
    TradeResult TradeMenu(const TradeView& view, const Nav& nav);
    int InspectCard(const Item& item, std::span<const Hint> hints);
    void WorkbenchItems(const fusionui::screens::WorkbenchList& list);
    PipBoyResult InventoryList(const PipBoyView& view, const Nav& nav);
    ModalResult PipBoyQuantity(std::string_view header, core::inventory::Quantity& quantity, const Nav& nav, ImU32 tint = 0);
}
