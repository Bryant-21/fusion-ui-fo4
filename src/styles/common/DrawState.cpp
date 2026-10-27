// Drawing state the inline helpers in styles/HudDraw.h and styles/fo76/HudDraw.h share across modules.
#include "styles/HudDraw.h"
#include "styles/fo76/HudDraw.h"

namespace fusionui::styles::hudDraw {
    TextShadow& CurrentTextShadow() {
        thread_local TextShadow shadow = TextShadow::Outline;
        return shadow;
    }
}

namespace fusionui::fo76::hud::draw {
    ImVec2& GroupShift() {
        thread_local ImVec2 shift{};
        return shift;
    }
}
