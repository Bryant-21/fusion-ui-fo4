#pragma once
// FO76's keypad menu (keypadmenu.swf) drawn natively. FO76 draws only KeyRects_mc, the 4 x 3 selector frames
// over the 3D keypad it zooms the camera to; with no keypad model on screen the keys, title, code readout and
// hint bar are drawn under FO76's frames at their 16:9 positions. Draw inside fo4::BeginMenu(id, dim,
// kFo76Stage).
#include "fusionui/fo76/Theme.h"
#include "core/Keypad.h"

#include "fusionui/screens/Keypad.h"

namespace fusionui::fo76 {
    using fusionui::screens::KeypadModel;
    using fusionui::screens::KeypadOutcome;
    KeypadOutcome Keypad(KeypadModel& model);
}
