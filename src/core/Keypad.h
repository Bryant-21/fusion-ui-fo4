#pragma once
// Engine-free rules of the keypad (FO76 keypadmenu's 4 x 3 key grid: KeypadInfoData row / column), shared by
// the fo76 widget, Tales and the tests. Keys run 1 2 3 / 4 5 6 / 7 8 9 / DEL 0 ENTER.
#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace fusionui::core::keypad {
    inline constexpr int kRows = 4, kColumns = 3, kKeys = kRows * kColumns;
    inline constexpr int kDeleteKey = 9, kSubmitKey = 11;
    inline constexpr std::array<std::string_view, kKeys> kLabels{"1", "2", "3", "4", "5", "6", "7", "8", "9", "DEL", "0", "ENTER"};

    enum class Key { Up, Down, Left, Right, Select, Submit, Delete, Clear, Cancel };

    struct Outcome {
        enum class Kind { None, Submit, Cancel } kind = Kind::None;
        std::string code;  // the entered digits on Submit
    };

    struct Model {
        std::string title = "KEYPAD";
        int digits = 4;  // 1..8
        std::string entered;
        int selected = 0;      // 0..11, row-major
        bool pending = false;  // a submitted code waits for the caller; only Cancel still works

        void Configure(int count, std::string_view name) {
            digits = std::clamp(count, 1, 8);
            title = name.empty() ? "KEYPAD" : std::string(name);
            entered.clear();
            pending = false;
        }

        Outcome Press(Key key) {
            if (key == Key::Cancel) return {Outcome::Kind::Cancel, {}};
            if (pending) return {};
            switch (key) {
            case Key::Up: selected = (selected + kKeys - kColumns) % kKeys; break;
            case Key::Down: selected = (selected + kColumns) % kKeys; break;
            case Key::Left: selected = selected / kColumns * kColumns + (selected + kColumns - 1) % kColumns; break;
            case Key::Right: selected = selected / kColumns * kColumns + (selected + 1) % kColumns; break;
            case Key::Select:
                if (selected == kDeleteKey) return Press(Key::Delete);
                if (selected == kSubmitKey) return Press(Key::Submit);
                return Digit(kLabels[static_cast<std::size_t>(selected)][0] - '0');
            case Key::Delete:
                if (!entered.empty()) entered.pop_back();
                break;
            case Key::Clear: entered.clear(); break;
            case Key::Submit:
                if (static_cast<int>(entered.size()) != digits) break;
                pending = true;
                return {Outcome::Kind::Submit, entered};
            case Key::Cancel: break;
            }
            return {};
        }

        Outcome Digit(int digit) {
            if (!pending && digit >= 0 && digit <= 9 && static_cast<int>(entered.size()) < digits)
                entered.push_back(static_cast<char>('0' + digit));
            return {};
        }

        // "1 2 _ _": the entered digits, then an underscore per digit still to enter.
        std::string Readout() const {
            std::string out;
            for (int i = 0; i < digits; ++i) {
                if (i) out.push_back(' ');
                out.push_back(i < static_cast<int>(entered.size()) ? entered[static_cast<std::size_t>(i)] : '_');
            }
            return out;
        }
    };
}
