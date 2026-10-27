#pragma once
// Geometry and stepping of FO76's main menu, pause menu and settings page, engine-free. Units are FO76's
// 1920 x 1080 stage with x measured from the screen's left edge (the lists hug it at any aspect),
// measured from the converted seventysixmenu / menulistcomponent hosts.
#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <vector>

namespace fusionui::core::mainmenu {
    struct Box {
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        bool Contains(float x, float y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
    };

    // Title rows: large (CONTINUE, NEW, LOAD...) then minor (SETTINGS, CREW, QUIT); pause rows all one size.
    struct ListMetrics {
        float textX, top, rowHeight, smallHeight, barTopRight, barBottomRight;
        float smallGap = 0.0F;  // between the last large row and the first minor one
    };
    // Where every style's title and pause rows start (FO76's pause text x), so the lists line up on the left whichever
    // style draws them.
    inline constexpr float kListLeft = 185.0F;
    // FO76 menulistcomponent: the list sits at (326, 294.65); a large row's banner spans y 15.25-80.65 of its 65 px
    // Sizer_mc pitch, its label field starts at x -103.3 (+2 px gutter); a small row's Sizer_mc is 29.6 px, its band
    // 2.6 px lower in its row than a large row's; the banner runs to x 735 at the top and 627 at the bottom.
    inline constexpr ListMetrics kTitle{kListLeft, 310.0F, 65.0F, 29.6F, 735.0F, 627.0F, 2.6F};
    inline constexpr ListMetrics kPause{kListLeft, 315.0F, 47.2F, 47.2F, 570.0F, 485.0F};

    // Row hit boxes, top to bottom; `minor[i]` marks a title minor row.
    inline std::vector<Box> RowBoxes(const ListMetrics& m, const std::vector<bool>& minor) {
        std::vector<Box> boxes;
        float y = m.top;
        bool gap = false;
        for (const bool s : minor) {
            if (s && !gap) {
                y += m.smallGap;
                gap = true;
            }
            const float h = s ? m.smallHeight : m.rowHeight;
            boxes.push_back({0.0F, y, m.barTopRight, y + h});
            y += h;
        }
        return boxes;
    }

    inline std::optional<int> RowAt(std::span<const Box> boxes, float x, float y) {
        for (std::size_t i = 0; i < boxes.size(); ++i)
            if (boxes[i].Contains(x, y)) return static_cast<int>(i);
        return std::nullopt;
    }

    // Settings options panel: 13 rows visible, scrolled to keep the selection in view.
    struct SettingsMetrics {
        float x0 = 613.0F, x1 = 1452.0F, top = 308.0F, rowHeight = 49.0F;
        float labelX = 633.0F, valueX = 1345.0F;
        int visible = 13;
    };
    inline constexpr SettingsMetrics kSettings{};

    inline int ScrollFor(int selected, int scroll, int count, int visible) {
        if (count <= visible) return 0;
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + visible) scroll = selected - visible + 1;
        return std::clamp(scroll, 0, count - visible);
    }

    // Moves `selected` by `dy` (wrapping, as FO4's lists do), skipping disabled rows; unchanged if all are disabled.
    inline int StepSelection(int selected, int dy, const std::vector<bool>& disabled) {
        const int count = static_cast<int>(disabled.size());
        if (count == 0 || dy == 0) return selected;
        int at = std::clamp(selected, 0, count - 1);
        for (int tries = 0; tries < count; ++tries) {
            at = (at + (dy > 0 ? 1 : -1) + count) % count;
            if (!disabled[static_cast<std::size_t>(at)]) return at;
        }
        return selected;
    }

    // A stepper's next value: left / right clamp at the ends (FO4's option steppers do not wrap).
    inline int StepValue(int value, int dx, int options) {
        if (options <= 0) return value;
        return std::clamp(value + dx, 0, options - 1);
    }

    // Scale and offset that cover `frame` with a `width` x `height` image, centred.
    struct Cover {
        float x = 0, y = 0, scale = 0;
    };
    // FO76's loading cog (loadingmenu.swf sprite 35) turns clockwise one revolution per 6 s (2.01 deg a frame
    // at 30 fps): radians clockwise after `seconds` on screen, kept in [0, 2pi).
    inline float VaultCogAngle(float seconds) {
        constexpr float kTurn = 6.2831853F, kPeriod = 6.0F;
        const float t = std::fmod(seconds, kPeriod);
        return (t < 0.0F ? t + kPeriod : t) * (kTurn / kPeriod);
    }

    inline std::optional<Cover> CoverRect(float frameW, float frameH, float width, float height) {
        if (!(frameW > 0 && frameH > 0 && width > 0 && height > 0)) return std::nullopt;
        const float scale = std::max(frameW / width, frameH / height);
        return Cover{(frameW - width * scale) / 2, (frameH - height * scale) / 2, scale};
    }
}
