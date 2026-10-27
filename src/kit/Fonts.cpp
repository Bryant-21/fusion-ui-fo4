#include "fusionui/fo76/Style.h"
#include "kit/StyleInternal.h"

#include <algorithm>

namespace fusionui::fo76 {
    namespace {
        struct State {
            float textScale = 1.0F;
            ImU32 accent = 0;
            ImU32 appliedHud = 0;  // the HUD colour ApplyColors last saw through theme::Hud
        };
        State& Current() { return common::ContextSlot<State>(); }
    }

    Fonts InitContext(const std::filesystem::path& fontDir, float scale) {
        const auto fonts = common::InitBaseContext(fontDir);
        detail::ApplyStyle();
        SetTextScale(scale);
        common::SetStyleRebuild([](float uiScale) {
            detail::ApplyStyle();
            auto& style = ImGui::GetStyle();
            style.ScaleAllSizes(uiScale);
            style.FontSizeBase = theme::ButtonSize * TextScale();
        });
        return fonts;
    }

    void detail::RefreshHudColors() {
        auto& state = Current();
        const ImU32 hud = common::HudColor();
        if (state.appliedHud == hud) return;
        state.appliedHud = hud;
        detail::ApplyColors();
    }

    float TextScale() { return Current().textScale * common::UiScale(); }

    ImU32 Accent() { return Current().accent ? Current().accent : common::HudColor(); }

    ImU32 Accent(float brightness, int alpha) {
        const ImU32 c = Accent();
        const auto channel = [&](int shift) {
            return static_cast<ImU32>(std::clamp(static_cast<float>((c >> shift) & 0xFF) * brightness, 0.0F, 255.0F)) << shift;
        };
        return channel(IM_COL32_R_SHIFT) | channel(IM_COL32_G_SHIFT) | channel(IM_COL32_B_SHIFT) |
               static_cast<ImU32>(std::clamp(alpha, 0, 255)) << IM_COL32_A_SHIFT;
    }

    void SetAccent(ImU32 color) {
        color |= IM_COL32_A_MASK;
        if (color == Current().accent) return;
        Current().accent = color;
        detail::ApplyColors();
    }

    void SetTextScale(float scale) {
        Current().textScale = scale;
        ImGui::GetStyle().FontSizeBase = theme::ButtonSize * TextScale();
    }
}
