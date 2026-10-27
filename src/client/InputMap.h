#pragma once
#include "fusionui/Abi.h"

#include <cstdint>
#include <imgui.h>

namespace fusionui::client {
    struct StickState {
        float lx{};
        float ly{};
        float rx{};
        float ry{};
    };

    ImGuiKey KeyFromVk(std::uint32_t vk);
    ImGuiKey KeyFromPad(std::uint32_t padButton);
    void ApplyEvents(ImGuiIO& io, const FUSIONUI_Frame& frame, StickState& sticks, float deadzone = 0.2F);
}
