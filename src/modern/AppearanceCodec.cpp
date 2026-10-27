#include "fusionui/modern/AppearanceCodec.h"
#include "core/Crosshairs.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>

namespace fusionui::modern::appearance {
    namespace {
        std::optional<float> Number(const std::optional<std::string>& text) {
            if (!text || text->empty()) return std::nullopt;
            float value = 0.0F;
            const auto* begin = text->data();
            const auto result = std::from_chars(begin, begin + text->size(), value);
            if (result.ec != std::errc{} || result.ptr != begin + text->size() || !std::isfinite(value)) return std::nullopt;
            return value;
        }

        float Read(const Lookup& client, const Lookup& shared, std::string_view key, float fallback, float low, float high) {
            auto value = Number(client ? client(key) : std::nullopt);
            if (!value) value = Number(shared ? shared(key) : std::nullopt);
            return std::clamp(value.value_or(fallback), low, high);
        }
    }

    std::string_view FamilyName(theme::Family family) {
        return theme::Info(family).name;
    }

    theme::Family ParseFamily(std::string_view text) {
        if (text == "fo3") return theme::Family::NewVegas;
        for (const auto& info : theme::Families)
            if (info.name == text) return info.family;
        return theme::Family::Modern;
    }

    std::string_view PackChoice(std::string_view text, bool cursor) {
        if (text == "fo3") return "fnv";
        if (text == "theme" || text == "default" || (cursor && text == "simple")) return text;
        for (const auto& info : theme::Families)
            if (!info.pack.empty() && info.pack == text) return text;
        return "theme";
    }

    std::string Section(std::string_view client) {
        if (client.empty()) return "Modern";
        // A dot would nest the client's table one level deeper.
        std::string name(client);
        std::ranges::replace(name, '.', '_');
        return "Modern." + name;
    }

    std::string_view CrosshairChoice(std::string_view text) {
        return text == "theme" || text == "none" || core::crosshairs::ValidKey(text) ? text : "theme";
    }

    theme::Appearance Resolve(const Lookup& client, const Lookup& shared, const theme::Appearance& defaults) {
        theme::Appearance out;
        out.backgroundOpacity = Read(client, shared, "BackgroundOpacity", defaults.backgroundOpacity, theme::MinBackgroundOpacity, 1.0F);
        out.textScale = Read(client, shared, "TextScale", defaults.textScale, theme::MinTextScale, theme::MaxTextScale);
        out.tableTextSize =
            Read(client, shared, "TableTextSize", defaults.tableTextSize, theme::MinTableTextSize, theme::MaxTableTextSize);
        auto family = client ? client("Theme") : std::nullopt;
        if (!family && shared) family = shared("Theme");
        out.family = family ? ParseFamily(*family) : theme::Info(defaults.family).family;
        const auto choice = [&](std::string_view key, const std::string& fallback, bool cursor) {
            auto value = client ? client(key) : std::nullopt;
            if (!value && shared) value = shared(key);
            return std::string(PackChoice(value.value_or(fallback), cursor));
        };
        out.cursor = choice("Cursor", defaults.cursor, true);
        out.iconSet = choice("IconSet", defaults.iconSet, false);
        auto crosshair = client ? client("Crosshair") : std::nullopt;
        if (!crosshair && shared) crosshair = shared("Crosshair");
        out.crosshair = CrosshairChoice(crosshair.value_or(defaults.crosshair));
        return out;
    }

    std::vector<std::pair<std::string, std::string>> Format(const theme::Appearance& appearance) {
        return {{"BackgroundOpacity", std::format("{:.2f}", appearance.backgroundOpacity)},
                {"TextScale", std::format("{:.2f}", appearance.textScale)},
                {"TableTextSize", std::format("{:.2f}", appearance.tableTextSize)},
                {"Cursor", std::string(PackChoice(appearance.cursor, true))},
                {"IconSet", std::string(PackChoice(appearance.iconSet, false))},
                {"Crosshair", std::string(CrosshairChoice(appearance.crosshair))}};
    }
}
