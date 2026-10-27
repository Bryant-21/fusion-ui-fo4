#pragma once

#include "fusionui/Abi.h"

#include <functional>

namespace fusionui::game {
    void SetGameTaskHost(const FUSIONUI_HostApi* host);
    void QueueLocalGameTask(std::function<void()> task);
}
