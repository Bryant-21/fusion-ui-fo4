#pragma once
#include <string_view>

namespace fusionui::screens::detail {
    // Shared by the menu and HUD dispatchers: a family without its own renderer falls back to fo76, logged once per screen.
    bool NativeGameStyle();
    void Fallback(std::string_view screen);
}
