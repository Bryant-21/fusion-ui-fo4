#pragma once
#include "fusionui/Abi.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace fusionui::core {
    class SettingsRegistry {
    public:
        // MOD SETTINGS replaces the pause menu, so a settings panel pauses whatever its flag says.
        static bool PausesGame(const FUSIONUI_ClientDesc& desc) {
            return (desc.flags & FUSIONUI_FLAG_PAUSES_GAME) != 0 ||
                   ((desc.flags & FUSIONUI_FLAG_SETTINGS) != 0 && desc.kind == FUSIONUI_KIND_MODAL);
        }
        void Add(FUSIONUI_ClientId id, const FUSIONUI_ClientDesc& desc);
        bool Contains(FUSIONUI_ClientId id) const;
        FUSIONUI_ClientId Resolve(FUSIONUI_ClientId id = 0) const;
        void Select(FUSIONUI_ClientId id);
        // Stands in for the selection until one is made, so a panel from an earlier session reopens.
        void Prefer(std::string_view label) { preferred_ = label; }
        const char* Label(FUSIONUI_ClientId id) const;
        const char* Category(FUSIONUI_ClientId id) const;
        std::uint32_t Panels(FUSIONUI_SettingsPanel* panels, std::uint32_t capacity) const;

    private:
        FUSIONUI_ClientId First() const;
        struct Panel {
            FUSIONUI_ClientId id;
            std::string label;
            std::string icon;
            std::string category;
        };
        std::deque<Panel> panels_;
        FUSIONUI_ClientId selected_{};
        std::string preferred_;
    };
}
