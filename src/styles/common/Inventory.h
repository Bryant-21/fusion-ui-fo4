#pragma once
#include "fusionui/screens/Inventory.h"
#include "fusionui/screens/ButtonHints.h"

namespace fusionui::styles::inventory {
    namespace view = fusionui::screens;
    view::ListResult InventoryPanel(view::Side, const view::Panel&, std::span<const view::Item>, view::ListModel&, bool focused, const view::Nav&, bool prices);
    int TabBar(std::span<const std::string>, int selected, std::span<const bool> enabled);
    float ItemCard(const view::Item&, float centerX, float bottom, view::CardLook);
    void EffectLines(std::span<const view::Effect>, float centerX, float bottom);
    view::TradeResult TradeMenu(const view::TradeView&, const view::Nav&);
    int InspectCard(const view::Item&, std::span<const view::Hint>);
    void WorkbenchItems(const view::WorkbenchList&);
    view::PipBoyResult InventoryList(const view::PipBoyView&, const view::Nav&);
    view::ModalResult PipBoyQuantity(std::string_view, core::inventory::Quantity&, const view::Nav&, ImU32 tint);
    view::ModalResult Quantity(const view::QuantityModal&, core::inventory::Quantity&, const view::Nav&);
    int Confirm(const view::ConfirmModal&, int& selected, const view::Nav&);
    int Repair(const view::RepairModal&, int& selected, const view::Nav&);
    void ButtonHintBar(std::span<const view::ButtonHint>, float centerX, float plateTop, ImU32 ink);
}
