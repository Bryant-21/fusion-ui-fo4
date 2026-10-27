#pragma once
// Fallout 76's favorites wheel (radialmenu.swf, RadialMenuRingInner), engine-free: sector geometry, the
// mouse pointer, the entry and icon timelines, and the wheel / quick-inventory state machine Tales'
// converted movie ran. The owner feeds commands and acts on the returned intent.
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace fusionui::core::wheel {
    inline constexpr int kSlots = 12;

    // Twelve 30-degree sectors, slot 2 pointing right and slots running clockwise (screen y down), so slot 0
    // sits at -60 degrees. x / y is any direction from the centre; returns 0..11.
    int Sector(float x, float y);
    // The slot's direction in radians (screen y down).
    float SlotAngle(int slot);
    // FO4's quickkey captions beside the slots: 1..9, 0, -, =.
    const char* HotkeyLabel(int slot);

    // FO76 shows no cursor: mouse deltas steer a bounded direction (radius 240, dead zone 12, the converted
    // wheel's values). A move outside the dead zone selects its sector.
    struct Pointer {
        float x = 0.0F, y = 0.0F;
        // The slot the pointer now points at, or -1 inside the dead zone.
        int Move(float dx, float dy);
    };

    // RadialMenuEntryInner selectorFadeOn (frames 10-15) / selectorFadeOff (16-21), 30 fps: the selection
    // highlight's alpha `age` seconds after the slot was selected / deselected.
    float HighlightAlpha(bool selected, float age);
    // radialCategoryIconContainer "selected" (8-13) / "unselected" (1-6): gold glow strength and colour
    // multiplier of a slot icon.
    struct IconPose {
        float glow = 0.0F;
        float brightness = 1.0F;
    };
    IconPose IconPoseAt(bool selected, float age);

    // One quick-inventory row (FO76 onShowInventory list).
    struct Row {
        std::uint32_t id = 0;  // the owner's row id, handed back by Assign
        std::string text;
        int count = 1;
        int slot = -1;  // quickkey slot it is assigned to, -1 none
        bool equipped = false;
    };
    // Rows as the list shows them: by lower-case text, then id.
    void SortRows(std::vector<Row>& rows);

    struct State {
        int selected = -1;
        bool listOpen = false;
        int listIndex = -1;
        bool listMoved = false;  // the player moved the list since it opened (a refresh keeps the row)
        std::uint32_t listId = 0;
        // Seconds since each slot was last selected / deselected; settled (past every fade) at the start.
        std::array<float, kSlots> ages = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
        float openAge = 0.0F;  // seconds since the wheel opened (ring and centre roll-on)
    };

    // What the owner knows this frame.
    struct Facts {
        std::array<bool, kSlots> occupied{};
        bool change = true;   // the quick inventory can be requested and assigned
        bool inspect = true;  // inspect / repair is available
        bool tent = false;    // the survival tent can be placed
        std::span<const Row> rows;  // sorted (SortRows)
    };

    enum class Command { Accept, Cancel, Close, Next, Previous, Change, Inspect, Tent };
    enum class IntentKind { None, Use, Close, OpenList, CloseList, Assign, Inspect, Tent };
    struct Intent {
        IntentKind kind = IntentKind::None;
        int slot = -1;
        std::uint32_t row = 0;  // Assign: Row::id
    };

    // Selects `slot` (-1 none) unless the list is open; restarts both slots' fades. Returns whether it changed.
    bool Select(State& state, int slot);
    // The converted wheel's Navigate(): what `command` does in the current state.
    Intent Step(State& state, Command command, const Facts& facts);
    // New rows arrived (SetInventory): keep the chosen row, or pick the selected slot's row if untouched.
    void RowsChanged(State& state, const Facts& facts);
    // Advances the slot fades.
    void Advance(State& state, float seconds);

    // FO76 RadialMenu.updateButtonBar for the favorites wheel, in populateButtonBar order.
    enum class Hint { Tent, Scroll, Select, Use, Change, Inspect, InspectRepair, Back };
    std::vector<Hint> Hints(const State& state, const Facts& facts, bool gamepad, bool repairable);
}
