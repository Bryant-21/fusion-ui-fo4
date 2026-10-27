#pragma once
#include "core/Keypad.h"

namespace fusionui::screens {
    using KeypadModel = core::keypad::Model;
    using KeypadOutcome = core::keypad::Outcome;

    // Applies this frame's keyboard, pad and mouse input to `model`, then draws it. Keys: digits (row and
    // number pad), arrows / D-pad move, Space / A press the selected key, Enter / Y submit, Backspace / X
    // delete, Delete clears, Esc / Tab / B cancel; hovering a key selects it and a click presses it.
    KeypadOutcome Keypad(KeypadModel& model);
}
