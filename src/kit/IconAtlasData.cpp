#include "kit/IconAtlasData.h"

#include <nlohmann/json.hpp>
#include <cmath>

namespace fusionui::kit {
    namespace {
        bool Rect(const nlohmann::json& j, IconRect& out) {
            if (!j.is_array() || j.size() != 4) return false;
            for (const auto& v : j) if (!v.is_number() || !std::isfinite(v.get<float>())) return false;
            out = {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
            return true;
        }
    }

    std::optional<IconAtlasData> ParseIconAtlas(std::string_view json) {
        const auto root = nlohmann::json::parse(json, nullptr, false);
        if (!root.is_object() || root.value("version", 0) != 1) return std::nullopt;
        IconAtlasData data;
        data.texture = root.value("texture", "");
        data.width = root.value("width", 0);
        data.height = root.value("height", 0);
        if (data.texture.empty() || data.width <= 0 || data.height <= 0) return std::nullopt;
        const auto icons = root.find("icons");
        if (icons == root.end() || !icons->is_object()) return std::nullopt;
        for (const auto& [name, value] : icons->items()) {
            IconEntry entry;
            entry.tintable = value.value("tintable", false);
            entry.mask = value.value("mask", false);
            entry.monochrome = value.value("monochrome", false);
            entry.alphaMask = value.value("alphaMask", false);
            entry.shadow = value.value("shadow", true);
            entry.aspect = value.value("aspect", 1.0F);
            if (!std::isfinite(entry.aspect) || entry.aspect <= 0) return std::nullopt;
            entry.aperture = value.value("aperture", 0.0F);
            entry.luminanceMask = value.value("luminanceMask", false);
            if (!std::isfinite(entry.aperture) || entry.aperture < 0 || entry.aperture > 1) return std::nullopt;
            if (value.contains("clip") && !Rect(value["clip"], entry.clip)) return std::nullopt;
            if (entry.clip.u0 < 0 || entry.clip.v0 < 0 || entry.clip.u1 > 1 || entry.clip.v1 > 1 ||
                entry.clip.u1 <= entry.clip.u0 || entry.clip.v1 <= entry.clip.v0) return std::nullopt;
            if (value.contains("layers")) {
                if (!value["layers"].is_array()) return std::nullopt;
                for (const auto& part : value["layers"]) {
                    IconEntry::Layer layer;
                    if (!part.is_object() || !Rect(part.value("shape", nlohmann::json()), layer.shape) ||
                        !Rect(part.value("bounds", nlohmann::json()), layer.bounds)) return std::nullopt;
                    if (layer.bounds.u1 <= layer.bounds.u0 || layer.bounds.v1 <= layer.bounds.v0) return std::nullopt;
                    entry.layers.push_back(layer);
                }
                if (!entry.mask) return std::nullopt;
            }
            if (!Rect(value.value("shape", nlohmann::json()), entry.shape)) return std::nullopt;
            const auto bands = value.value("bands", nlohmann::json::array());
            if (entry.tintable) {
                if (bands.size() != 4) return std::nullopt;
                for (std::size_t i = 0; i < 4; ++i)
                    if (!Rect(bands[i], entry.bands[i])) return std::nullopt;
            }
            data.icons.emplace(name, entry);
        }
        return data;
    }
}
