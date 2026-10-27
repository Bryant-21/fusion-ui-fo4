#include "client/SettingsHost.h"
#include "fusionui/Common.h"
#include "fusionui/Settings.h"
#include "fusionui/Texture.h"
#include "fusionui/modern/Theme.h"
#include "fusionui/modern/Widgets.h"

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace fusionui::client {
    namespace {
#include "client/SettingsLogos.inc"

        // Section logos; device-level, so every settings host shares them.
        const Texture* CategoryLogo(ID3D11Device* device, std::string_view category) {
            static std::optional<Texture> fusion, bacup;
            static bool loaded{};
            if (!loaded && device) {
                loaded = true;
                fusion = LoadImageFile(device, std::as_bytes(std::span(kFusionLogoPng)));
                bacup = LoadImageFile(device, std::as_bytes(std::span(kBacupLogoPng)));
            }
            if (category == "Fusion" && fusion) return &*fusion;
            if (category == "BACUP" && bacup) return &*bacup;
            return nullptr;
        }
    }

    namespace {
        // shortcut: each mod's page draws its own search bar now and nothing writes this; SettingsSearch() and
        // SettingsMatches() stay exported for Fusion HUD builds that still call them; drop both once none do.
        char settingsSearch[128]{};

        bool ContainsFolded(std::string_view text, std::string_view query) {
            return !std::ranges::search(text, query, [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); })
                        .empty();
        }
    }

    std::string SettingsHost::Title(const char* label, const char* version) {
        if (!version || !version[0]) return {};
        std::string title = label && label[0] ? std::string(label) + "  " : std::string();
        if (version[0] >= '0' && version[0] <= '9') title += 'v';
        return title + version;
    }

    void SettingsHost::Draw(Client& client, const FrameContext& frame, bool familyControlled) {
        namespace theme = modern::theme;
        namespace w = modern::w;
        appearance_.familyControlled = familyControlled;
        theme::Apply(common::UiScale());
        if (!std::exchange(navigationRestored_, true)) {
            fullscreen_ = theme::SavedFullscreen("FUSIONUI_Settings");
            autoExpand_ = theme::SavedWindowFlag("FUSIONUI_Settings", "ExpandModSettings");
            navigationExpanded_ = autoExpand_;
        }
        const bool autoExpand = autoExpand_;
        const auto popupOpen = [] {
            return ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        };
        // ImGui can dismiss a popup in NewFrame before the host sees its cancelling key.
        const bool backBlocked = ImGui::GetIO().WantTextInput || popupWasOpen_ || popupOpen();
        bool close{};
        const bool armed = closeArmed_;
        if (!ImGui::IsKeyDown(ImGuiKey_Escape) && !ImGui::IsKeyDown(ImGuiKey_GamepadFaceRight)) closeArmed_ = true;
        ImGui::PushFont(theme::CurrentFonts().body, theme::Px(theme::BodySize));
        ImVec2 size;
        if (w::BeginAppWindow("##fusionuiSettings", {frame.width, frame.height}, fullscreen_, {theme::Px(1500), theme::Px(900)}, size)) {
            ImGui::BeginChild("##settingsHeader", {0, theme::Px(68)}, ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoScrollbar);
            ImGui::SetCursorPos({theme::Px(24), theme::Px(20)});
            {
                w::FontScope title(theme::CurrentFonts().bold, 22);
                ImGui::PushStyleColor(ImGuiCol_Text, theme::Accent);
                ImGui::TextUnformatted(modern::icon::Gear);
                ImGui::PopStyleColor();
                ImGui::SameLine(0, theme::Px(12));
                ImGui::TextUnformatted("MOD SETTINGS");
            }
            if (!title_.empty()) {
                const float titleBottom = ImGui::GetItemRectMax().y;
                ImGui::SameLine(0, theme::Px(18));
                ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, titleBottom - ImGui::GetTextLineHeight() - theme::Px(2)});
                ImGui::PushStyleColor(ImGuiCol_Text, theme::Muted);
                ImGui::TextUnformatted(title_.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - theme::Px(24) - 4 * ImGui::GetFrameHeight() - 3 * ImGui::GetStyle().ItemSpacing.x);
            w::QuickSettings("settingsAppearance", appearance_, [&] {
                w::Heading("Mod list");
                if (w::Switch("Expand mod settings on open", autoExpand)) {
                    theme::SaveWindowFlag("FUSIONUI_Settings", "ExpandModSettings", !autoExpand);
                    autoExpand_ = navigationExpanded_ = !autoExpand;
                }
            }, "Settings appearance");
            ImGui::SameLine();
            w::WindowLauncher(client, "fusionuiSettings");
            ImGui::SameLine();
            if (w::FullscreenToggle(fullscreen_)) theme::SaveFullscreen("FUSIONUI_Settings", fullscreen_);
            ImGui::SameLine();
            close = w::Icon("closeSettings", modern::icon::Close, "Close (Esc / B)");
            ImGui::EndChild();
            ImGui::Separator();

            const float sidebarWidth = std::min(theme::Px(270), ImGui::GetContentRegionAvail().x * 0.3F);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::Translucent(theme::Sidebar));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {theme::Px(14), theme::Px(20)});
            ImGui::BeginChild("##settingsMods", {sidebarWidth, 0}, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
            ImGui::PopStyleVar();
            auto panels = SettingsPanels();
            const auto lower = [](std::string text) {
                std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return text;
            };
            std::ranges::stable_sort(panels, {}, [&](const auto& panel) { return lower(panel.label ? panel.label : ""); });
            std::vector<std::string> categories;
            for (const auto& panel : panels) {
                const std::string category = SettingsCategory(panel.id);
                if (std::ranges::find(categories, category) == categories.end()) categories.push_back(category);
            }
            std::ranges::stable_sort(categories, {}, lower);
            std::stable_partition(categories.begin(), categories.end(), [](const auto& category) { return category == "Script Extender"; });
            std::stable_partition(categories.begin(), categories.end(), [](const auto& category) { return category == "Fusion"; });
            std::stable_partition(categories.begin(), categories.end(), [](const auto& category) { return category == "BACUP"; });
            const auto favoriteKey = [](const auto& panel) { return std::string("Favorite:") + (panel.label ? panel.label : ""); };
            const auto isFavorite = [&](const auto& panel) {
                const auto key = favoriteKey(panel);
                const auto found = favorites_.find(key);
                return found != favorites_.end() ? found->second
                                                 : favorites_.emplace(key, theme::SavedWindowFlag("FUSIONUI_Settings", key)).first->second;
            };
            const auto drawPanel = [&](const auto& panel) {
                ImGui::PushID(static_cast<int>(panel.id));
                const bool favorite = isFavorite(panel);
                const float side = ImGui::GetFrameHeight();
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 0});
                if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_NoSavedSettings)) {
                    ImGui::TableSetupColumn("mod", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("favorite", ImGuiTableColumnFlags_WidthFixed, side + theme::Px(4));
                    ImGui::TableNextColumn();
                    if (w::NavigationItem("##mod", panel.label, panel.icon, panel.selected != 0,
                                          panel.selected ? &navigationExpanded_ : nullptr)) OpenSettings(panel.id);
                    ImGui::TableNextColumn();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (theme::Px(44) - side) / 2);
                    if (w::Icon("favorite", modern::icon::Star, favorite ? "Remove from favorites" : "Add to favorites", favorite))
                    {
                        favorites_[favoriteKey(panel)] = !favorite;
                        theme::SaveWindowFlag("FUSIONUI_Settings", favoriteKey(panel), !favorite);
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
                if (panel.selected && navigationExpanded_) {
                    ImGui::Indent(theme::Px(14));
                    client.DrawSettingsNavigation(frame);
                    ImGui::Unindent(theme::Px(14));
                }
                ImGui::PopID();
            };
            if (std::ranges::any_of(panels, isFavorite)) {
                w::Heading("Favorites");
                for (const auto& panel : panels) if (isFavorite(panel)) drawPanel(panel);
                ImGui::Dummy({0, theme::Px(12)});
            }
            for (const auto& category : categories) {
                const auto listed = [&](const auto& panel) { return category == SettingsCategory(panel.id) && !isFavorite(panel); };
                if (!std::ranges::any_of(panels, listed)) continue;
                const auto* logo = CategoryLogo(frame.d3d, category);
                const float logoSide = theme::Px(22), logoGap = theme::Px(8);
                const float left = ImGui::GetCursorScreenPos().x;
                if (logo) ImGui::Indent(logoSide + logoGap);
                w::Heading(category.c_str());
                if (logo) {
                    ImGui::Unindent(logoSide + logoGap);
                    const float mid = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) / 2;
                    ImGui::GetWindowDrawList()->AddImage(logo->Id(), {left, mid - logoSide / 2}, {left + logoSide, mid + logoSide / 2});
                }
                for (const auto& panel : panels) if (listed(panel)) drawPanel(panel);
                ImGui::Dummy({0, theme::Px(12)});
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            const auto sidebarMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddLine({sidebarMax.x, ImGui::GetItemRectMin().y}, sidebarMax, theme::Border);
            ImGui::SameLine(0, 0);
            ImGui::BeginChild("##settingsBody", {0, 0}, ImGuiChildFlags_NavFlattened);
            client.Draw(frame);
            ImGui::EndChild();
        }
        ImGui::End();
        const auto back = [](ImGuiKey key) {
            return ImGui::TestKeyOwner(key, ImGuiKeyOwner_NoOwner) && ImGui::IsKeyPressed(key, false);
        };
        if (armed && !backBlocked && !popupOpen() && !ImGui::GetIO().WantTextInput &&
            (back(ImGuiKey_Escape) || back(ImGuiKey_GamepadFaceRight))) close = true;
        popupWasOpen_ = popupOpen();
        ImGui::PopFont();
        if (close) fusionui::Close(client);
    }
}

namespace fusionui {
    std::string_view SettingsSearch() { return client::settingsSearch; }

    bool SettingsMatches(std::string_view label, std::string_view help) {
        const std::string_view query = client::settingsSearch;
        return query.empty() || client::ContainsFolded(label, query) || client::ContainsFolded(help, query);
    }
}
