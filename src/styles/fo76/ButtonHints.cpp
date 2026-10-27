#include "styles/fo76/ButtonHints.h"
#include "styles/fo76/HudDraw.h"

namespace fusionui::fo76 {
    void ButtonHintBar(std::span<const ButtonHint> hints, float centerX, float plateTop, ImU32 ink) {
        using namespace hud::draw;
        if (hints.empty()) return;
        constexpr float kKeyGap = 7, kHintGap = 28, kPad = 8, kKeyCap = 17, kLabelCap = 20, kGlyphRadius = 13.5F;
        const bool pad = common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD;
        const auto keyWidth = [&](const ButtonHint& h) {
            return pad && h.padButton != common::kNoPad ? common::PadGlyphWidth(fo4::Px(kGlyphRadius), h.padButton) / fo4::Px(1.0F)
                                                        : CapsWidth(kKeyCap, h.key);
        };
        float width = 2 * kPad - kHintGap;
        for (const auto& h : hints) width += keyWidth(h) + kKeyGap + CapsWidth(kLabelCap, Upper(h.label), true) + kHintGap;
        float x = centerX - width / 2;
        Box(x, plateTop, x + width - 1, plateTop + 41, IM_COL32(0, 0, 0, 102));
        x += kPad;
        for (const auto& h : hints) {
            const auto color = h.enabled ? ink : Fade(ink, 0.5F);
            const float keyW = keyWidth(h);
            if (pad && h.padButton != common::kNoPad)
                common::PadGlyph(fo4::Canvas(), fo4::Stage(x + keyW / 2, plateTop + 21), fo4::Px(kGlyphRadius), h.padButton, color);
            else Caps(x, plateTop + 14, kKeyCap, h.key, color);
            x += keyW + kKeyGap;
            const auto label = Upper(h.label);
            Caps(x, plateTop + 9, kLabelCap, label, color, true);
            x += CapsWidth(kLabelCap, label, true) + kHintGap;
        }
    }
}
