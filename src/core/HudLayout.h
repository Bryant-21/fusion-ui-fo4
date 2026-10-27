#pragma once
// The player's HUD layout: per style family, an offset (stage units), scale and visibility for each HUD component.
// Pure data, maths and TOML; fusionui/fo4/Hud.h applies it at Anchor and fusionhud/HudLayoutEditor.h edits it.
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fusionframework::config {
    class Document;
}

namespace fusionui::core::hudlayout {
    enum class Id : std::uint8_t {
        None, Health, Ap, Needs, FusionCore, Experience, Team, Crosshair, Enemy, Stealth, Crit, Encounter,
        Compass, Quests, Notifications, Hold, Loot, Revive, Messages, Location, Subtitles, Count
    };
    inline constexpr std::size_t kCount = static_cast<std::size_t>(Id::Count);

    enum class Group : std::uint8_t { Status, Combat, Quests, Notifications, Interaction };
    inline constexpr std::array<std::string_view, 5> kGroupLabels{"Status", "Combat", "Quests", "Notifications", "Interaction"};

    struct Info {
        Id id;
        std::string_view key, label;
        Group group;
        bool frame;    // part of FO4's own HUD (frame and reticle), which the FO4 style keeps
        bool minimal;  // shown by the Minimal preset
    };
    // Editor order; keys are the TOML table names and never change.
    inline constexpr std::array<Info, kCount - 1> kComponents{{
        {Id::Health, "health", "Health", Group::Status, true, true},
        {Id::Ap, "ap", "Action points", Group::Status, true, true},
        {Id::Needs, "needs", "Weapon, needs & effects", Group::Status, true, false},
        {Id::FusionCore, "fusion_core", "Fusion core", Group::Status, true, false},
        {Id::Experience, "experience", "Experience", Group::Status, true, false},
        {Id::Location, "location", "Location name", Group::Status, false, false},
        {Id::Team, "team", "Team", Group::Status, false, false},
        {Id::Crosshair, "crosshair", "Crosshair", Group::Combat, true, true},
        {Id::Enemy, "enemy", "Enemy target", Group::Combat, true, false},
        {Id::Stealth, "stealth", "Stealth", Group::Combat, true, false},
        {Id::Crit, "crit", "Critical meter", Group::Combat, true, false},
        {Id::Encounter, "encounter", "Encounter boss", Group::Combat, true, false},
        {Id::Compass, "compass", "Compass", Group::Quests, true, true},
        {Id::Quests, "quests", "Quest tracker", Group::Quests, true, false},
        {Id::Notifications, "notifications", "Notifications", Group::Notifications, false, false},
        {Id::Messages, "messages", "Messages & objectives", Group::Notifications, false, false},
        {Id::Subtitles, "subtitles", "Subtitles", Group::Notifications, false, false},
        {Id::Hold, "hold", "Hold prompt", Group::Interaction, true, false},
        {Id::Loot, "loot", "Quick loot & item info", Group::Interaction, false, false},
        {Id::Revive, "revive", "Revive prompt", Group::Interaction, false, false},
    }};
    const Info& InfoOf(Id id);
    std::optional<Id> FromKey(std::string_view key);

    enum class Family : std::uint8_t { Fo76, Fo4, Pipboy3000, Skyrim, Starfield };
    std::string_view FamilyKey(Family family);
    // FO4 keeps its stock frame, so only Tales' additions move there.
    inline bool Editable(Family family, Id id) { return id != Id::None && !(family == Family::Fo4 && InfoOf(id).frame); }

    inline constexpr float kMinScale = 0.5F, kMaxScale = 2.0F;
    struct Transform {
        float x = 0, y = 0;  // offset from the default place, stage units
        float scale = 1;
        bool visible = true;
        bool operator==(const Transform&) const = default;
    };
    struct Layout {
        std::array<Transform, kCount> parts{};
        Transform& operator[](Id id) { return parts[static_cast<std::size_t>(id)]; }
        const Transform& operator[](Id id) const { return parts[static_cast<std::size_t>(id)]; }
        bool operator==(const Layout&) const = default;
    };
    Layout Minimal();

    struct Rect {
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        bool Empty() const { return x1 <= x0 || y1 <= y0; }
        bool Contains(float x, float y) const { return x >= x0 && x <= x1 && y >= y0 && y <= y1; }
        float Area() const { return (x1 - x0) * (y1 - y0); }
    };
    Rect Union(Rect a, Rect b);

    // Stage point p lands at origin + scale * p. `edgeX` is the anchored edge's origin, `base` the stage scale;
    // the component scales about `pivot` (stage units) and moves by its offset.
    struct Placement {
        float x, y, scale;
    };
    Placement Place(float edgeX, float edgeY, float base, const Transform& t, float pivotX, float pivotY);
    // Scale pivot of a component's unscaled box: its anchored side (edge -1 left, 0 centre, 1 right) and the side
    // nearer the screen edge it hugs vertically, so content growing away from that edge keeps the pivot still.
    std::array<float, 2> Pivot(Rect box, int edge, float stageHeight);

    // Snapping: the correction that brings the nearest of a span's edges or centre onto a target line or grid
    // line, if one is within `threshold`; `guide` receives the line it snapped to.
    float SnapAxis(float lo, float hi, std::span<const float> targets, float grid, float threshold, float* guide = nullptr);
    // Shift-drag keeps only the larger axis of the total drag.
    std::array<float, 2> LockAxis(float dx, float dy);

    class History {
    public:
        void Reset(const Layout& layout) { states_ = {layout}; at_ = 0; }
        void Push(const Layout& layout);
        const Layout* Undo() { return at_ > 0 ? &states_[--at_] : nullptr; }
        const Layout* Redo() { return at_ + 1 < states_.size() ? &states_[++at_] : nullptr; }
        bool CanUndo() const { return at_ > 0; }
        bool CanRedo() const { return at_ + 1 < states_.size(); }

    private:
        std::vector<Layout> states_{Layout{}};
        std::size_t at_ = 0;
    };

    // hud_layout.toml: [<family>.<component>] tables, user presets in [presets.<name>.<family>.<component>].
    namespace cfg = fusionframework::config;
    Layout Read(const cfg::Document& document, Family family, std::string_view preset = {});
    void Write(cfg::Document& document, Family family, const Layout& layout, std::string_view preset = {});
    void RemovePreset(cfg::Document& document, std::string_view preset);
    std::vector<std::string> PresetNames(const cfg::Document& document);
    // Dots split TOML table paths, so preset names keep letters, digits, spaces, '-' and '_'.
    std::string PresetName(std::string_view typed);

    // Missing or unreadable reads as empty; Update refuses to overwrite a file that fails to parse.
    cfg::Document Load(const std::filesystem::path& file);
    bool Update(const std::filesystem::path& file, const std::function<void(cfg::Document&)>& edit);
}
