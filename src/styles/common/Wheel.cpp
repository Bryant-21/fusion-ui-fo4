#include "styles/common/Menus.h"
#include "styles/Controls.h"
#include "styles/Frames.h"
#include "core/MenuStyles.h"
#include "core/MenuTimings.h"
#include "fusionui/fo76/Theme.h"
#include <format>

namespace fusionui::styles::menus {
    namespace {
        using namespace hudDraw;
        namespace wheel = screens::wheel;
        namespace rules = core::wheel;
        using modern::theme::Family;

        void Silhouette(const wheel::Slot& slot, float x, float y, float size, ImU32 ink) {
            if (slot.runs.empty()) return Icon(slot.icon, x, y, size, ink);
            int left = INT_MAX, top = INT_MAX, right = INT_MIN, bottom = INT_MIN;
            for (const auto& run : slot.runs) { left = std::min(left, run.x); top = std::min(top, run.y); right = std::max(right, run.x + run.length); bottom = std::max(bottom, run.y + 1); }
            const float unit = size / static_cast<float>(std::max(1, std::max(right - left, bottom - top)));
            for (const auto& run : slot.runs)
                Box(x - static_cast<float>(right - left) * unit / 2 + static_cast<float>(run.x - left) * unit,
                    y - static_cast<float>(bottom - top) * unit / 2 + static_cast<float>(run.y - top) * unit,
                    static_cast<float>(run.length) * unit, unit, Fade(ink, static_cast<float>(run.alpha) / 255));
        }

        void Condition(float x, float y, float width, float value, ImU32 ink) {
            frames::Bar(x, y, width, 3, std::min(value, 1.0F), Fade(ink, value > 1 ? 0.35F : 1.0F));
            if (value > 1) frames::Bar(x, y, width, 3, value - 1, ink);
        }

        void Info(const wheel::Slot& slot, float center, float top, ImU32 ink, float width) {
            if (slot.name.empty()) return;
            const std::string name = slot.name + (slot.count > 1 ? std::format(" ({})", slot.count) : "");
            float y = Wrapped(center, top, 24, width, name, ink, true, kit::Align::Center);
            if (slot.equipped) { Icon("item.equipped", center - 12, y + 10, 16, ink); y += 24; }
            if (!slot.ammo.empty()) { Text(center, y, 16, std::format("{} ({})", slot.ammo, slot.ammoCount), ink, kit::Align::Center); y += 25; }
            if (slot.condition >= 0) { Text(center - 105, y, 13, "CND", ink, kit::Align::Right, false); Condition(center - 90, y + 8, 190, slot.condition, ink); }
        }

        std::vector<fo4::Hint> HintsFor(const wheel::Model& model, bool gamepad) {
            const auto facts = screens::wheel::FactsOf(model);
            const int selected = model.state.selected;
            const bool repair = selected >= 0 && selected < rules::kSlots && model.slots[static_cast<std::size_t>(selected)].condition >= 0;
            std::vector<fo4::Hint> hints;
            for (auto hint : rules::Hints(model.state, facts, gamepad, repair)) {
                switch (hint) {
                case rules::Hint::Tent: hints.push_back({FUSIONUI_PAD_Y, "T", "Survival Tent"}); break;
                case rules::Hint::Scroll: hints.push_back({common::kNoPad, "Mouse wheel", "Scroll"}); break;
                case rules::Hint::Select: hints.push_back({static_cast<std::uint32_t>(model.state.listOpen ? FUSIONUI_PAD_A : FUSIONUI_PAD_RS), model.state.listOpen ? "Space" : "Arrows", "Select"}); break;
                case rules::Hint::Use: if (!model.state.listOpen) hints.push_back({FUSIONUI_PAD_A, "Space", "Use"}); break;
                case rules::Hint::Change: hints.push_back({FUSIONUI_PAD_RB, "C", "Change"}); break;
                case rules::Hint::Inspect: hints.push_back({FUSIONUI_PAD_RS, "X", "Inspect"}); break;
                case rules::Hint::InspectRepair: hints.push_back({FUSIONUI_PAD_RS, "X", "Inspect/Repair"}); break;
                case rules::Hint::Back: hints.push_back({FUSIONUI_PAD_B, "Tab", "Back"}); break;
                }
            }
            return hints;
        }

        // Fallout 4's FavoritesMenu.swf: Menu_mc locked to the bottom-right safe rect at (1216, 684), scale 0.7485;
        // FavoritesCross at (-417, -480) in it, its twelve 57-unit FavoritesEntry clips on a 60-unit cross.
        namespace fo4cross {
            constexpr float kMenuX = 1216, kMenuY = 684, kScale = 0.7485F, kCrossX = -417, kCrossY = -480;
            using core::menus::kFo4CrossCells, core::menus::kFo4CrossEntry;

            ImVec2 ToStage(float x, float y) { return {kMenuX + kScale * x, kMenuY + kScale * y}; }
            ImVec2 CrossPoint(float x, float y) { return ToStage(kCrossX + x, kCrossY + y); }

            // FO4 menu text has no outline or shadow; (x, y) is the glyph box top in the 720-line stage.
            void Label(float x, float y, float size, std::string_view text, ImU32 color, bool bold, kit::Align align, float fit) {
                if (const float width = TextWidth(size, text, bold, 0); fit > 0 && width > fit) size *= fit / width;  // TEXTAUTOSZ_SHRINK
                const float s = Scale();
                kit::Text(x * s, y * s, size * s, text, color, bold, 0, align);
            }

            void Arrows(ImU32 ink) {
                // Shape 121: four arrows round the empty centre cell.
                const auto tri = [&](ImVec2 a, ImVec2 b, ImVec2 c) {
                    const auto p = [](ImVec2 v) { const auto s = CrossPoint(v.x, v.y); return At(s.x, s.y); };
                    kit::Canvas()->AddTriangleFilled(p(a), p(b), p(c), ink);
                };
                const auto stem = [&](float x0, float y0, float x1, float y1) {
                    const auto a = CrossPoint(x0, y0), b = CrossPoint(x1, y1);
                    Box(a.x, a.y, b.x - a.x, b.y - a.y, ink);
                };
                tri({205.55F, 193.55F}, {209.0F, 185.5F}, {212.45F, 193.55F}); stem(208.3F, 193.55F, 209.7F, 198.5F);
                tri({205.55F, 223.05F}, {209.0F, 230.6F}, {212.45F, 223.05F}); stem(208.3F, 218.1F, 209.7F, 223.05F);
                tri({193.75F, 204.6F}, {185.7F, 208.05F}, {193.75F, 211.5F}); stem(193.75F, 207.35F, 198.7F, 208.75F);
                tri({223.25F, 204.6F}, {231.3F, 208.05F}, {223.25F, 211.5F}); stem(218.3F, 207.35F, 223.25F, 208.75F);
            }

            void Draw(const wheel::Model& model, ImU32 ink, float alpha) {
                Anchor(Edge::Right);
                const bool keyboard = common::ActiveDevice() != FUSIONUI_DEVICE_GAMEPAD;
                const auto occupied = wheel::FactsOf(model).occupied;
                const int selected = core::menus::Fo4CrossClamp(occupied, model.state.selected);
                for (int i = 0; i < rules::kSlots; ++i) {
                    if (core::menus::Fo4CrossHidden(occupied, i)) continue;
                    const auto& slot = model.slots[static_cast<std::size_t>(i)];
                    const auto [ex, ey] = kFo4CrossCells[static_cast<std::size_t>(i)];
                    const auto origin = CrossPoint(ex, ey), center = CrossPoint(ex + 28.25F, ey + 27.5F);
                    const float size = kFo4CrossEntry * kScale;
                    // bUseShadedBackground: the engine darkens the entry's bounds.
                    Box(origin.x, origin.y, size, size, Fade(IM_COL32(0, 0, 0, 128), alpha));
                    if (!slot.name.empty()) Silhouette(slot, center.x, center.y, 32 * kScale, ink);  // HotkeyIcons_6 art spans ~31 of 54
                    // Quickkey_tf ($MAIN_Font 20 at (2, 0)) shows for keyboard and mouse only.
                    if (keyboard) Label(origin.x + 4 * kScale, origin.y + 2 * kScale, 20 * kScale, rules::HotkeyLabel(i), ink, false, kit::Align::Left, 0);
                }
                // Selection_mc (shape 123): a 3-unit hollow square over the selected entry.
                if (selected >= 0 && selected < rules::kSlots) {
                    const auto [ex, ey] = kFo4CrossCells[static_cast<std::size_t>(selected)];
                    const auto a = CrossPoint(ex + 0.5F, ey + 0.75F), b = CrossPoint(ex + 56.5F, ey + 56.75F);
                    kit::Canvas()->AddRect(At(a.x, a.y), At(b.x, b.y), ink, 0, 0, Px(3 * kScale));
                }
                Arrows(ink);
                // ItemName_tf ($MAIN_Font_Bold 28, 400 wide) and ItemAmmo_tf ($MAIN_Font 24, 300 wide), centred over the cross.
                if (selected < 0 || selected >= rules::kSlots) return;
                const auto& slot = model.slots[static_cast<std::size_t>(selected)];
                if (slot.name.empty()) return;
                const auto name = slot.count > 1 ? std::format("{} ({})", slot.name, slot.count) : slot.name;
                const auto nameAt = ToStage(-406.5F + 198, -557);
                Label(nameAt.x, nameAt.y, 28 * kScale, name, ink, true, kit::Align::Center, 396 * kScale);
                if (!slot.ammo.empty()) {
                    const auto ammoAt = ToStage(-356.5F + 148, -519.4F);
                    Label(ammoAt.x, ammoAt.y, 24 * kScale, std::format("{} ({})", slot.ammo, slot.ammoCount), ink, false, kit::Align::Center, 296 * kScale);
                }
            }

            // FO4's own bracketed button-hint bar, on this 720-line stage.
            void Hints(std::span<const fo4::Hint> hints) {
                const float s = Scale();
                kit::HintBarStyle style;
                for (float* v : {&style.height, &style.keySize, &style.keyDy, &style.labelSize, &style.labelDy, &style.keyPad, &style.keyGap,
                                 &style.tailPad, &style.spacing, &style.bracketWidth, &style.bracketThickness, &style.bracketGap, &style.glyphRadius})
                    *v *= s;
                style.centerX = kit::CurrentStage().w / 2;
                kit::HintBar(hints, 680 * s, style);
            }
        }
    }

    void DrawWheel(const wheel::Model& model, float alpha) {
        Anchor(Edge::Center);
        const auto family = modern::theme::CurrentAppearance().family;
        const bool skyrim = family == Family::Skyrim, list = wheel::UsesList(family), starfield = family == Family::Starfield;
        const float open = skyrim ? core::menus::MenuFrame(core::menus::SkyrimFavoritesOpen, model.state.openAge) / 256.0F : 1;
        const auto ink = Fade(fo76::theme::Hud(model.palette.ink), alpha * open), accent = Fade(fo76::theme::Hud(model.palette.accent), alpha * open);
        const float dim = model.state.listOpen ? 0.4F : 1;
        const auto slotInk = Fade(ink, dim), slotAccent = Fade(accent, dim);
        const int selected = model.state.selected;
        if (list) {
            const float slide = skyrim ? core::menus::MenuFrame(core::menus::SkyrimFavoritesSlide, model.state.openAge) : 0;
            const float x = (skyrim ? 430.0F : 72.0F) + slide, top = skyrim ? 454.0F : 300.0F, width = skyrim ? 340.0F : 420.0F;
            frames::Panel(x - 15, top - 38, width + 30, skyrim ? 245.0F : 360.0F, slotInk);
            Text(x + width, top - 31, 20, model.tab, slotAccent, kit::Align::Right, true, width);
            const int first = skyrim ? std::max(0, selected) - 3 : 0, visible = skyrim ? 7 : 12;
            float y = top;
            for (int row = 0; row < visible; ++row) {
                const int index = first + row;
                const bool active = index == selected;
                const float height = skyrim ? active ? 35.0F : 28.0F : 26.0F;
                if (index >= 0 && index < rules::kSlots) {
                    const auto& slot = model.slots[static_cast<std::size_t>(index)];
                    const auto color = Fade(active ? slotAccent : slotInk, active ? 1.0F : 0.6F);
                    if (active) {
                        Line(x, y + height - 1, x + width, y + height - 1, color);
                        if (skyrim) frames::Diamond(x + width + 8, y + height / 2, 4, color);
                        else kit::Canvas()->AddRect(At(x, y), At(x + width, y + height), color, 0, 0, Px(1));
                    }
                    Text(x + 7, y + 4, 15, rules::HotkeyLabel(index), color, kit::Align::Left, false);
                    if (slot.equipped) Icon("item.equipped", x + 36, y + height / 2, 13, color);
                    const auto name = slot.name.empty() ? std::string("—") : slot.name + (slot.count > 1 ? std::format(" ({})", slot.count) : "");
                    // The Pip-Boy 3000's item lists are left aligned after the equipped mark; Skyrim's favorites hug the right.
                    if (skyrim) Text(x + width - 9, y + 4, 18, name, color, kit::Align::Right, active, width - 65, 1);
                    else Text(x + 52, y + 4, 19, name, color, kit::Align::Left, active, width - 61);
                }
                y += height;
            }
            if (selected >= 0 && selected < rules::kSlots && !model.state.listOpen) {
                const auto& slot = model.slots[static_cast<std::size_t>(selected)];
                Silhouette(slot, 1000, 405, 90, ink);
                Info(slot, 1000, 475, ink, 360);
            }
        } else if (starfield) {
            for (int i = 0; i < rules::kSlots; ++i) {
                const auto& slot = model.slots[static_cast<std::size_t>(i)];
                const bool active = i == selected;
                const float phase = core::menus::MenuFrame(core::menus::StarfieldFavoritesOpen[static_cast<std::size_t>(i)], model.state.openAge) / 256.0F;
                const auto color = Fade(active ? slotAccent : slotInk, phase * (active ? 1.0F : 0.6F));
                const auto [cx, cy] = core::menus::kStarfieldCrossCenters[static_cast<std::size_t>(i)];
                const float x = 640 + cx / 1.5F, y = 360 + cy / 1.5F;
                Box(x - 26, y - 26, 52, 52, Fade(IM_COL32(16, 27, 39, 220), alpha * dim * phase));
                if (active) { Box(x - 26, y - 26, 2, 52, color); kit::Canvas()->AddRect(At(x - 26, y - 26), At(x + 26, y + 26), color, 0, 0, Px(1)); }
                if (!slot.name.empty()) Silhouette(slot, x, y - 2, 30.0F, color);
                Text(x - 21, y - 25, 11, rules::HotkeyLabel(i), color, kit::Align::Left, false);
                if (slot.count > 1) Text(x + 21, y + 10, 11, std::to_string(slot.count), color, kit::Align::Right, false);
                if (slot.equipped) Icon("item.equipped", x + 18, y - 16, 10, color);
                if (slot.condition >= 0) Condition(x - 21, y + 22, 42, slot.condition, color);
            }
            Text(640, 85.0F, 20, model.tab, slotAccent, kit::Align::Center, true, 330);
            if (selected >= 0 && selected < rules::kSlots) Info(model.slots[static_cast<std::size_t>(selected)], 1040.0F, 290.0F, slotInk, 360.0F);
        } else {
            fo4cross::Draw(model, slotInk, alpha * dim);
            Anchor(Edge::Center);
        }
        if (model.state.listOpen) {
            // Fallout 4's cross sits bottom right, so its quick inventory opens on the left.
            const float x = list ? 850.0F : starfield ? 915.0F : 90.0F, top = 220, width = list ? 360.0F : 330.0F, bottom = 635;
            frames::Panel(x, top - 38, width, bottom - top + 38, ink);
            Text(x + width / 2, top - 29, 20, model.listTitle, accent, kit::Align::Center, true, width - 20);
            const int count = static_cast<int>(model.rows.size()), visible = static_cast<int>((bottom - top) / 30);
            const int first = std::clamp(model.state.listIndex - visible + 1, 0, std::max(0, count - visible));
            if (!count) Text(x + width / 2, top + 12, 18, model.noItems, ink, kit::Align::Center, false, width - 25);
            for (int i = first; i < count && i < first + visible; ++i) {
                const auto& row = model.rows[static_cast<std::size_t>(i)];
                const float y = top + (i - first) * 30;
                const bool active = i == model.state.listIndex;
                if (active) { Box(x + 8, y, width - 16, 28, Fade(accent, 0.15F)); Line(x + 8, y + 28, x + width - 8, y + 28, accent); }
                float left = x + 15;
                if (row.equipped) { Icon("item.equipped", left + 5, y + 13, 12, ink); left += 20; }
                if (row.slot >= 0) { Icon("item.favorite", left + 5, y + 13, 12, ink); left += 20; }
                const auto name = row.text + (row.count > 1 ? std::format(" ({})", row.count) : "");
                Text(left, y + 4, 16, name, active ? accent : ink, kit::Align::Left, active, width - (left - x) - 20);
            }
        }
        const auto hints = HintsFor(model, common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD);
        if (!list && !starfield) fo4cross::Hints(hints);
        else controls::Hints(hints, 672, ink, false);
    }
}
