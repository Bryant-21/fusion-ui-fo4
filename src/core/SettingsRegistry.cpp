#include "core/SettingsRegistry.h"

#include <algorithm>
#include <cctype>
#include <tuple>
#include <cstddef>
#include <string_view>

namespace fusionui::core {
    void SettingsRegistry::Add(FUSIONUI_ClientId id, const FUSIONUI_ClientDesc& desc) {
        if (!id || desc.size < FUSIONUI_CLIENTDESC_MIN_SIZE || desc.kind != FUSIONUI_KIND_MODAL ||
            !(desc.flags & FUSIONUI_FLAG_SETTINGS) || Contains(id)) return;
        const char* label = desc.name;
        if (desc.size >= offsetof(FUSIONUI_ClientDesc, settingsLabel) + sizeof(desc.settingsLabel) &&
            desc.settingsLabel && desc.settingsLabel[0]) label = desc.settingsLabel;
        const char* icon = "\xEF\x80\x93";
        if (desc.size >= offsetof(FUSIONUI_ClientDesc, settingsIcon) + sizeof(desc.settingsIcon) &&
            desc.settingsIcon && desc.settingsIcon[0]) icon = desc.settingsIcon;
        const char* category = "Script Extender";
        if (desc.size >= offsetof(FUSIONUI_ClientDesc, settingsCategory) + sizeof(desc.settingsCategory) &&
            desc.settingsCategory && desc.settingsCategory[0]) category = desc.settingsCategory;
        if (std::string_view(category) == "UI 21") category = "Script Extender";
        panels_.push_back({id, label ? label : "", icon, category});
    }

    bool SettingsRegistry::Contains(FUSIONUI_ClientId id) const {
        return std::ranges::any_of(panels_, [id](const Panel& panel) { return panel.id == id; });
    }

    const char* SettingsRegistry::Category(FUSIONUI_ClientId id) const {
        for (const auto& panel : panels_) if (panel.id == id) return panel.category.c_str();
        return "Script Extender";
    }

    FUSIONUI_ClientId SettingsRegistry::Resolve(FUSIONUI_ClientId id) const {
        if (id) return Contains(id) ? id : 0;
        if (selected_) return selected_;
        if (const auto it = std::ranges::find(panels_, preferred_, &Panel::label); it != panels_.end()) return it->id;
        return First();
    }

    // The panel the sidebar lists first: BACUP, Fusion, Script Extender, then other categories A-Z, each by label.
    FUSIONUI_ClientId SettingsRegistry::First() const {
        const auto lower = [](std::string text) {
            std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return text;
        };
        const auto rank = [](const Panel& panel) {
            return panel.category == "BACUP" ? 0 : panel.category == "Fusion" ? 1 : panel.category == "Script Extender" ? 2 : 3;
        };
        const auto first = std::ranges::min_element(panels_, [&](const Panel& a, const Panel& b) {
            return std::tuple(rank(a), lower(a.category), lower(a.label)) < std::tuple(rank(b), lower(b.category), lower(b.label));
        });
        return first == panels_.end() ? 0 : first->id;
    }

    const char* SettingsRegistry::Label(FUSIONUI_ClientId id) const {
        for (const auto& panel : panels_) if (panel.id == id) return panel.label.c_str();
        return "";
    }

    void SettingsRegistry::Select(FUSIONUI_ClientId id) {
        if (Contains(id)) selected_ = id;
    }

    std::uint32_t SettingsRegistry::Panels(FUSIONUI_SettingsPanel* panels, std::uint32_t capacity) const {
        if (panels)
            for (std::size_t i = 0; i < std::min<std::size_t>(capacity, panels_.size()); ++i)
                panels[i] = {panels_[i].id, panels_[i].label.c_str(), panels_[i].id == selected_ ? 1u : 0u,
                             panels_[i].icon.c_str()};
        return static_cast<std::uint32_t>(panels_.size());
    }
}
