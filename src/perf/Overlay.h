#pragma once
#include "fusionui/FusionUI.h"

namespace fusionui::perf {
    // The input-transparent overlay the host opens while the mode is not Off.
    Client& OverlayClient();
    // Registers the overlay and the MOD SETTINGS "Performance" page with this module's FusionUI.
    void RegisterPerfClients();
}
