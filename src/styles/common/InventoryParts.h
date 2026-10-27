#pragma once
// Row pieces shared by the inventory screens and the HUD's quick-loot / item-hover panels.
#include "fusionui/screens/Inventory.h"
#include "styles/Frames.h"
#include "styles/Controls.h"
#include "core/IconRuns.h"

#include <span>
#include <string_view>

namespace fusionui::styles::inventory::parts {
    namespace view = fusionui::screens;
    using namespace hudDraw;
    using namespace controls;
    using modern::theme::Family;

    inline Family FamilyNow() { return modern::theme::CurrentAppearance().family; }
    inline bool Classic() { return FamilyNow() == Family::Fallout3 || FamilyNow() == Family::NewVegas; }

    inline void Condition(float x, float y, float width, float value, ImU32 ink) {
        frames::Bar(x, y, width, 4, std::min(value, 1.0F), Fade(ink, value > 1 ? 0.35F : 1.0F));
        if (value > 1) frames::Bar(x, y, width, 4, value - 1, ink);
    }

    inline void ItemIcon(std::span<const view::hud::IconRun> runs, std::string_view key, float cx, float cy, float size, ImU32 ink) {
        if (!runs.empty()) core::iconruns::DrawRuns(kit::Canvas(), runs, cx, cy, size, ink, [](float x, float y) { return At(x, y); });
        else if (!key.empty()) Icon(key, cx, cy, size, ink);
    }

    inline void Stars(int count, float x, float y, float size, ImU32 ink) {
        for (int i = 0; i < count; ++i) Icon("legendary.star", x + i * (size + 2), y, size, ink);
    }

    inline void Stat(const view::StatRow& stat, float x, float y, float width, ImU32 ink) {
        const float label = stat.labelIcon.empty() ? x : x + 21;
        if (!stat.labelIcon.empty()) Icon(stat.labelIcon, x + 7, y + 9, 15, ink);
        Text(label, y, 14, stat.label, ink, Align::Left, false, width / 2 - (label - x));
        const int compare = std::clamp(stat.compare, -3, 3);
        const float right = x + width - std::abs(compare) * 8;
        Text(right, y, 14, stat.value, ink, Align::Right, true, width / 2 - 22);
        if (!stat.valueIcon.empty()) Icon(stat.valueIcon, right - std::min(TextWidth(14, stat.value), width / 2 - 22) - 12, y + 9, 15, ink);
        for (int i = 0; i < std::abs(compare); ++i) {
            const float cx = right + 5 + i * 8, cy = y + 8, sign = compare > 0 ? 1.0F : -1.0F;
            kit::Canvas()->AddTriangleFilled(At(cx - 3, cy + sign * 3), At(cx + 3, cy + sign * 3), At(cx, cy - sign * 3), ink);
        }
    }

    inline void Selection(Rect box, bool selected, bool focused, ImU32 ink, bool player) {
        // Starfield's InvItem / InvCategory: every row sits on a flat plate (10%), the highlighted one at 40%; no bar, no outline.
        if (FamilyNow() == Family::Starfield) return Box(box.x, box.y, box.width, box.height - 2, Fade(ink, selected ? 0.4F : 0.1F));
        if (!selected) return;
        if (FamilyNow() == Family::Fallout4) {
            Box(box.x, box.y, box.width, box.height, Fade(ink, focused ? 1.0F : 0.45F));
            const float edge = player ? box.x + box.width : box.x, dir = player ? 1.0F : -1.0F;
            kit::Canvas()->AddTriangleFilled(At(edge, box.y), At(edge + dir * 8.5F, box.y + box.height / 2), At(edge, box.y + box.height), ink);
            Line(edge + dir * 12, box.y, edge + dir * 20, box.y + box.height / 2, ink, 3);
            Line(edge + dir * 20, box.y + box.height / 2, edge + dir * 12, box.y + box.height, ink, 3);
        } else if (Classic()) {
            Box(box.x, box.y, box.width, box.height - 1, Fade(ink, 40.0F / 255));
            kit::Canvas()->AddRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height - 1), ink, 0, 0, Px(1));
        } else {
            Box(box.x, box.y, box.width, box.height, IM_COL32(0, 0, 0, 160));
            Line(box.x, box.y + box.height - 1, box.x + box.width, box.y + box.height - 1, Fade(ink, 0.65F));
        }
    }
}
