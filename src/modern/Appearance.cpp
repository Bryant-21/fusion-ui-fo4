#include "fusionui/Paths.h"
#include "fusionui/Common.h"
#include "fusionui/modern/AppearanceCodec.h"
#include "fusionui/modern/Theme.h"
#include "kit/Settings.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>

namespace fusionui::modern::theme {
    namespace {
        constexpr auto kSection = "Modern";
        constexpr auto kWindowsSection = "Windows";

        struct Store {
            Appearance appearance;
            Appearance defaults;
            std::string section = kSection;
            std::filesystem::file_time_type loadedWrite{};
            std::chrono::steady_clock::time_point nextCheck{};
            bool loaded = false;
        };

        Store& Current() {
            return common::ContextSlot<Store>();
        }

        const std::filesystem::path& File() {
            return settings::Shared().Path();
        }

        std::optional<std::string> Text(const fusionframework::config::Value* value) {
            if (!value) return std::nullopt;
            if (value->is_string()) return value->as_string();
            if (value->is_floating()) return std::format("{}", value->as_floating());
            if (value->is_integer()) return std::to_string(value->as_integer());
            return std::nullopt;
        }

        appearance::Lookup Reader(std::string section) {
            return [section = std::move(section)](std::string_view key) -> std::optional<std::string> {
                return Text(settings::Shared().Load().Find(section, key));
            };
        }

        std::filesystem::file_time_type LastWrite() {
            std::error_code error;
            const auto time = std::filesystem::last_write_time(File(), error);
            return error ? std::filesystem::file_time_type{} : time;
        }

        void Load(Store& store) {
            const auto client = Reader(store.section);
            const auto shared = store.section == kSection ? appearance::Lookup{} : Reader(kSection);
            store.appearance = appearance::Resolve(client, shared, store.defaults);
            store.loadedWrite = LastWrite();
            store.loaded = true;
        }
    }

    void SetAppearanceClient(std::string_view client, const Appearance& defaults) {
        auto& store = Current();
        const auto section = appearance::Section(client);
        if (store.section == section && store.loaded && store.defaults.backgroundOpacity == defaults.backgroundOpacity &&
            store.defaults.textScale == defaults.textScale && store.defaults.tableTextSize == defaults.tableTextSize &&
            store.defaults.family == defaults.family && store.defaults.cursor == defaults.cursor &&
            store.defaults.iconSet == defaults.iconSet && store.defaults.crosshair == defaults.crosshair) return;
        store.section = section;
        store.defaults = defaults;
        store.loaded = false;
    }

    const Appearance& DefaultAppearance() { return Current().defaults; }

    const Appearance& CurrentAppearance() {
        auto& store = Current();
        const auto now = std::chrono::steady_clock::now();
        if (!store.loaded) {
            Load(store);
        } else if (now >= store.nextCheck) {
            if (LastWrite() != store.loadedWrite) Load(store);
        }
        if (now >= store.nextCheck) store.nextCheck = now + std::chrono::seconds(1);
        return store.appearance;
    }

    bool SavedFullscreen(std::string_view window) {
        return SavedWindowFlag(window, "Fullscreen", false);
    }

    void SaveFullscreen(std::string_view window, bool fullscreen) {
        SaveWindowFlag(window, "Fullscreen", fullscreen);
    }

    bool SavedWindowFlag(std::string_view window, std::string_view name, bool fallback) {
        return settings::Shared().Load().GetBool(kWindowsSection, std::string(window) + std::string(name), fallback);
    }

    void SaveWindowFlag(std::string_view window, std::string_view name, bool value) {
        settings::Shared().Update([&](auto& document) { document.Set(kWindowsSection, std::string(window) + std::string(name), value); });
    }

    std::string SavedWindowText(std::string_view window, std::string_view name) {
        return settings::Shared().Load().GetString(kWindowsSection, std::string(window) + std::string(name), "");
    }

    void SaveWindowText(std::string_view window, std::string_view name, std::string_view value) {
        settings::Shared().Update(
            [&](auto& document) { document.Set(kWindowsSection, std::string(window) + std::string(name), std::string(value)); });
    }

    void SetAppearance(const Appearance& appearance, bool save) {
        auto& store = Current();
        store.appearance = {std::clamp(appearance.backgroundOpacity, MinBackgroundOpacity, 1.0F),
                            std::clamp(appearance.textScale, MinTextScale, MaxTextScale),
                            std::clamp(appearance.tableTextSize, MinTableTextSize, MaxTableTextSize),
                            Info(appearance.family).family, std::string(appearance::PackChoice(appearance.cursor, true)),
                            std::string(appearance::PackChoice(appearance.iconSet, false)),
                            std::string(appearance::CrosshairChoice(appearance.crosshair))};
        store.loaded = true;
        if (!save) return;
        settings::Shared().Update([&](auto& document) {
            for (const auto& [key, value] : appearance::Format(store.appearance)) {
                double number = 0.0;
                const auto end = value.data() + value.size();
                if (!value.empty() && std::from_chars(value.data(), end, number).ptr == end)
                    document.Set(store.section, key, number);
                else
                    document.Set(store.section, key, value);
            }
        });
        store.loadedWrite = LastWrite();
    }

    void SetFamily(Family family, bool allWindows) {
        auto& store = Current();
        CurrentAppearance();
        family = Info(family).family;
        store.appearance.family = family;
        const std::string name(appearance::FamilyName(family));
        settings::Shared().Update([&](auto& document) {
            if (!allWindows) {
                document.Set(store.section, "Theme", name);
                return;
            }
            document.Set(kSection, "Theme", name);
            for (auto& entry : document.Root().at(kSection).as_table())
                if (entry.second.is_table()) entry.second.as_table().erase("Theme");
        });
        store.loadedWrite = LastWrite();
    }
}
