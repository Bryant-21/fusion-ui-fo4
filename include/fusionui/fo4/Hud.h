#pragma once
// HUD pieces in the Fallout 4 look for overlay clients (ClientOptions::modal = false). Coordinates are
// FO4 stage units like the rest of fusionui/fo4. Engine queries (when to draw, world -> screen) are in
// fusionui/fo4/HudGame.h, which only plugin builds link.
#include "fusionui/fo4/Style.h"
#include "core/Hud.h"
#include "core/HudLayout.h"

#include <filesystem>
#include <string_view>

namespace fusionui::fo4::hud {
    using core::hud::MeterFill;
    using core::hud::ToastQueue;

    // A full-screen layer that never takes input or focus; draw HUD pieces between Begin and End. The
    // stage scales with the display height; Anchor picks which screen edge stage x = 0 / w lines up with
    // (Begin starts centred), so a group laid out for 16:9 keeps hugging its edge at 21:9 or 32:9.
    void Begin(const char* id, StageSize stage = kFo4Stage);
    void End();
    enum class Edge { Left, Center, Right };
    void Anchor(Edge edge);

    // Stage point of a screen pixel (projected world points arrive in pixels).
    ImVec2 ScreenToStage(ImVec2 pixel);

    // A movable HUD component (core/HudLayout.h): from construction to destruction, Anchor places the drawing by
    // the player's layout for the current style family and the drawn box is recorded; a hidden component's
    // vertices are made transparent. Parts nest (the inner one owns its own draws). Outside any part, and for
    // world-projected pieces, the stage is untouched.
    using LayoutId = core::hudlayout::Id;
    class Part {
    public:
        explicit Part(LayoutId id);
        ~Part();
        Part(const Part&) = delete;
        Part& operator=(const Part&) = delete;

    private:
        LayoutId previous_;
    };

    namespace layout {
        core::hudlayout::Family CurrentFamily();
        // hud_layout.toml beside the plugin's assets unless SetFile points elsewhere (before the first draw).
        std::filesystem::path File();
        void SetFile(std::filesystem::path file);
        // The saved layout a HUD layer uses; read from File() on first use.
        core::hudlayout::Layout Saved(core::hudlayout::Family family);
        void SetSaved(core::hudlayout::Family family, const core::hudlayout::Layout& layout);
        // This context draws `layout` (nullptr: the saved one), hidden parts at `hiddenAlpha`: the editor's preview.
        void Use(const core::hudlayout::Layout* layout, float hiddenAlpha = 0.0F);
        // A part's screen box drawn this frame in this context (empty when it was not drawn).
        core::hudlayout::Rect Box(LayoutId id);
    }

    // Text with a thin black outline, readable over any scene.
    void OutlinedText(float x, float y, float size, std::string_view text, ImU32 color, Align align = Align::Left,
                      bool bold = false);

    // Bar with a frame at 40 % and a solid fill of `fill` (0..1). `segments` > 1 splits the fill into
    // blocks like FO4's AP and health meters.
    void Meter(float x, float y, float width, float height, float fill, ImU32 color, int segments = 0);

    // Icon (fusionui/Icons.h key) followed by an upper-case label; returns the row's width.
    float IconLabel(float x, float y, float size, std::string_view iconKey, std::string_view label, ImU32 color);

    // The queue's visible toasts stacked downward from (x, y), one row per toast, aligned to x.
    void Toasts(const ToastQueue& queue, float x, float y, float size, Align align = Align::Left);
}
