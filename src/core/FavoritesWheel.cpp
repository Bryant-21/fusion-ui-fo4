#include "core/FavoritesWheel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

namespace fusionui::core::wheel {
    namespace {
        constexpr float kPointerRadius = 240.0F, kDeadZone = 12.0F, kFps = 30.0F;

        template <std::size_t N>
        float Frame(const float (&table)[N], float age) {
            const auto frame = static_cast<std::size_t>(std::max(age, 0.0F) * kFps);
            return table[std::min(frame, N - 1)];
        }

        bool Occupied(const Facts& facts, int slot) { return slot >= 0 && slot < kSlots && facts.occupied[static_cast<std::size_t>(slot)]; }

        void PickSlotRow(State& state, const Facts& facts) {
            state.listIndex = facts.rows.empty() ? -1 : 0;
            state.listId = facts.rows.empty() ? 0 : facts.rows.front().id;
            for (std::size_t i = 0; i < facts.rows.size(); ++i)
                if (facts.rows[i].slot == state.selected) {
                    state.listIndex = static_cast<int>(i);
                    state.listId = facts.rows[i].id;
                }
        }

        void MoveList(State& state, const Facts& facts, int delta) {
            if (facts.rows.empty()) return;
            const int next = std::clamp(state.listIndex + delta, 0, static_cast<int>(facts.rows.size()) - 1);
            if (next == state.listIndex) return;
            state.listIndex = next;
            state.listId = facts.rows[static_cast<std::size_t>(next)].id;
            state.listMoved = true;
        }

        std::string Lower(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return out;
        }
    }

    int Sector(float x, float y) {
        const auto sector = static_cast<int>(std::floor(std::atan2(y, x) * 6.0F / std::numbers::pi_v<float> + 2.5F));
        return (sector % kSlots + kSlots) % kSlots;
    }

    float SlotAngle(int slot) { return static_cast<float>(slot - 2) * std::numbers::pi_v<float> / 6.0F; }

    const char* HotkeyLabel(int slot) {
        static constexpr const char* kLabels[kSlots] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
        return slot >= 0 && slot < kSlots ? kLabels[slot] : "";
    }

    int Pointer::Move(float dx, float dy) {
        x += dx;
        y += dy;
        const float length = std::hypot(x, y);
        if (length > kPointerRadius) {
            x *= kPointerRadius / length;
            y *= kPointerRadius / length;
        }
        return length < kDeadZone ? -1 : Sector(x, y);
    }

    float HighlightAlpha(bool selected, float age) {
        static constexpr float kOn[] = {84 / 256.0F, 137 / 256.0F, 185 / 256.0F, 223 / 256.0F, 248 / 256.0F, 1.0F};
        static constexpr float kOff[] = {64 / 256.0F, 44 / 256.0F, 26 / 256.0F, 12 / 256.0F, 3 / 256.0F, 0.0F};
        return selected ? Frame(kOn, age) : Frame(kOff, age);
    }

    IconPose IconPoseAt(bool selected, float age) {
        static constexpr float kOnGlow[] = {0.0F, 0.296875F, 0.578125F, 0.796875F, 0.94921875F, 1.0F};
        static constexpr float kOnBright[] = {230 / 256.0F, 238 / 256.0F, 245 / 256.0F, 251 / 256.0F, 255 / 256.0F, 1.0F};
        static constexpr float kOffGlow[] = {1.0F, 0.6875F, 0.40625F, 0.1875F, 0.0390625F, 0.0F};
        static constexpr float kOffBright[] = {1.0F, 248 / 256.0F, 241 / 256.0F, 235 / 256.0F, 231 / 256.0F, 230 / 256.0F};
        return selected ? IconPose{Frame(kOnGlow, age), Frame(kOnBright, age)} : IconPose{Frame(kOffGlow, age), Frame(kOffBright, age)};
    }

    void SortRows(std::vector<Row>& rows) {
        std::ranges::sort(rows, [](const Row& a, const Row& b) {
            const auto left = Lower(a.text), right = Lower(b.text);
            return left != right ? left < right : a.id < b.id;
        });
    }

    bool Select(State& state, int slot) {
        if (state.listOpen || slot < -1 || slot >= kSlots || slot == state.selected) return false;
        if (state.selected >= 0) state.ages[static_cast<std::size_t>(state.selected)] = 0.0F;
        state.selected = slot;
        if (slot >= 0) state.ages[static_cast<std::size_t>(slot)] = 0.0F;
        return true;
    }

    Intent Step(State& state, Command command, const Facts& facts) {
        if (command == Command::Close) return {IntentKind::Close};
        if (state.listOpen) {
            switch (command) {
            case Command::Accept: {
                if (state.listIndex < 0 || state.listIndex >= static_cast<int>(facts.rows.size()) || state.selected < 0) return {};
                state.listOpen = false;
                return {IntentKind::Assign, state.selected, facts.rows[static_cast<std::size_t>(state.listIndex)].id};
            }
            case Command::Cancel:
            case Command::Change: state.listOpen = false; return {IntentKind::CloseList};
            case Command::Next: MoveList(state, facts, 1); return {};
            case Command::Previous: MoveList(state, facts, -1); return {};
            default: return {};
            }
        }
        switch (command) {
        case Command::Accept: return Occupied(facts, state.selected) ? Intent{IntentKind::Use, state.selected} : Intent{};
        case Command::Cancel: return {IntentKind::Close};
        case Command::Next: Select(state, (state.selected + 1 + kSlots) % kSlots); return {};
        case Command::Previous: Select(state, (state.selected - 1 + kSlots) % kSlots); return {};
        case Command::Change:
            if (state.selected < 0 || !facts.change) return {};
            state.listOpen = true;
            state.listMoved = false;
            PickSlotRow(state, facts);
            return {IntentKind::OpenList, state.selected};
        case Command::Inspect:
            return facts.inspect && Occupied(facts, state.selected) ? Intent{IntentKind::Inspect, state.selected} : Intent{};
        case Command::Tent: return facts.tent ? Intent{IntentKind::Tent} : Intent{};
        default: return {};
        }
    }

    void RowsChanged(State& state, const Facts& facts) {
        if (state.listOpen && !state.listMoved) {
            PickSlotRow(state, facts);
            return;
        }
        state.listIndex = -1;
        for (std::size_t i = 0; i < facts.rows.size(); ++i)
            if (facts.rows[i].id == state.listId) state.listIndex = static_cast<int>(i);
        if (state.listIndex < 0 && !facts.rows.empty()) {
            state.listIndex = 0;
            state.listId = facts.rows.front().id;
        }
    }

    void Advance(State& state, float seconds) {
        for (auto& age : state.ages) age = std::min(age + seconds, 10.0F);
        state.openAge = std::min(state.openAge + seconds, 10.0F);
    }

    std::vector<Hint> Hints(const State& state, const Facts& facts, bool gamepad, bool repairable) {
        std::vector<Hint> hints;
        if (!state.listOpen && facts.tent) hints.push_back(Hint::Tent);
        if (!state.listOpen && !gamepad) hints.push_back(Hint::Scroll);
        hints.push_back(Hint::Select);
        hints.push_back(Hint::Use);
        if (!state.listOpen && facts.change) hints.push_back(Hint::Change);
        if (!state.listOpen && facts.inspect && Occupied(facts, state.selected)) hints.push_back(repairable ? Hint::InspectRepair : Hint::Inspect);
        // FO76 has no visible way back from its list; Tales adds one so a mouse alone can leave it.
        if (state.listOpen) hints.push_back(Hint::Back);
        return hints;
    }
}
