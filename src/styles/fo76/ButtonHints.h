#pragma once
// FO76's BSButtonHintBar: one row of "KEY) LABEL" hints on a translucent plate, centred under the screen
// (C.A.M.P. build menu, favorites wheel, death screen). Stage units on FO76's 1920 x 1080 stage.
#include "fusionui/fo76/Stage.h"

#include <span>
#include <string>

#include "fusionui/screens/ButtonHints.h"

namespace fusionui::fo76 {
    using fusionui::screens::ButtonHint;
    void ButtonHintBar(std::span<const ButtonHint> hints, float centerX, float plateTop, ImU32 ink);
}
