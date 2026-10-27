#include <F4SE/F4SE.h>

#include "fusionui/FusionUI.h"
#include "fusionui/Tasks.h"
#include "fusionui/Windows.h"
#include "fusionui/Settings.h"

#include <spdlog/spdlog.h>

#include <Windows.h>

namespace fusionui {
    namespace {
        const Window* FindInstalled(std::string_view id) {
            for (const auto* window : InstalledWindows({}))
                if (window->id == id) return window;
            return nullptr;
        }

        void Send(const Window& window) {
            QueueGameTask([plugin = window.plugin] {
                auto* messaging = F4SE::GetMessagingInterface();
                if (!messaging || !messaging->Dispatch(kOpenWindowMessage, nullptr, 0, plugin))
                    spdlog::warn("FusionUI: could not ask {} to open its window", plugin);
            });
        }
    }

    std::vector<const Window*> InstalledWindows(std::string_view self) {
        return InstalledWindows(self, [](const Window& window) {
            return window.id == "fusionuiSettings" ? !SettingsPanels().empty() : ::GetModuleHandleW(window.module) != nullptr;
        });
    }

    bool OpenWindow(std::string_view id) {
        if (id == "fusionuiSettings") return OpenSettings();
        const auto* window = FindInstalled(id);
        if (window) Send(*window);
        return window != nullptr;
    }

    bool SwitchToWindow(Client& self, std::string_view id) {
        const auto* window = FindInstalled(id);
        if (!window) return false;
        Close(self);
        if (id == "fusionuiSettings") QueueGameTask([] { OpenSettings(); });
        else Send(*window);
        return true;
    }
}
