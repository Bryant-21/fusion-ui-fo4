#pragma once
#include "fusionui/screens/PhotoMode.h"
#include "fusionui/fo4/Style.h"

namespace fusionui::styles {
    inline void PhotoOverlays(const screens::PhotoModeView& view) {
        const auto paint = [](const screens::PhotoOverlay& overlay) {
            if (!overlay.texture || overlay.width <= 0 || overlay.height <= 0) return;
            fo4::Canvas()->AddImage(overlay.texture, fo4::Stage(960 - overlay.width / 2, 540 - overlay.height / 2),
                                   fo4::Stage(960 + overlay.width / 2, 540 + overlay.height / 2));
        };
        const bool textureOnTop = view.texture.topmost && !view.frame.topmost;
        paint(textureOnTop ? view.frame : view.texture);
        paint(textureOnTop ? view.texture : view.frame);
    }
}
