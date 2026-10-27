#pragma once
#include "fusionui/fo4/Style.h"

#include <span>
#include <string>

namespace fusionui::screens {
    struct ButtonHint {
        std::string key;    // keyboard caption with its ")": "Space)", "Q)"
        std::string label;  // drawn upper case
        bool enabled = true;
        std::uint32_t padButton = common::kNoPad;  // FUSIONUI_PAD_*: its glyph replaces `key` while a pad is active
    };

    // The bar's plate spans [plateTop, plateTop + 42] around `centerX`; FO76's ButtonHintBar_mc sits 5 above it.
    void ButtonHintBar(std::span<const ButtonHint> hints, float centerX, float plateTop, ImU32 ink);
}
