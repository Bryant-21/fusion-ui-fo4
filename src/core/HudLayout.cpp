#include "core/HudLayout.h"
#include "fusionframework/Config.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>

namespace fusionui::core::hudlayout {
    namespace {
        constexpr Info kNone{Id::None, "", "", Group::Status, false, false};

        std::string Table(Family family, std::string_view preset, std::string_view component = {}) {
            std::string table = preset.empty() ? std::string(FamilyKey(family)) : "presets." + std::string(preset) + "." + std::string(FamilyKey(family));
            if (!component.empty()) table += "." + std::string(component);
            return table;
        }

        cfg::Value* Find(cfg::Value& root, std::string_view table, std::string_view key) {
            cfg::Value* at = &root;
            for (const auto& part : cfg::SplitPath(table)) {
                if (!at->is_table() || !at->as_table().contains(part)) return nullptr;
                at = &at->as_table().at(part);
            }
            return at->is_table() && at->as_table().contains(std::string(key)) ? &at->as_table().at(std::string(key)) : nullptr;
        }

        template <class T>
        void SetCommented(cfg::Document& document, const std::string& table, std::string_view key, T value, std::string_view comment) {
            document.Set(table, key, value);
            if (auto* slot = Find(document.Root(), table, key); slot && slot->comments().empty())
                slot->comments().push_back("# " + std::string(comment));
        }

        std::mutex& FileMutex() {
            static std::mutex mutex;
            return mutex;
        }
    }

    const Info& InfoOf(Id id) {
        for (const auto& info : kComponents)
            if (info.id == id) return info;
        return kNone;
    }

    std::optional<Id> FromKey(std::string_view key) {
        for (const auto& info : kComponents)
            if (info.key == key) return info.id;
        return std::nullopt;
    }

    std::string_view FamilyKey(Family family) {
        switch (family) {
        case Family::Fo4: return "fo4";
        case Family::Pipboy3000: return "pipboy3000";
        case Family::Skyrim: return "skyrim";
        case Family::Starfield: return "starfield";
        default: return "fo76";
        }
    }

    Layout Minimal() {
        Layout layout;
        for (const auto& info : kComponents) layout[info.id].visible = info.minimal;
        return layout;
    }

    Rect Union(Rect a, Rect b) {
        if (a.Empty()) return b;
        if (b.Empty()) return a;
        return {std::min(a.x0, b.x0), std::min(a.y0, b.y0), std::max(a.x1, b.x1), std::max(a.y1, b.y1)};
    }

    Placement Place(float edgeX, float edgeY, float base, const Transform& t, float pivotX, float pivotY) {
        const float s = std::clamp(t.scale, kMinScale, kMaxScale);
        return {edgeX + base * (t.x + pivotX * (1 - s)), edgeY + base * (t.y + pivotY * (1 - s)), base * s};
    }

    std::array<float, 2> Pivot(Rect box, int edge, float stageHeight) {
        const float x = edge < 0 ? box.x0 : edge > 0 ? box.x1 : (box.x0 + box.x1) / 2;
        const float y = (box.y0 + box.y1) / 2 < stageHeight / 2 ? box.y0 : box.y1;
        return {x, y};
    }

    float SnapAxis(float lo, float hi, std::span<const float> targets, float grid, float threshold, float* guide) {
        const std::array<float, 3> lines{lo, (lo + hi) / 2, hi};
        float best = threshold, correction = 0;
        std::optional<float> snapped;
        const auto consider = [&](float line, float target) {
            if (const float d = std::abs(target - line); d <= best) {
                best = d;
                correction = target - line;
                snapped = target;
            }
        };
        for (const float line : lines) {
            for (const float target : targets) consider(line, target);
            if (grid > 0) consider(line, std::round(line / grid) * grid);
        }
        if (guide && snapped) *guide = *snapped;
        return correction;
    }

    std::array<float, 2> LockAxis(float dx, float dy) {
        return std::abs(dx) >= std::abs(dy) ? std::array{dx, 0.0F} : std::array{0.0F, dy};
    }

    void History::Push(const Layout& layout) {
        if (states_[at_] == layout) return;
        states_.resize(at_ + 1);
        states_.push_back(layout);
        at_ = states_.size() - 1;
    }

    Layout Read(const cfg::Document& document, Family family, std::string_view preset) {
        Layout layout;
        for (const auto& info : kComponents) {
            const auto table = Table(family, preset, info.key);
            auto& t = layout[info.id];
            const auto finite = [](double v, float fallback) { return std::isfinite(v) ? static_cast<float>(v) : fallback; };
            t.x = finite(document.GetFloat(table, "x", 0), 0);
            t.y = finite(document.GetFloat(table, "y", 0), 0);
            t.scale = std::clamp(finite(document.GetFloat(table, "scale", 1), 1), kMinScale, kMaxScale);
            t.visible = document.GetBool(table, "visible", true);
        }
        return layout;
    }

    void Write(cfg::Document& document, Family family, const Layout& layout, std::string_view preset) {
        for (const auto& info : kComponents) {
            if (!Editable(family, info.id)) continue;
            const auto table = Table(family, preset, info.key);
            const auto& t = layout[info.id];
            SetCommented(document, table, "visible", t.visible, std::string(info.label) + ": false hides it");
            SetCommented(document, table, "x", t.x, "Offset right from the default place, in HUD stage units (1080 to the screen height)");
            SetCommented(document, table, "y", t.y, "Offset down from the default place, in HUD stage units");
            SetCommented(document, table, "scale", t.scale, "Size, 0.5 to 2.0");
        }
    }

    void RemovePreset(cfg::Document& document, std::string_view preset) {
        auto& root = document.Root();
        if (!root.is_table() || !root.as_table().contains("presets")) return;
        auto& presets = root.as_table().at("presets");
        if (presets.is_table()) presets.as_table().erase(std::string(preset));
    }

    std::vector<std::string> PresetNames(const cfg::Document& document) {
        std::vector<std::string> names;
        if (const auto* presets = document.Table("presets"))
            for (const auto& [name, value] : presets->as_table())
                if (value.is_table()) names.push_back(name);
        return names;
    }

    std::string PresetName(std::string_view typed) {
        std::string name;
        for (const char c : typed)
            if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_') name += c;
        const auto first = name.find_first_not_of(' ');
        if (first == std::string::npos) return {};
        return name.substr(first, name.find_last_not_of(' ') - first + 1);
    }

    cfg::Document Load(const std::filesystem::path& file) {
        std::lock_guard lock(FileMutex());
        const auto text = cfg::ReadFile(file);
        return text ? cfg::Document::TryParse(*text, nullptr, "hud_layout.toml") : cfg::Document{};
    }

    bool Update(const std::filesystem::path& file, const std::function<void(cfg::Document&)>& edit) {
        std::lock_guard lock(FileMutex());
        cfg::Document document;
        if (const auto text = cfg::ReadFile(file)) {
            std::string error;
            document = cfg::Document::TryParse(*text, &error, "hud_layout.toml");
            if (!error.empty()) return false;
        }
        edit(document);
        return cfg::WriteFile(file, document.Serialize());
    }
}
