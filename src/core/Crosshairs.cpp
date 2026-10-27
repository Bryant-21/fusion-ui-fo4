#include "core/Crosshairs.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <set>

namespace fusionui::core::crosshairs {
    bool ValidGame(std::string_view game) { return std::ranges::find(Games, game) != Games.end(); }

    bool ValidKey(std::string_view key) {
        return !key.empty() && key.size() <= MaxKeyLength && key.front() != '.' && key.back() != '.' &&
               key.find("..") == key.npos && std::ranges::all_of(key, [](char c) {
                   return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
               });
    }

    std::string CustomKey(std::string_view game, std::string_view filename) {
        if (!ValidGame(game)) return {};
        std::string normalized(filename);
        for (auto& c : normalized) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        auto key = "custom." + std::string(game) + "." + normalized;
        if (ValidKey(key)) return key;
        std::uint64_t hash = 14695981039346656037ULL;
        for (const unsigned char c : normalized) hash = (hash ^ c) * 1099511628211ULL;
        return std::format("custom.{}.id-{:016x}", game, hash);
    }

    Catalog Parse(const nlohmann::json& document) {
        Catalog out;
        if (!document.is_object() || !document.contains("version") || document["version"] != 1 ||
            !document.contains("crosshairs") || !document["crosshairs"].is_array() || document["crosshairs"].size() > 256) {
            out.warnings.emplace_back("Invalid built-in crosshair catalog (expected version 1).");
            return out;
        }
        std::set<std::string> keys, defaults;
        for (const auto& row : document["crosshairs"]) {
            if (!row.is_object() || !row.contains("key") || !row["key"].is_string() ||
                !row.contains("label") || !row["label"].is_string() || !row.contains("game") || !row["game"].is_string() ||
                (row.contains("default") && !row["default"].is_boolean())) {
                out.warnings.emplace_back("Skipped an invalid built-in crosshair entry.");
                continue;
            }
            fusionui::crosshairs::Entry entry{row["key"].get<std::string>(), row["label"].get<std::string>(),
                                          row["game"].get<std::string>(), row.value("default", false)};
            if (!ValidGame(entry.game) || !ValidKey(entry.key) || !entry.key.starts_with(entry.game + ".crosshair.") ||
                entry.label.empty() || entry.label.size() > 80 || !keys.insert(entry.key).second) {
                out.warnings.emplace_back("Skipped an invalid or duplicate built-in crosshair: " + entry.key);
                continue;
            }
            if (entry.isDefault && !defaults.insert(entry.game).second) {
                entry.isDefault = false;
                out.warnings.emplace_back("Ignored a second default crosshair for " + entry.game);
            }
            out.entries.push_back(std::move(entry));
        }
        return out;
    }

    const fusionui::crosshairs::Entry* Resolve(std::span<const fusionui::crosshairs::Entry> entries,
                                          std::string_view choice, std::string_view game) {
        if (choice == "none") return nullptr;
        if (game == "fo3") game = "fnv";
        const std::string legacy = choice.starts_with("fo3.crosshair.") ? "fnv" + std::string(choice.substr(3)) : "";
        if (!legacy.empty()) choice = legacy;
        if (choice != "theme")
            for (const auto& entry : entries)
                if (entry.key == choice) return &entry;
        for (const bool custom : {true, false})
            for (const auto candidate : {game, game == "fnv" && custom ? std::string_view("fo3") : std::string_view{}})
                for (const auto& entry : entries)
                    if (!candidate.empty() && entry.game == candidate && entry.isDefault && entry.custom == custom) return &entry;
        return nullptr;
    }
}
