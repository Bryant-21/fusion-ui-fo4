#pragma once
#include "fusionui/Abi.h"
namespace fusionui::game::KeybindingsRuntime {
    bool Register(const FUSIONUI_KeybindingProvider& provider);
    void Start();
}
