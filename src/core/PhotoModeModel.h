#pragma once
// Engine-free rules of FO76's photo mode (selfiemenu.swf: SelfieMenu, Panel, OptionSliderWithLabel,
// OptionStepperWithLabel): five panels of sliders and steppers, row selection that skips inactive rows,
// slider steps, wrapping steppers and RESET. Every value change comes back as a Change for the caller to
// apply, as the movie's BGSCodeObj callbacks did.
#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fusionui::core::photo {
    inline constexpr int kPanelCount = 5;  // Lens, Player, Cinematic, Filters, Overlays
    // Slider_Range_*_mc width and Slider_Bar_*_mc width: one left / right press moves the bar one unit.
    inline constexpr float kSliderTrack = 216.55F, kSliderBar = 6.0F;

    struct Control {
        std::string name;   // the engine's name ("Field of View")
        std::string label;  // display text
        bool slider = false;
        bool sub = false;     // indented (Strength / Distance / Range, Pose, Texture, Frame)
        bool active = true;   // inactive rows draw at 25 % and are skipped by up / down
        float value = 0.0F, min = 0.0F, max = 1.0F, defaultValue = 0.0F, speed = 1.0F;
        std::vector<std::string> options;  // stepper options, upper case
        int selection = 0;

        [[nodiscard]] std::string_view Shown() const {
            return selection >= 0 && selection < static_cast<int>(options.size()) ? std::string_view(options[static_cast<std::size_t>(selection)]) : std::string_view{};
        }
        [[nodiscard]] float Fraction() const { return max > min ? (value - min) / (max - min) : 0.0F; }
        [[nodiscard]] bool Arrows() const { return options.size() > 1; }
    };

    // A slider's new value, or a stepper's new option index.
    struct Change {
        std::string name;
        bool slider = false;
        float value = 0.0F;
        bool operator==(const Change&) const = default;
    };
    using Changes = std::vector<Change>;

    struct Panel {
        std::vector<Control> controls;
        int selected = -1;
    };

    inline std::string Upper(std::string_view text) {
        std::string out(text);
        for (auto& c : out)
            if (static_cast<unsigned char>(c) < 0x80) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return out;
    }

    class Model {
    public:
        std::array<Panel, kPanelCount> panels{};
        int panel = 0;
        bool visible = true;  // TOGGLE MENU hides the panel and hint bar, never the frame / texture

        Control* Find(std::string_view name) {
            for (auto& p : panels)
                for (auto& c : p.controls)
                    if (c.name == name) return &c;
            return nullptr;
        }
        [[nodiscard]] const Control* Find(std::string_view name) const { return const_cast<Model*>(this)->Find(name); }

        Panel& Current() { return panels[static_cast<std::size_t>(panel)]; }
        [[nodiscard]] const Panel& Current() const { return panels[static_cast<std::size_t>(panel)]; }
        Control* Selected() {
            auto& p = Current();
            return p.selected >= 0 && p.selected < static_cast<int>(p.controls.size()) ? &p.controls[static_cast<std::size_t>(p.selected)] : nullptr;
        }

        // AddSlider: the bar starts at `defaultValue`, which the movie reports as a change.
        Change AddSlider(int panelIndex, std::string name, std::string label, float min, float max, float defaultValue, bool sub = false) {
            Control c{.name = std::move(name), .label = std::move(label), .slider = true, .sub = sub, .min = min, .max = max,
                      .defaultValue = defaultValue};
            c.value = std::clamp(defaultValue, min, max);
            Change change{c.name, true, c.value};
            Add(panelIndex, std::move(c));
            return change;
        }
        // AddStepper with no options yet; Fill gives it some.
        void AddStepper(int panelIndex, std::string name, std::string label, bool sub = false) {
            Add(panelIndex, Control{.name = std::move(name), .label = std::move(label), .sub = sub});
        }

        // clearOptions + addOption (upper case, duplicates dropped) + setDefault(options[defaultIndex]), which
        // selects that option and reports it. An empty list leaves the row inactive with `emptyLabel`.
        std::optional<Change> Fill(std::string_view name, const std::vector<std::string>& options, int defaultIndex = 0,
                                   std::string_view emptyLabel = "NONE") {
            auto* c = Find(name);
            if (!c || c->slider) return std::nullopt;
            c->options.clear();
            for (const auto& option : options) {
                auto upper = Upper(option);
                if (std::ranges::find(c->options, upper) == c->options.end()) c->options.push_back(std::move(upper));
            }
            c->active = !c->options.empty();
            if (c->options.empty()) c->options.push_back(Upper(emptyLabel));
            const std::string wanted = defaultIndex >= 0 && defaultIndex < static_cast<int>(options.size())
                                           ? Upper(options[static_cast<std::size_t>(defaultIndex)]) : c->options.front();
            const auto found = std::ranges::find(c->options, wanted);
            if (found == c->options.end()) return std::nullopt;
            return Select(*c, static_cast<int>(found - c->options.begin()));
        }

        // UpdatePairedSliderValue: set a slider from code; a change only when the value moved.
        std::optional<Change> SetSlider(std::string_view name, float value) {
            auto* c = Find(name);
            return c && c->slider ? SetValue(*c, value) : std::nullopt;
        }

        void SetPanel(int index) { panel = std::clamp(index, 0, kPanelCount - 1); }
        // OnLShoulder / OnRShoulder: no wrap. True when the panel changed (the movie plays its navigate sound).
        bool Previous() {
            if (panel == 0) return false;
            SetPanel(panel - 1);
            return true;
        }
        bool Next() {
            if (panel == kPanelCount - 1) return false;
            SetPanel(panel + 1);
            return true;
        }
        void Toggle() { visible = !visible; }

        void Up() {
            auto& p = Current();
            if (p.selected > 0) SelectControl(p, p.selected - 1);
        }
        void Down() {
            auto& p = Current();
            if (p.selected < static_cast<int>(p.controls.size()) - 1) SelectControl(p, p.selected + 1);
        }
        Changes Left() { return Step(-1); }
        Changes Right() { return Step(1); }

        // OnReset: a slider returns to its default, a stepper to its first option.
        Changes Reset() {
            Changes out;
            auto* c = Selected();
            if (!c) return out;
            if (c->slider) {
                if (auto change = SetValue(*c, c->defaultValue)) out.push_back(*change);
            } else {
                out.push_back(Select(*c, 0));
                Paired(*c, out);
            }
            return out;
        }

    private:
        void Add(int panelIndex, Control control) {
            auto& p = panels[static_cast<std::size_t>(std::clamp(panelIndex, 0, kPanelCount - 1))];
            p.controls.push_back(std::move(control));
            if (p.selected < 0) SelectControl(p, 0);
        }

        // Panel.selectControl: moving down searches forward for an active row, otherwise backward; with
        // none found the selection stays.
        static void SelectControl(Panel& p, int target) {
            const int count = static_cast<int>(p.controls.size());
            if (p.selected >= 0 && p.selected < target) {
                for (int i = target; i < count; ++i)
                    if (p.controls[static_cast<std::size_t>(i)].active) { p.selected = i; return; }
            } else {
                for (int i = std::min(target, count - 1); i >= 0; --i)
                    if (p.controls[static_cast<std::size_t>(i)].active) { p.selected = i; return; }
            }
        }

        static std::optional<Change> SetValue(Control& c, float value) {
            const float clamped = std::clamp(value, c.min, c.max);
            if (clamped == c.value) return std::nullopt;
            c.value = clamped;
            return Change{c.name, true, clamped};
        }

        static Change Select(Control& c, int index) {
            c.selection = index;
            return {c.name, false, static_cast<float>(index)};
        }

        // Changing the vanity light style puts its strength back to the default.
        void Paired(const Control& c, Changes& out) {
            if (c.name != "Vanity Light Style") return;
            if (auto* strength = Find("Vanity Light Strength"))
                if (auto change = SetValue(*strength, strength->defaultValue)) out.push_back(*change);
        }

        Changes Step(int direction) {
            Changes out;
            auto* c = Selected();
            if (!c) return out;
            if (c->slider) {
                const float step = (c->max - c->min) / (kSliderTrack - kSliderBar) * c->speed;
                if (auto change = SetValue(*c, c->value + step * static_cast<float>(direction))) out.push_back(*change);
                return out;
            }
            const int count = static_cast<int>(c->options.size());
            if (count < 2) return out;
            out.push_back(Select(*c, (c->selection + direction + count) % count));
            Paired(*c, out);
            return out;
        }
    };
}
