#pragma once
#include "fusionui/Abi.h"

namespace fusionui::client {
    // The table fusionui::perf calls (the elected host's, or the preview's). Metrics constructed before
    // this are registered now.
    void SetPerfHostApi(const FUSIONUI_HostApi* host);
}
