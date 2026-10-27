#pragma once
// FO76's photo mode (selfiemenu.swf) drawn natively: the five-tab panel bottom right (Lens, Player,
// Cinematic, Filters, Overlays) with its slider and stepper rows, the Z / C bumper hints, the
// BSButtonHintBar along the bottom, and the full-stage frame and texture overlays under them. Navigation
// lives in core::photo::Model; this only draws it. Draw inside fo4::BeginMenu(id, 0, kFo76Stage).
#include "fusionui/fo76/Stage.h"
#include "core/PhotoModeModel.h"

#include <array>
#include <string>

#include "fusionui/screens/PhotoMode.h"

namespace fusionui::fo76 {
    using fusionui::screens::PhotoOverlay;
    using fusionui::screens::PhotoModeView;
    using fusionui::screens::kPhotoTabIcons;
    void PhotoMode(const PhotoModeView& view);
}
