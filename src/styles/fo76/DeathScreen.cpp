#include "styles/fo76/DeathScreen.h"
#include "styles/fo76/ButtonHints.h"
#include "fusionui/fo76/Theme.h"
#include "styles/fo76/HudDraw.h"

namespace fusionui::fo76::death {
    void Draw(const Model& m, float alpha) {
        using namespace hud::draw;
        fo4::hud::Anchor(fo4::hud::Edge::Center);
        if (core::death::FrameFor(m.seconds) == core::death::Frame::Countdown) {
            // Shape 55: black at 127 / 255 from y 834 down, across the whole screen.
            const ImVec2 size = ImGui::GetIO().DisplaySize;
            fo4::Canvas()->AddRectFilled({0.0F, fo4::Stage(0.0F, 834.0F).y}, size, Fade(IM_COL32(0, 0, 0, 127), alpha));
            // $MAIN_Font_Bold 58 / 89 text fields, hard 45-degree shadows (distance 2, no blur).
            FilteredCaps(kHard, 959.8F, 883.5F, 41.2F, m.title, theme::Hud(m.titleColor), alpha, true, Align::Center);
            FilteredCaps(kHard, 958.9F, 947.4F, 63.3F, core::death::CountdownText(m.seconds), theme::Hud(m.ink), alpha, true, Align::Center);
            return;
        }
        // ButtonHintBar_mc at (960, 1010): the button hint, at alpha 0 in the Countdown frame.
        const ButtonHint hint{"Space)", m.hint, true, FUSIONUI_PAD_A};
        fusionui::fo76::ButtonHintBar({&hint, 1}, 960.0F, 1015.0F, Fade(theme::Hud(m.ink), alpha));
    }
}
