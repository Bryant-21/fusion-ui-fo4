#include "core/Hud.h"

#include <algorithm>
#include <cmath>

namespace fusionui::core::hud {
    bool ShouldDraw(const MenuState& s) {
        return s.uiReady && s.hudMenuOpen && s.gameLoaded && !s.menusHidden && !s.loading && !s.mainMenu && s.blockingMenus == 0;
    }

    std::optional<std::array<float, 2>> NormalizedToScreen(float nx, float ny, float nz, float width, float height, float margin) {
        if (!std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz) || nz <= 0.0F) return std::nullopt;
        if (nx < -margin || nx > 1.0F + margin || ny < -margin || ny > 1.0F + margin) return std::nullopt;
        return std::array{nx * width, (1.0F - ny) * height};
    }

    float MeterFill(float current, float maximum) {
        if (!(maximum > 0.0F) || !std::isfinite(current)) return 0.0F;
        return std::clamp(current / maximum, 0.0F, 1.0F);
    }

    ToastQueue::ToastQueue(std::size_t maxVisible, float fade) : maxVisible_(std::max<std::size_t>(maxVisible, 1)), fade_(fade) {}

    void ToastQueue::Push(std::string text, std::string icon, float duration) {
        toasts_.push_back({std::move(text), std::move(icon), std::max(duration, 0.0F), 0.0F});
    }

    void ToastQueue::Update(float deltaSeconds) {
        const auto active = std::min(toasts_.size(), maxVisible_);
        for (std::size_t i = 0; i < active; ++i) toasts_[i].age += std::max(deltaSeconds, 0.0F);
        std::erase_if(toasts_, [](const Toast& t) { return t.age >= t.duration; });
    }

    void ToastQueue::Clear() { toasts_.clear(); }

    bool ToastQueue::Empty() const { return toasts_.empty(); }

    std::vector<ToastQueue::Shown> ToastQueue::Visible() const {
        std::vector<Shown> shown;
        const auto active = std::min(toasts_.size(), maxVisible_);
        for (std::size_t i = 0; i < active; ++i) {
            const auto& t = toasts_[i];
            float alpha = 1.0F;
            if (fade_ > 0.0F) alpha = std::clamp(std::min(t.age, t.duration - t.age) / fade_, 0.0F, 1.0F);
            shown.push_back({&t, alpha, i});
        }
        return shown;
    }
}
