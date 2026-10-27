#pragma once
// Engine-free HUD rules shared by every overlay client: when a HUD overlay may draw, where a projected
// world point lands on screen, and the toast queue's timing.
#include <array>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace fusionui::core::hud {
    struct MenuState {
        bool uiReady = false;
        bool hudMenuOpen = false;
        bool loading = false;
        bool mainMenu = false;
        bool gameLoaded = false;
        bool menusHidden = false;  // the console's tm (UI::menuSystemVisible off)
        int blockingMenus = 0;  // open menus that hide the HUD (Pip-Boy, container, workshop, pause...)
    };

    // HUD overlays draw on top of every vanilla menu, so they show only when the game's own HUD would.
    bool ShouldDraw(const MenuState& state);

    // NiCamera::WorldPtToScreenPt3 output (0..1, y up, z > 0 in front) -> pixels, or nullopt when the
    // point is behind the camera, not finite, or more than `margin` (fraction of the screen) outside it.
    // The engine itself never rejects off-viewport points.
    std::optional<std::array<float, 2>> NormalizedToScreen(float nx, float ny, float nz, float width, float height,
                                                           float margin = 0.0F);

    // 0..1, empty for a zero or negative maximum.
    float MeterFill(float current, float maximum);

    struct Toast {
        std::string text;
        std::string icon;  // fusionui icon key, may be empty
        float duration = 3.0F;
        float age = 0.0F;
    };

    // Banners shown a few at a time: only the oldest `maxVisible` age; later ones wait their turn. Each
    // fades in and out over `fade` seconds.
    class ToastQueue {
    public:
        explicit ToastQueue(std::size_t maxVisible = 3, float fade = 0.35F);
        void Push(std::string text, std::string icon = {}, float duration = 3.0F);
        void Update(float deltaSeconds);
        void Clear();
        [[nodiscard]] bool Empty() const;

        struct Shown {
            const Toast* toast;
            float alpha;
            std::size_t slot;  // 0 = oldest visible
        };
        [[nodiscard]] std::vector<Shown> Visible() const;

    private:
        std::deque<Toast> toasts_;
        std::size_t maxVisible_;
        float fade_;
    };
}
