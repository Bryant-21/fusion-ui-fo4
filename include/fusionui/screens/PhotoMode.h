#pragma once
#include <imgui.h>
#include "core/PhotoModeModel.h"

#include <array>
#include <string>

namespace fusionui::screens {
    // A frame or texture image, drawn at its pixel size centred on the 1920 x 1080 stage (ImageResource
    // with FO76's screen position 0, 0). `topmost` textures cover the frame unless the frame is topmost too.
    struct PhotoOverlay {
        ImTextureID texture{};
        float width = 0.0F, height = 0.0F;
        bool topmost = false;
    };

    struct PhotoModeView {
        core::photo::Model model;
        PhotoOverlay frame, texture;
        // TAKE SNAPSHOT, TOGGLE MENU, RESET, EXIT (keys Space / T / R / Tab, pad A / X / Y / B).
        std::array<std::string, 4> hints{"TAKE SNAPSHOT", "TOGGLE MENU", "RESET", "EXIT"};
    };

    // Tab icon keys in panel order.
    inline constexpr std::array<const char*, core::photo::kPanelCount> kPhotoTabIcons{
        "photo.tab.lens", "photo.tab.player", "photo.tab.cinematic", "photo.tab.filters", "photo.tab.overlay"};

    void PhotoMode(const PhotoModeView& view);
}
