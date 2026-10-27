#pragma once
#include "fusionui/Keybindings.h"
namespace RE { class ButtonEvent; }
namespace fusionui::game::McmRuntime {
    void RegisterPapyrus();
    void Start();
    void OnButton(const RE::ButtonEvent& event);
    std::vector<keys::Binding> CatalogBindings();
}
