#pragma once
#include "fusionui/Icons.h"
#include "fusionui/fo4/Hud.h"
#include "fusionui/fo76/Filters.h"
#include "fusionui/modern/Theme.h"
#include "core/StyleFonts.h"

#include <cmath>

namespace fusionui::styles::hudDraw {
    namespace kit = fusionui::fo4;
    using kit::Align;
    using kit::hud::Anchor;
    using kit::hud::Edge;

    // Tales shares its 1080-line canvas with overlays; these measurements remain in FO4's 720-line units.
    inline float Scale() { return kit::CurrentStage().h / 720.0F; }
    // Text shadow per style: Starfield's movies use a hard 1-unit (of 1920) black copy at 45 degrees; the rest keep the outline.
    // Soft: the menus' blurred drop shadow (kMenuShadow), so full-screen menus over the live world read over bright scenes.
    // None: the caller draws the shadow itself (Shadowed).
    enum class TextShadow { Outline, Drop, Soft, None };
    inline constexpr fo76::DropShadow kMenuShadow{IM_COL32(0, 0, 0, 255), 45.0F, 2.0F, 4.0F, 2.5F};
    // FNV / FO3 (and Classic HUD Remastered): a tight, soft dark halo a little down and right under every ink, text,
    // art and rules alike. Four taps `spread` around `offset`; they stack to ~0.68 under the stroke and fade at its edge.
    struct ClassicShadowLook { float offset, spread, alpha; };
    inline constexpr ClassicShadowLook kClassicShadow{0.5F, 0.5F, 0.25F};
    inline bool ClassicFamily() {
        const auto family = modern::theme::CurrentAppearance().family;
        return family == modern::theme::Family::Fallout3 || family == modern::theme::Family::NewVegas;
    }
    template <class Tap>
    void ClassicShadowTaps(Tap&& tap) {
        const float o = kClassicShadow.offset, s = kClassicShadow.spread;
        for (const auto [dx, dy] : {std::pair{o - s, o}, {o + s, o}, {o, o - s}, {o, o + s}}) tap(dx, dy);
    }
    // Thread-local, kept in FusionUI.dll so every module drawing text sees the same shadow.
    TextShadow& CurrentTextShadow();
    struct ShadowScope {
        TextShadow saved = CurrentTextShadow();
        explicit ShadowScope(TextShadow shadow) { CurrentTextShadow() = shadow; }
        ~ShadowScope() { CurrentTextShadow() = saved; }
    };
    inline ImVec2 At(float x, float y) { return kit::Stage(x * Scale(), y * Scale()); }
    inline float Px(float value) { return kit::Px(value * Scale()); }
    inline ImU32 Fade(ImU32 color, float alpha) {
        const auto a = static_cast<ImU32>(static_cast<float>(color >> IM_COL32_A_SHIFT) * std::clamp(alpha, 0.0F, 1.0F));
        return (color & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
    }
    inline void Box(float x, float y, float w, float h, ImU32 color) {
        kit::Canvas()->AddRectFilled(At(x, y), At(x + w, y + h), color);
    }
    inline void Line(float x0, float y0, float x1, float y1, ImU32 color, float width = 1) {
        kit::Canvas()->AddLine(At(x0, y0), At(x1, y1), color, Px(width));
    }
    inline ImFont* Font(bool bold = false) {
        if (modern::theme::CurrentAppearance().family == modern::theme::Family::Fallout4) {
            const auto& fonts = common::CurrentFonts();
            auto* font = bold ? fonts.bold : fonts.regular;
            return font ? font : ImGui::GetFont();
        }
        const auto& fonts = modern::theme::CurrentFonts();
        auto* font = bold ? fonts.bold : fonts.body;
        return font ? font : ImGui::GetFont();
    }
    inline float TextWidth(float size, std::string_view text, bool bold = true, float tracking = 0) {
        if (text.empty()) return 0;
        const auto family = modern::theme::CurrentAppearance().family;
        const float scale = Scale();
        int glyphs = 0;
        for (const auto byte : text) if ((static_cast<unsigned char>(byte) & 0xC0) != 0x80) ++glyphs;
        if (family == modern::theme::Family::Fallout4)
            return kit::TextWidth(size * scale, text, bold, tracking * scale) / scale;
        return Font(bold)->CalcTextSizeA(Px(size * core::styles::FontEm(family, bold)), FLT_MAX, 0,
                                       text.data(), text.data() + text.size()).x / Px(1) + tracking * static_cast<float>(glyphs - 1);
    }
    inline void Text(float x, float y, float size, std::string_view text, ImU32 color,
              Align align = Align::Left, bool bold = true, float fit = 0, float tracking = 0) {
        if (text.empty()) return;
        const auto family = modern::theme::CurrentAppearance().family;
        const float scale = Scale();
        const auto widthAt = [&](float pointSize) { return TextWidth(pointSize, text, bold, tracking); };
        const float width = widthAt(size);
        if (fit > 0 && width > fit) {
            const float factor = fit / width;
            size *= factor;
            tracking *= factor;
        }
        if (align == Align::Center) x -= widthAt(size) / 2;
        if (align == Align::Right) x -= widthAt(size);
        const auto paint = [&](float dx, float dy, ImU32 ink) {
            if (family == modern::theme::Family::Fallout4) {
                kit::Text((x + dx) * scale, (y + dy) * scale, size * scale, text, ink, bold, tracking * scale);
                return;
            }
            const auto origin = At(x + dx, y + dy);
            auto* font = Font(bold);
            const float pixels = Px(size * core::styles::FontEm(family, bold));
            if (tracking == 0) {
                kit::Canvas()->AddText(font, pixels, origin, ink, text.data(), text.data() + text.size());
                return;
            }
            int index = 0;
            const auto* end = text.data() + text.size();
            for (const char* at = text.data(); at < end; ++index) {
                const char* next = at + 1;
                while (next < end && (static_cast<unsigned char>(*next) & 0xC0) == 0x80) ++next;
                const float before = at == text.data() ? 0 : font->CalcTextSizeA(pixels, FLT_MAX, 0, text.data(), at).x;
                kit::Canvas()->AddText(font, pixels, {origin.x + before + Px(tracking) * static_cast<float>(index), origin.y}, ink, at, next);
                at = next;
            }
        };
        const float inkAlpha = static_cast<float>(color >> IM_COL32_A_SHIFT) / 255;
        // Black ink (a highlighted row on its plate) gets no black shadow: it only smeared the letters bold.
        const bool blackInk = (color & ~IM_COL32_A_MASK) == 0;
        if (blackInk || CurrentTextShadow() == TextShadow::None) {
            // no shadow
        } else if (CurrentTextShadow() == TextShadow::Outline && ClassicFamily()) {
            ClassicShadowTaps([&](float dx, float dy) { paint(dx, dy, Fade(kit::Black, inkAlpha * kClassicShadow.alpha)); });
        } else if (CurrentTextShadow() == TextShadow::Soft) {
            fo76::ShadowLayers(kMenuShadow, inkAlpha, [&](ImVec2 offset, ImU32 ink) { paint(offset.x / Px(1), offset.y / Px(1), ink); });
        } else if (CurrentTextShadow() == TextShadow::Drop || family == modern::theme::Family::Fallout4) {
            // FO4's HUD text carries a soft drop shadow; the four-way outline read as a blocky FNV face there.
            paint(1 / 1.5F, 1 / 1.5F, Fade(kit::Black, inkAlpha));
        } else {
            const auto outline = Fade(kit::Black, inkAlpha * 0.85F);
            for (const auto [dx, dy] : {std::pair{-0.75F, 0.0F}, {0.75F, 0.0F}, {0.0F, -0.75F}, {0.0F, 0.75F}})
                paint(dx, dy, outline);
        }
        paint(0, 0, color);
    }
    inline void Icon(std::string_view key, float x, float y, float size, ImU32 color) {
        fusionui::icons::DrawGame(kit::Canvas(), key, At(x, y), Px(size), color);
    }
    inline float WrappedHeight(float size, float width, std::string_view text, bool bold = false) {
        if (text.empty()) return 0;
        const auto family = modern::theme::CurrentAppearance().family;
        const float advance = family == modern::theme::Family::Fallout4 ? (bold ? 7.484F / 8.038F : 7.413F / 7.892F) : 1;
        const float pixels = Px(size * core::styles::FontEm(family, bold));
        const float height = Font(bold)->CalcTextSizeA(pixels, FLT_MAX, Px(width) / advance, text.data(), text.data() + text.size()).y;
        return std::round(height / pixels) * (size + 5);
    }
    inline float Wrapped(float x, float y, float size, float width, std::string_view text, ImU32 color, bool bold = false, Align align = Align::Right) {
        auto* font = Font(bold);
        const char* begin = text.data();
        const char* end = begin + text.size();
        while (begin < end) {
            const auto newline = std::find(begin, end, '\n');
            const auto family = modern::theme::CurrentAppearance().family;
            const float advance = family == modern::theme::Family::Fallout4 ? (bold ? 7.484F / 8.038F : 7.413F / 7.892F) : 1;
            const char* next = font->CalcWordWrapPosition(Px(size * core::styles::FontEm(family, bold)), begin, newline, Px(width) / advance);
            if (next == begin && next != newline) {
                ++next;
                while (next < newline && (static_cast<unsigned char>(*next) & 0xC0) == 0x80) ++next;
            }
            Text(x, y, size, {begin, static_cast<std::size_t>(next - begin)}, color, align, bold);
            y += size + 5;
            begin = next;
            while (begin < end && (*begin == ' ' || *begin == '\t' || *begin == '\r')) ++begin;
            if (begin == newline && begin < end) ++begin;
        }
        return y;
    }
    // Draws `draw()` with the classic halo under everything it draws (text included) in FO3 / FNV; elsewhere just
    // draws it. `draw` runs five times there, so it must have no side effects beyond drawing. Nested calls draw once.
    template <class Draw>
    void Shadowed(Draw&& draw) {
        thread_local bool inside = false;
        if (inside || !ClassicFamily()) {
            draw();
            return;
        }
        inside = true;
        const ShadowScope bare(TextShadow::None);
        auto* list = kit::Canvas();
        ClassicShadowTaps([&](float dx, float dy) {
            const int start = list->VtxBuffer.Size;
            draw();
            const ImVec2 offset{Px(dx), Px(dy)};
            for (int i = start; i < list->VtxBuffer.Size; ++i) {
                auto& vertex = list->VtxBuffer[i];
                vertex.pos = {vertex.pos.x + offset.x, vertex.pos.y + offset.y};
                vertex.col = Fade(kit::Black, static_cast<float>(vertex.col >> IM_COL32_A_SHIFT) / 255 * kClassicShadow.alpha);
            }
        });
        draw();
        inside = false;
    }
    inline void Meter(float x, float y, float width, float height, float fill, ImU32 color, bool right = false) {
        Box(x, y, width, height, Fade(color, 0.18F));
        const float amount = width * std::clamp(fill, 0.0F, 1.0F);
        Box(right ? x + width - amount : x, y, amount, height, color);
    }
}
