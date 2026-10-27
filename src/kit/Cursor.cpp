#include "fusionui/Common.h"
#include "fusionui/Icons.h"
#include "fusionui/modern/GameArt.h"
#include "fusionui/modern/Theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace fusionui::common {
    namespace {
        std::string cursorKey;
        std::string cursorOutlineKey;
    }

    void SetCursorIcon(std::string_view key) {
        cursorKey = key;
        cursorOutlineKey = cursorKey.empty() ? std::string{} : cursorKey + ".outline";
    }

    void DrawGameCursor(ImDrawList* list, ImVec2 tip, ImU32 color, float scale) {
        const auto& appearance = modern::theme::CurrentAppearance();
        const auto choice = modern::art::Pack(appearance.cursor, appearance.family);
        const auto* cursor = modern::art::FindCursor(choice);
        std::string_view key = choice == "simple" ? std::string_view{} : cursor ? cursor->key : cursorKey;
        std::string_view outline = cursor ? cursor->outline : cursorOutlineKey;
        float frame = cursor ? cursor->size : 21.75F;
        if (!cursor)
            for (const auto& candidate : modern::art::Cursors)
                if (candidate.key == key) frame = candidate.size;
        if (!key.empty() && icons::HasArt(key)) {
            const float size = frame * scale;
            const ImVec2 end{tip.x + size * (cursor ? cursor->aspect : 1.0F), tip.y + size};
            const bool hasOutline = !outline.empty() && icons::HasArt(outline);
            if (hasOutline) icons::DrawRect(list, outline, tip, end, IM_COL32(0x18, 0x1C, 0x1D, 0xFF));
            icons::DrawRect(list, key, tip, end, color, 1.0F, !hasOutline);
            return;
        }
        // Notched arrowhead in 1080p pixels: straight left edge, point at the top-left.
        constexpr std::array<ImVec2, 4> kShape{ImVec2{0.0F, 0.0F}, ImVec2{0.0F, 21.0F}, ImVec2{5.5F, 15.5F}, ImVec2{15.0F, 15.5F}};
        std::array<ImVec2, 4> p{};
        for (std::size_t i = 0; i < p.size(); ++i) p[i] = {tip.x + kShape[i].x * scale, tip.y + kShape[i].y * scale};
        const std::array<ImVec2, 4> shadow{ImVec2{p[0].x + scale, p[0].y + scale}, ImVec2{p[1].x + scale, p[1].y + scale},
                                           ImVec2{p[2].x + scale, p[2].y + scale}, ImVec2{p[3].x + scale, p[3].y + scale}};
        // The notch makes the shape concave, so it fills as two triangles.
        list->AddTriangleFilled(shadow[0], shadow[1], shadow[2], IM_COL32(0, 0, 0, 110));
        list->AddTriangleFilled(shadow[0], shadow[2], shadow[3], IM_COL32(0, 0, 0, 110));
        list->AddTriangleFilled(p[0], p[1], p[2], color);
        list->AddTriangleFilled(p[0], p[2], p[3], color);
        list->AddPolyline(p.data(), static_cast<int>(p.size()), IM_COL32(0, 0, 0, 200), ImDrawFlags_Closed, scale);
    }

    bool DrawShapeCursor(ImDrawList* list, ImGuiMouseCursor shape, ImVec2 pos, ImU32 color, float scale) {
        struct Shape {
            ImWchar glyph;
            float angle;
            ImVec2 hotspot;  // fraction of the glyph box
        };
        constexpr ImVec2 kCentre{0.5F, 0.5F};
        constexpr float kDiagonal = 0.78539816F;
        Shape s{};
        switch (shape) {
        case ImGuiMouseCursor_ResizeAll: s = {0xF0B2, 0.0F, kCentre}; break;    // arrows-up-down-left-right
        case ImGuiMouseCursor_ResizeEW: s = {0xF07E, 0.0F, kCentre}; break;     // arrows-left-right
        case ImGuiMouseCursor_ResizeNS: s = {0xF07D, 0.0F, kCentre}; break;     // arrows-up-down
        case ImGuiMouseCursor_ResizeNWSE: s = {0xF07E, kDiagonal, kCentre}; break;
        case ImGuiMouseCursor_ResizeNESW: s = {0xF07E, -kDiagonal, kCentre}; break;
        case ImGuiMouseCursor_TextInput: s = {0xF246, 0.0F, kCentre}; break;    // i-cursor
        case ImGuiMouseCursor_Hand: s = {0xF25A, 0.0F, {0.3F, 0.0F}}; break;    // hand-pointer, index fingertip
        case ImGuiMouseCursor_NotAllowed: s = {0xF05E, 0.0F, kCentre}; break;   // ban
        case ImGuiMouseCursor_Wait:
        case ImGuiMouseCursor_Progress: {
            DrawGameCursor(list, pos, color, scale);
            const ImVec2 centre{pos.x + 19.0F * scale, pos.y + 21.0F * scale};
            const float start = static_cast<float>(ImGui::GetTime()) * 6.0F, radius = 4.5F * scale;
            list->PathArcTo(centre, radius, start, start + 4.7F, 24);
            list->PathStroke(IM_COL32(0x18, 0x1C, 0x1D, 0xFF), ImDrawFlags_None, 3.5F * scale);
            list->PathArcTo(centre, radius, start, start + 4.7F, 24);
            list->PathStroke(color, ImDrawFlags_None, 1.75F * scale);
            return true;
        }
        default: return false;
        }
        auto* font = modern::theme::CurrentFonts().bold;
        if (!font) return false;
        const float size = 16.0F * scale;
        auto* baked = font->GetFontBaked(size);
        const auto* glyph = baked ? baked->FindGlyphNoFallback(s.glyph) : nullptr;
        if (!glyph || !glyph->Visible) return false;
        const ImVec2 origin{pos.x - (glyph->X0 + (glyph->X1 - glyph->X0) * s.hotspot.x),
                            pos.y - (glyph->Y0 + (glyph->Y1 - glyph->Y0) * s.hotspot.y)};
        const int first = list->VtxBuffer.Size;
        const float edge = std::max(1.0F, scale * 0.75F);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                if (dx || dy) font->RenderChar(list, size, {origin.x + dx * edge, origin.y + dy * edge}, IM_COL32(0x18, 0x1C, 0x1D, 0xFF), s.glyph);
        font->RenderChar(list, size, origin, color, s.glyph);
        if (s.angle != 0.0F) {
            const float c = std::cos(s.angle), n = std::sin(s.angle);
            for (int i = first; i < list->VtxBuffer.Size; ++i) {
                auto& v = list->VtxBuffer[i].pos;
                const ImVec2 d{v.x - pos.x, v.y - pos.y};
                v = {pos.x + d.x * c - d.y * n, pos.y + d.x * n + d.y * c};
            }
        }
        return true;
    }
}
