#pragma once
#include "fusionui/fo76/Theme.h"
#include "fusionui/screens/InventoryModals.h"

namespace fusionui::fo76 {
    using fusionui::screens::ModalResult;
    using fusionui::screens::QuantityModal;
    using fusionui::screens::ConfirmModal;
    using fusionui::screens::RepairOption;
    using fusionui::screens::RepairModal;
    ModalResult Quantity(const QuantityModal& modal, core::inventory::Quantity& quantity, const fo4::Nav& nav);
    int Confirm(const ConfirmModal& modal, int& selected, const fo4::Nav& nav);
    int Repair(const RepairModal& modal, int& selected, const fo4::Nav& nav);

}
