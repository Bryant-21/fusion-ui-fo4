#include "styles/common/Inventory.h"
#include "styles/common/InventoryParts.h"
#include "styles/Frames.h"
#include "styles/Controls.h"
#include "core/IconRuns.h"
#include "core/InventoryStyles.h"

#include <format>
#include <array>

namespace fusionui::styles::inventory {
    namespace {
        using namespace hudDraw;
        using modern::theme::Family;
        namespace rows = core::inventory::styles;
        namespace rules = core::inventory;

        using namespace parts;
        float Width() { return kit::CurrentStage().w / Scale(); }
        ImU32 Ink(ImU32 tint = 0) { return tint ? tint : common::HudColor(); }

        using namespace controls;

        float Scroll(const char* name, Rect box, float content, ImU32 ink, bool interactive, bool keys = false) {
            auto* storage = ImGui::GetStateStorage();
            const auto id = ImGui::GetID(name);
            float offset = storage->GetFloat(id);
            if (interactive) {
                if (Hover(box)) offset -= ImGui::GetIO().MouseWheel * 28;
                if (keys) {
                    if (ImGui::IsKeyPressed(ImGuiKey_PageDown) || ImGui::IsKeyPressed(ImGuiKey_GamepadRStickDown)) offset += 56;
                    if (ImGui::IsKeyPressed(ImGuiKey_PageUp) || ImGui::IsKeyPressed(ImGuiKey_GamepadRStickUp)) offset -= 56;
                }
            }
            offset = std::clamp(offset, 0.0F, std::max(0.0F, content - box.height));
            storage->SetFloat(id, offset);
            if (content > box.height) {
                const float thumb = box.height * box.height / content;
                Box(box.x + box.width - 3, box.y + (box.height - thumb) * offset / (content - box.height), 2, thumb, ink);
            }
            return offset;
        }

        std::string RowName(const view::Item& item, float size, float width, bool bold) {
            const auto suffix = item.count == 1 ? std::string{} : std::format(" ({})", item.count);
            if (TextWidth(size, item.name + suffix, bold) <= width) return item.name + suffix;
            std::size_t low = 0, high = rules::Utf8Length(item.name);
            while (low < high) {
                const auto middle = (low + high + 1) / 2;
                if (TextWidth(size, item.name.substr(0, rules::Utf8Prefix(item.name, middle)) + "…" + suffix, bold) <= width) low = middle;
                else high = middle - 1;
            }
            return item.name.substr(0, rules::Utf8Prefix(item.name, low)) + "…" + suffix;
        }

        float CardWidth(float center) {
            const float nominal = FamilyNow() == Family::Skyrim ? 410.0F : Classic() ? 350.0F : 340.0F;
            return std::min(nominal, std::max(80.0F, 2 * std::min(center, Width() - center) - 16));
        }

        bool HasStat(const view::StatRow& stat) {
            return !stat.label.empty() || !stat.value.empty() || !stat.labelIcon.empty() || !stat.valueIcon.empty() || stat.compare != 0;
        }

        enum class CardMode { Details, Stats, Interactive };
        float DrawCard(const view::Item& item, float center, float bottom, ImU32 ink, float maximum = 450, bool interactive = true, float widthLimit = FLT_MAX, CardMode mode = CardMode::Details) {
            if (item.disabled) ink = Fade(ink, 0.5F);
            const float width = std::min(CardWidth(center), widthLimit), left = center - width / 2, inner = width - 24;
            const std::string title = mode == CardMode::Stats ? "ITEM STATS" : item.name + (item.count == 1 ? "" : std::format(" ({})", item.count));
            const rules::RowFlags flags{item.equipped, item.favorite, item.tagged, item.quest, item.setItem, item.setActive, item.locked};
            const auto lead = rules::LeadIcon(flags);
            const auto trailing = rules::TrailingIcons(flags);
            const bool badges = item.legendaryStars > 0 || !lead.empty() || !trailing.empty();
            float full = 24 + WrappedHeight(20, inner - (item.iconKey.empty() ? 0 : 27), title, true) + WrappedHeight(12, inner, item.typeLine) + WrappedHeight(14, inner, item.description);
            if (badges) full += 22;
            if (item.condition >= 0) full += 23;
            for (const auto& stat : item.stats) if (HasStat(stat)) full += 23;
            if (item.weight >= 0) full += 23;
            if (item.value >= 0) full += 23;
            const float height = std::min(full, std::min(maximum, std::max(70.0F, bottom - 90)));
            const float top = bottom - height;
            struct Scroll { ImGuiID item{}; float offset{}; };
            auto& scroll = common::ContextSlot<Scroll>();
            const auto id = ImGui::GetID(item.name.c_str());
            if (id != scroll.item) { scroll.item = id; scroll.offset = 0; }
            if (interactive && Hover({left, top, width, height})) scroll.offset -= ImGui::GetIO().MouseWheel * 28;
            if (interactive && mode == CardMode::Interactive) {
                if (ImGui::IsKeyPressed(ImGuiKey_Home)) scroll.offset = 0;
                if (ImGui::IsKeyPressed(ImGuiKey_End)) scroll.offset = full;
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadRStickDown)) scroll.offset += 28;
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadRStickUp)) scroll.offset -= 28;
            }
            scroll.offset = std::clamp(scroll.offset, 0.0F, std::max(0.0F, full - height));
            frames::Panel(left, top, width, height, ink);
            kit::Canvas()->PushClipRect(At(left + 6, top + 5), At(left + width - 6, bottom - 5), true);
            float y = top + 12 - scroll.offset;
            if (!item.iconKey.empty()) Icon(item.iconKey, left + width - 18, y + 10, 20, ink);
            y = Wrapped(left + 12, y, 20, inner - (item.iconKey.empty() ? 0 : 27), title, ink, true, Align::Left);
            if (badges) {
                float x = left + 20;
                if (!lead.empty()) { Icon(lead, x, y + 8, 14, ink); x += 20; }
                for (const auto key : trailing) { Icon(key, x, y + 8, 14, ink); x += 20; }
                Stars(item.legendaryStars, x, y + 8, 14, ink);
                y += 22;
            }
            y = Wrapped(left + 12, y, 12, inner, item.typeLine, Fade(ink, 0.7F), false, Align::Left);
            y = Wrapped(left + 12, y, 14, inner, item.description, ink, false, Align::Left);
            if (item.condition >= 0) {
                Text(left + 12, y, 14, "CND", ink, Align::Left, false);
                Condition(left + 75, y + 8, inner - 63, item.condition, ink);
                y += 23;
            }
            for (const auto& stat : item.stats) if (HasStat(stat)) { Stat(stat, left + 12, y, inner, ink); y += 23; }
            if (item.weight >= 0) { Stat({"Weight", rules::FormatNumber(item.weight)}, left + 12, y, inner, ink); y += 23; }
            if (item.value >= 0) Stat({"Value", std::to_string(item.value), {}, item.currency}, left + 12, y, inner, ink);
            kit::Canvas()->PopClipRect();
            if (full > height) {
                const float thumb = (height - 16) * height / full;
                Box(left + width - 4, top + 8 + (height - 16 - thumb) * scroll.offset / (full - height), 2, thumb, ink);
            }
            return top;
        }

        // Stars and a bonus icon lead the wrapped line.
        float Lead(const view::Effect& effect) { return static_cast<float>(std::max(0, effect.stars) * 16 + (effect.icon.empty() ? 0 : 16)); }

        float EffectsHeight(std::span<const view::Effect> effects, float width) {
            float height = 0;
            for (const auto& effect : effects) height += WrappedHeight(14, width - Lead(effect), effect.text) + 4;
            return height;
        }

        void Effects(std::span<const view::Effect> effects, float center, float bottom, ImU32 ink, float widthLimit = FLT_MAX, float maximum = FLT_MAX, bool interactive = true) {
            const float width = std::min(CardWidth(center), widthLimit), x = center - width / 2;
            const float full = EffectsHeight(effects, width), height = std::min(full, maximum);
            const Rect box{x, bottom - height, width, height};
            float y = box.y - Scroll("##inventory-effects", box, full, ink, interactive);
            kit::Canvas()->PushClipRect(At(x, box.y), At(x + width, bottom), true);
            for (const auto& effect : effects) {
                const int stars = std::max(0, effect.stars);
                const ImU32 lineInk = effect.inactive ? (ink & ~IM_COL32_A_MASK) | ((ink >> IM_COL32_A_SHIFT) / 2 << IM_COL32_A_SHIFT) : ink;
                Stars(stars, x + 7, y + 8, 12, lineInk);
                if (!effect.icon.empty()) Icon(effect.icon, x + stars * 16 + 7, y + 8, 12, lineInk);
                y = Wrapped(x + Lead(effect), y, 14, width - Lead(effect), effect.text, lineInk, false, Align::Left) + 4;
            }
            kit::Canvas()->PopClipRect();
        }

        struct PanelPlace { Rect frame, list; float title; };
        PanelPlace Place(view::Side side) {
            const bool player = side == view::Side::Player;
            if (FamilyNow() == Family::Fallout4) {
                const float x = player ? 185.35F : Width() - 505.85F;
                return {{x, 195.5F, 321.15F, 326}, {x, player ? 223.0F : 223.5F, 321.15F, player ? 26.1F * 9 : 26.2F * 10}, 171};
            }
            if (Classic()) {
                const float x = player ? 50.0F : Width() - 443;
                return {{x - 8, 145, 409, 430}, {x, 160, 393, 400}, 115};
            }
            if (FamilyNow() == Family::Skyrim) return {{230, 100, 250, 500}, {235, 128, 240, 455}, 76};
            return {{52.0F / 1.5F, 128.0F / 1.5F, 426.5F / 1.5F, 500},
                    {52.0F / 1.5F, 128.0F / 1.5F, 426.5F / 1.5F, 500}, 95.0F / 1.5F};
        }

        // One list row inside `target`; the name's top at `y`. `subtitle` is a smaller line under the name. `badges`: Starfield's
        // wrapped name over a badge line, which needs its own tall rows.
        void ItemRow(Rect target, const view::Item& item, bool selected, ImU32 ink, bool prices, rows::Metrics metrics, float y,
                     std::string_view subtitle = {}, float cellWidth = 52, bool badges = true) {
            auto color = FamilyNow() == Family::Fallout4 && selected ? IM_COL32_BLACK : ink;
            if (item.disabled) color = Fade(color, 0.35F);
            float x = target.x + 12, right = target.x + target.width - 10;
            const rules::RowFlags flags{item.equipped, item.favorite, item.tagged, item.quest, item.setItem, item.setActive, item.locked};
            if (const auto lead = rules::LeadIcon(flags); !lead.empty()) { Icon(lead, x + 5, target.y + target.height / 2, 13, color); x += 20; }
            if (!item.iconRuns.empty() || !item.iconKey.empty()) {
                const float size = std::min(metrics.font + 2, target.height - 4);
                ItemIcon(item.iconRuns, item.iconKey, x + size / 2, target.y + target.height / 2, size, color);
                x += size + 6;
            }
            if (prices && item.value >= 0) {
                Text(right, y, metrics.font - 2, std::to_string(item.value), color, Align::Right, false, 62);
                Icon(item.currency, right - 72, target.y + target.height / 2, 13, color);
                right -= 88;
            }
            // shortcut: Tales' columns without their headings / search strip here (the fo76 style draws those);
            // add a heading row when another family needs sorting by click.
            const float cellY = subtitle.empty() ? y : target.y + (target.height - metrics.font) / 2;
            for (auto cell = item.cells.rbegin(); cell != item.cells.rend(); ++cell) {
                Text(right, cellY, metrics.font - 2, *cell, color, Align::Right, false, cellWidth - 4);
                right -= cellWidth;
            }
            if (badges && FamilyNow() == Family::Starfield) {
                const float available = std::max(15.0F, right - x - 4), badgeY = target.y + target.height - 9;
                kit::Canvas()->PushClipRect(At(x, target.y + 4), At(x + available, target.y + target.height - 17), true);
                Wrapped(x, target.y + 4, metrics.font, available, item.name, color, true, Align::Left);
                kit::Canvas()->PopClipRect();
                float badgeX = x + 6;
                for (const auto key : rules::TrailingIcons(flags)) { Icon(key, badgeX, badgeY, 12, color); badgeX += 18; }
                Stars(std::max(0, item.legendaryStars), badgeX, badgeY, 11, color);
                badgeX += std::max(0, item.legendaryStars) * 13.0F;
                if (item.count != 1) Text(badgeX + 8, badgeY - 7, 11, std::format("({})", item.count), color, Align::Left, false, std::max(15.0F, right - badgeX - 8));
                if (item.rowMeter >= 0) {
                    frames::Bar(target.x + 10, target.y + target.height - 1, target.width - 20, 1, std::min(item.rowMeter, 1.0F), Fade(color, item.rowMeter > 1 ? 0.35F : 1.0F));
                    if (item.rowMeter > 1) frames::Bar(target.x + 10, target.y + target.height - 1, target.width - 20, 1, item.rowMeter - 1, color);
                }
                return;
            }
            for (const auto key : rules::TrailingIcons(flags)) { Icon(key, right - 6, target.y + target.height / 2, 13, color); right -= 18; }
            const int stars = std::max(0, item.legendaryStars);
            const float starWidth = stars * 13.0F + (item.unique ? 16.0F : 0.0F);
            const std::string name = item.name + (item.count == 1 ? "" : std::format(" ({})", item.count));
            const float available = std::max(15.0F, right - x - starWidth - 4);
            kit::Canvas()->PushClipRect(At(x, target.y), At(x + available, target.y + target.height), true);
            // FO4 only turns the highlighted row's text black; the other families bold it.
            const bool bold = selected && FamilyNow() != Family::Fallout4;
            Text(x, y, metrics.font, RowName(item, metrics.font, available, bold), color, Align::Left, bold);
            kit::Canvas()->PopClipRect();
            const float starsX = x + std::min(TextWidth(metrics.font, name, bold), available) + 10;
            Stars(stars, starsX, target.y + target.height / 2, 11, color);
            if (item.unique) Icon("item.unique", starsX + stars * 13.0F + (stars ? 2 : 0), target.y + target.height / 2, 12, color);
            if (!subtitle.empty()) {
                const float size = std::round(metrics.font * 0.62F);
                kit::Canvas()->PushClipRect(At(x, target.y), At(right, target.y + target.height), true);
                Text(x + 2, y + metrics.font, size, subtitle, Fade(color, 0.85F), Align::Left, false);
                kit::Canvas()->PopClipRect();
            }
            if (item.rowMeter >= 0) Condition(target.x + 10, target.y + target.height - 5, target.width - 20, item.rowMeter, color);
        }

        view::ListResult List(Rect box, std::span<const view::Item> items, view::ListModel& model, const view::Nav& nav,
                              bool focused, bool interactive, bool prices, ImU32 ink, bool player = true) {
            auto metrics = rows::Rows(FamilyNow());
            if (FamilyNow() == Family::Fallout4 && player) metrics.pitch = metrics.selected = 26.1F;
            model.SetVisibleRows(rows::VisibleRows(metrics, box.height));
            model.SetCount(static_cast<int>(items.size()));
            const int before = model.Selected();
            view::ListResult result;
            if (focused && interactive) {
                if (nav.dy) model.Move(nav.dy, false);
                if (nav.accept) result.activated = model.Selected();
                if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) model.Page(1);
                if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) model.Page(-1);
            }
            if (interactive && Hover(box) && ImGui::GetIO().MouseWheel != 0) model.Move(ImGui::GetIO().MouseWheel > 0 ? -1 : 1, false);
            if (interactive) {
                for (const auto& row : rows::RowBoxes(metrics, model.First(), model.Count(), model.Selected(), box.y, box.height)) {
                    const Rect target{box.x, row.top, box.width, row.height};
                    const auto delta = ImGui::GetIO().MouseDelta;
                    if (Hover(target) && (delta.x != 0 || delta.y != 0)) model.Select(row.index);
                    if (Click(target)) { model.Select(row.index); result.activated = row.index; }
                }
            }
            result.selectionChanged = before != model.Selected();
            kit::Canvas()->PushClipRect(At(box.x - 25, box.y), At(box.x + box.width + 25, box.y + box.height), true);
            for (const auto& row : rows::RowBoxes(metrics, model.First(), model.Count(), model.Selected(), box.y, box.height)) {
                const auto& item = items[static_cast<std::size_t>(row.index)];
                const bool selected = row.index == model.Selected();
                const Rect target{box.x, row.top, box.width, row.height};
                Selection(target, selected, focused, ink, player);
                const float y = row.top + (FamilyNow() == Family::Skyrim ? selected ? 20.65F : 5.65F : 4.0F);
                ItemRow(target, item, selected, ink, prices, metrics, y);
            }
            kit::Canvas()->PopClipRect();
            if (model.Count() > model.VisibleRows()) {
                const float thumb = box.height * static_cast<float>(model.VisibleRows()) / model.Count();
                const float top = box.y + (box.height - thumb) * model.First() / std::max(1, model.Count() - model.VisibleRows());
                Box(box.x + box.width + 5, top, 2, thumb, Fade(ink, 0.6F));
            }
            return result;
        }

        view::ListResult DrawPanel(view::Side side, const view::Panel& panel, std::span<const view::Item> items,
                                   view::ListModel& model, bool focused, const view::Nav& nav, bool prices, bool interactive) {
            const auto place = Place(side);
            const auto ink = Ink();
            if (FamilyNow() == Family::Fallout4) {
                for (const float y : {place.frame.y, place.frame.y + place.frame.height}) {
                    Line(place.frame.x, y, place.frame.x + place.frame.width, y, Fade(ink, 0.65F));
                    Line(place.frame.x, y, place.frame.x, y + (y == place.frame.y ? 5 : -5), ink);
                    Line(place.frame.x + place.frame.width, y, place.frame.x + place.frame.width, y + (y == place.frame.y ? 5 : -5), ink);
                }
            } else if (FamilyNow() != Family::Starfield) frames::Panel(place.frame.x, place.frame.y, place.frame.width, place.frame.height, Fade(ink, 0.65F));
            Text(place.list.x + 8, place.title, 22, panel.title, Fade(ink, focused ? 1.0F : 0.6F), Align::Left, true, place.list.width - 16);
            auto result = List(place.list, items, model, nav, focused, interactive, prices, ink, side == view::Side::Player);
            const float footer = place.frame.y + place.frame.height + 11;
            Text(place.list.x, footer, 14, panel.weight, ink, Align::Left, false, place.list.width / 2);
            if (!panel.currencyAmount.empty()) {
                Icon(panel.currency, place.list.x + place.list.width - 92, footer + 9, 16, ink);
                Text(place.list.x + place.list.width, footer, 14, panel.currencyAmount, ink, Align::Right, false, 80);
            }
            return result;
        }

        bool Enabled(std::span<const bool> enabled, int index) { return index >= 0 && (enabled.empty() || (index < static_cast<int>(enabled.size()) && enabled[static_cast<std::size_t>(index)])); }

        int Categories(std::span<const std::string> tabs, int selected, std::span<const bool> enabled, bool interactive, ImU32 tint = 0) {
            const bool starfield = FamilyNow() == Family::Starfield;
            const float x = starfield ? 52.0F / 1.5F : 35.0F, width = starfield ? 426.5F / 1.5F : 150.0F;
            const float normal = starfield ? 83.0F / 1.5F : 40.0F, expanded = starfield ? normal : 80.0F;
            const int visible = rows::VisibleRows({normal, expanded, 20}, 540);
            const int first = std::max(0, selected - visible + 1);
            float y = starfield ? 128.0F / 1.5F : 60.0F;
            int clicked = -1;
            for (int i = first; i < static_cast<int>(tabs.size()) && i < first + visible; ++i) {
                const bool active = i == selected, available = Enabled(enabled, i);
                const float height = active ? expanded : normal;
                const auto ink = Fade(Ink(tint), available ? active ? 1.0F : 0.6F : 0.25F);
                if (starfield) Selection({x, y, width, height}, active, true, ink, true);
                const float font = starfield ? 20.0F / 1.5F : active ? 26.0F : 20.0F;
                Text(x + 6, y + (height - font) / 2, font, tabs[static_cast<std::size_t>(i)], ink,
                     Align::Left, active || starfield, width - 12, starfield ? 0 : 0.8F);
                if (interactive && available && Click({x, y, width, height})) clicked = i;
                y += height;
            }
            return clicked;
        }

        int Cycle(std::span<const std::string> tabs, int selected, std::span<const bool> enabled, bool interactive) {
            if (!interactive) return -1;
            int direction = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_Z) || common::PadPressed(FUSIONUI_PAD_LB)) --direction;
            if (ImGui::IsKeyPressed(ImGuiKey_C) || common::PadPressed(FUSIONUI_PAD_RB)) ++direction;
            return rows::NextTab(selected, direction, static_cast<int>(tabs.size()), enabled);
        }

        int Tabs(std::span<const std::string> tabs, int selected, std::span<const bool> enabled, bool interactive, ImU32 tint = 0) {
            const int moved = Cycle(tabs, selected, enabled, interactive);
            if (FamilyNow() == Family::Skyrim || FamilyNow() == Family::Starfield) {
                const int clicked = Categories(tabs, selected, enabled, interactive, tint);
                return clicked >= 0 ? clicked : moved;
            }
            const float width = (Width() - 100) / static_cast<float>(std::max<std::size_t>(1, tabs.size()));
            for (std::size_t i = 0; i < tabs.size(); ++i) {
                const float x = 50 + static_cast<float>(i) * width;
                const bool active = static_cast<int>(i) == selected, available = Enabled(enabled, static_cast<int>(i));
                const auto ink = Fade(Ink(tint), available ? active ? 1.0F : 0.6F : 0.25F);
                Text(x + width / 2, 62, active ? 20.0F : 17.0F, tabs[i], ink, Align::Center, active, width - 8);
                if (active) Line(x + 4, 91, x + width - 4, 91, ink, Classic() ? 2.0F : 1.0F);
                if (interactive && available && Click({x, 55, width, 39})) return static_cast<int>(i);
            }
            return moved;
        }

        struct Browser {
            Family family{Family::Fallout76};
            std::uint64_t session{};
            int tab{-1}, selected{};
            bool categories{true};
            double time{-1};
        };

        Browser& Browse(bool pipboy, int tab, std::uint64_t session = 0) {
            auto& state = common::ContextSlot<std::array<Browser, 2>>()[pipboy ? 1 : 0];
            if (state.family != FamilyNow() || state.session != session || ImGui::GetTime() < state.time)
                state = {.family = FamilyNow(), .session = session, .tab = tab, .selected = tab};
            if (state.tab != tab) { state.selected = tab; state.tab = tab; }
            state.time = ImGui::GetTime();
            return state;
        }

        struct CategoryResult { int tab{-1}, hint{-1}; bool consumed{true}; };
        CategoryResult CategoryPage(Browser& state, std::span<const std::string> tabs, std::span<const bool> enabled,
                                    std::span<const view::Hint> hints, const view::Nav& nav, bool interactive, bool returning, ImU32 tint = 0) {
            CategoryResult result;
            if (interactive && !returning) {
                const int moved = rows::NextTab(state.selected, nav.dy, static_cast<int>(tabs.size()), enabled);
                if (moved >= 0) state.selected = moved;
                if (const int cycled = Cycle(tabs, state.selected, enabled, true); cycled >= 0) state.selected = cycled;
            }
            const int clicked = Categories(tabs, state.selected, enabled, interactive && !returning, tint);
            int back = -1;
            for (std::size_t i = 0; i < hints.size(); ++i) if (hints[i].padButton == FUSIONUI_PAD_B) back = static_cast<int>(i);
            std::vector<view::Hint> actions{{FUSIONUI_PAD_A, "E", "SELECT", !tabs.empty() && Enabled(enabled, state.selected)}};
            if (back >= 0) actions.push_back(hints[static_cast<std::size_t>(back)]);
            const int hint = Hints(actions, 666, Ink(tint), interactive && !returning);
            if (interactive && !returning && (clicked >= 0 || nav.accept || hint == 0)) {
                if (clicked >= 0) state.selected = clicked;
                if (!tabs.empty() && Enabled(enabled, state.selected)) { state.categories = false; result.tab = state.selected; }
            }
            if (interactive && !returning && (nav.cancel || hint == 1)) {
                result.hint = back;
                result.consumed = false;
            }
            return result;
        }

        std::optional<view::PreviewSlot> Preview(float center, float top, float bottom) {
            if (bottom - top < 65) return {};
            const float size = std::min(180.0F, bottom - top);
            const auto position = At(center, top + size / 2);
            const auto display = ImGui::GetIO().DisplaySize;
            return view::PreviewSlot{position.x / display.x, position.y / display.y, std::min(1.0F, Px(size) / display.y)};
        }

        void OwnerHeader(const view::TradeView& model, int& column, bool interactive) {
            const float x = FamilyNow() == Family::Skyrim ? 535.0F : 380.0F;
            const float width = (Width() - x - 35) / 2;
            for (int side = 0; side < 2; ++side) {
                const auto& panel = side ? model.other : model.player;
                const Rect box{x + side * width, 20, width - 10, 64};
                const auto ink = Fade(Ink(), column == side ? 1.0F : 0.5F);
                Text(box.x + 8, box.y, 20, panel.title, ink, Align::Left, true, box.width - 16);
                Text(box.x + 8, box.y + 30, 12, panel.weight, ink, Align::Left, false, box.width / 2);
                if (!panel.currencyAmount.empty()) {
                    Icon(panel.currency, box.x + box.width / 2 + 12, box.y + 39, 14, ink);
                    Text(box.x + box.width - 8, box.y + 30, 12, panel.currencyAmount, ink, Align::Right, false, box.width / 2 - 28);
                }
                if (interactive && Click(box)) column = side;
            }
        }

        void Doll(const view::PaperDoll& doll, float center, float top, ImU32 ink) {
            const auto base = Fade(ink, 0.22F);
            if (Classic()) {
                const auto draw = [&](std::string_view key, ImU32 color) {
                    icons::DrawRect(kit::Canvas(), key, At(center - 105, top), At(center + 105, top + 210), color);
                };
                draw("fnv.paperdoll", base);
                draw("fnv.paperdoll.face", ink);
                if (doll.base != view::PaperDoll::Base::Underwear) Icon(doll.base == view::PaperDoll::Base::VaultSuit ? "paperdoll.vaultsuit" : "item.type.apparel", center - 80, top + 180, 24, ink);
                if (doll.goggles) Icon("paperdoll.goggles", center - 80, top + 30, 24, ink);
                if (doll.gasMask) Icon("paperdoll.gasmask", center - 80, top + 60, 24, ink);
                for (const auto& [key, covered] : std::array<std::pair<const char*, bool>, 6>{{
                    {"fnv.paperdoll.head", doll.helmet}, {"fnv.paperdoll.torso", doll.torso},
                    {"fnv.paperdoll.left_arm", doll.leftArm}, {"fnv.paperdoll.right_arm", doll.rightArm},
                    {"fnv.paperdoll.left_leg", doll.leftLeg}, {"fnv.paperdoll.right_leg", doll.rightLeg}}})
                    if (covered) draw(key, ink);
            } else {
            kit::Canvas()->AddCircleFilled(At(center, top + 18), Px(16), Fade(ink, doll.helmet ? 0.8F : 0.22F));
            const auto part = [&](float x, float y, float w, float h, bool covered) { Box(x, y, w, h, covered ? ink : base); };
            part(center - 21, top + 39, 42, 69, doll.torso);
            part(center - 43, top + 42, 17, 78, doll.leftArm);
            part(center + 26, top + 42, 17, 78, doll.rightArm);
            part(center - 21, top + 113, 18, 84, doll.leftLeg);
            part(center + 3, top + 113, 18, 84, doll.rightLeg);
            if (doll.base != view::PaperDoll::Base::Underwear) Line(center - 18, top + 88, center + 18, top + 88, Fade(ink, 0.5F), 2);
            if (doll.base == view::PaperDoll::Base::VaultSuit) Line(center, top + 40, center, top + 105, Fade(ink, 0.5F), 3);
            if (doll.goggles) Line(center - 16, top + 14, center + 16, top + 14, ink, 3);
            if (doll.gasMask) { frames::Diamond(center, top + 25, 10, ink); Icon("resist.poison", center, top + 25, 12, ink); }
            }
            Icon(doll.resistIcon, center + 80, top + 14, 20, ink);
            if (Classic()) {
                Text(center + 45, top + 1, 14, std::to_string(doll.head), ink);
                Text(center + 40, top + 110, 14, std::to_string(doll.chest), ink);
                Text(center + 100, top + 70, 14, std::to_string(doll.leftArmResist), ink);
                Text(center - 100, top + 70, 14, std::to_string(doll.rightArmResist), ink, Align::Right);
                Text(center + 65, top + 185, 14, std::to_string(doll.leftLegResist), ink);
                Text(center - 65, top + 185, 14, std::to_string(doll.rightLegResist), ink, Align::Right);
                return;
            }
            Text(center + 38, top + 1, 14, std::to_string(doll.head), ink);
            Text(center + 52, top + 68, 14, std::to_string(doll.chest), ink);
            Text(center - 63, top + 88, 14, std::to_string(doll.leftArmResist), ink, Align::Right);
            Text(center + 63, top + 105, 14, std::to_string(doll.rightArmResist), ink);
            Text(center - 37, top + 173, 14, std::to_string(doll.leftLegResist), ink, Align::Right);
            Text(center + 37, top + 190, 14, std::to_string(doll.rightLegResist), ink);
        }

        void PipFooter(const view::PipBoyView& model, ImU32 ink) {
            Text(40, 634, 14, model.weight, ink, Align::Left, false, 180);
            Icon("currency.caps", Width() / 2 - 65, 645, 17, ink);
            Text(Width() / 2, 635, 14, model.currencyAmount, ink, Align::Left, false, 100);
            if (!model.damage.empty()) {
                Icon(model.damageIcon, Width() - 115, 645, 18, ink);
                Text(Width() - 90, 635, 14, model.damage, ink, Align::Left, false, 70);
            }
        }

        Rect Modal(float width, float height, ImU32 tint = 0) {
            height = std::min(height, 630.0F);
            Box(0, 0, Width(), 720, IM_COL32(0, 0, 0, 150));
            const Rect box{(Width() - width) / 2, (720 - height) / 2, width, height};
            Box(box.x, box.y, box.width, box.height, IM_COL32(0, 0, 0, 220));
            frames::Panel(box.x, box.y, box.width, box.height, Ink(tint));
            return box;
        }

        float ModalWidth() { return FamilyNow() == Family::Skyrim ? 410.0F : FamilyNow() == Family::Starfield ? 500.0F : Classic() ? 480.0F : 540.0F; }

        template <class Available>
        int MoveOption(int selected, int direction, int count, Available available) {
            if (!count) return -1;
            selected = std::clamp(selected, 0, count - 1);
            if (direction) {
                int next = selected;
                for (int i = 0; i < count; ++i) {
                    next = (next + (direction > 0 ? 1 : count - 1)) % count;
                    if (available(next)) return next;
                }
            }
            if (!available(selected)) for (int i = 0; i < count; ++i) if (available(i)) return i;
            return selected;
        }

        view::ModalResult QuantityDraw(const view::QuantityModal& modal, rules::Quantity& quantity, const view::Nav& nav, ImU32 tint, bool pipboy) {
            view::ModalResult result;
            const int before = quantity.value;
            quantity.maximum = std::max(1, quantity.maximum);
            quantity.value = std::clamp(quantity.value, 1, quantity.maximum);
            const auto ink = Ink(tint);
            const auto box = Modal(ModalWidth(), 240, ink);
            Text(Width() / 2, box.y + 16, 22, modal.header, ink, Align::Center, true, box.width - 32);
            Text(Width() / 2, box.y + 61, 40, std::to_string(quantity.value), ink, Align::Center);
            const Rect track{box.x + 44, box.y + 140, box.width - 88, 22};
            int delta = nav.dx;
            if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1) || ImGui::IsKeyPressed(ImGuiKey_PageUp)) delta += quantity.Step();
            if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1) || ImGui::IsKeyPressed(ImGuiKey_PageDown)) delta -= quantity.Step();
            if (pipboy) {
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadR2)) delta += std::max(1, quantity.maximum / 4);
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadL2)) delta -= std::max(1, quantity.maximum / 4);
            }
            if (Hover(box) && ImGui::GetIO().MouseWheel != 0) delta += ImGui::GetIO().MouseWheel > 0 ? 1 : -1;
            if (Click({track.x - 26, track.y, 22, 22})) --delta;
            if (Click({track.x + track.width + 4, track.y, 22, 22})) ++delta;
            quantity.Adjust(delta);
            struct Drag { bool active{}; int frame{-1}; };
            auto& drag = common::ContextSlot<Drag>();
            if (drag.frame + 1 != ImGui::GetFrameCount()) drag.active = false;
            drag.frame = ImGui::GetFrameCount();
            if (Click(track)) drag.active = true;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) drag.active = false;
            if (drag.active) {
                const double fraction = std::clamp(static_cast<double>((ImGui::GetIO().MousePos.x - At(track.x, 0).x) / Px(track.width)), 0.0, 1.0);
                quantity.value = 1 + static_cast<int>(std::llround(fraction * (quantity.maximum - 1)));
            }
            frames::Bar(track.x, track.y + 9, track.width, 4, quantity.Fraction(), ink);
            Box(track.x + track.width * quantity.Fraction() - 2, track.y - 2, 4, 26, ink);
            Text(track.x - 16, track.y - 2, 23, "<", ink, Align::Center);
            Text(track.x + track.width + 16, track.y - 2, 23, ">", ink, Align::Center);
            const std::array<view::Hint, 2> hints{{{FUSIONUI_PAD_A, "E", modal.accept}, {FUSIONUI_PAD_B, "Tab", modal.cancel}}};
            const int clicked = Hints(hints, box.y + 195, ink);
            result.choice = nav.cancel ? 1 : nav.accept ? 0 : clicked;
            result.changed = before != quantity.value;
            return result;
        }
    }

    view::TradeResult TradeMenu(const view::TradeView& model, const view::Nav& nav) {
        view::TradeResult result;
        result.column = std::clamp(model.column, 0, 1);
        if (model.interactive && nav.dx) result.column = nav.dx < 0 ? 0 : 1;
        const auto ink = Ink();
        const bool single = FamilyNow() == Family::Skyrim || FamilyNow() == Family::Starfield;
        if (single) Text(35, 22, 20, model.corner, Fade(ink, 0.65F), Align::Left, false, 150);
        else Text(Width() / 2, 12, 24, model.corner, Fade(ink, 0.65F), Align::Center, false, 300);
        if (single) OwnerHeader(model, result.column, model.interactive);
        if (FamilyNow() == Family::Starfield) {
            auto& state = Browse(false, model.tab, model.session);
            if (!model.interactive) state.categories = false;
            const bool returning = !state.categories && model.interactive && nav.cancel;
            if (returning) state.categories = true;
            if (state.categories) {
                const auto category = CategoryPage(state, model.tabs, model.tabEnabled, model.hints, nav, model.interactive, returning);
                result.tab = category.tab;
                result.hint = category.hint;
                result.inputConsumed = category.consumed;
                return result;
            }
            result.tab = Cycle(model.tabs, model.tab, model.tabEnabled, model.interactive);
        } else result.tab = Tabs(model.tabs, model.tab, model.tabEnabled, model.interactive);
        const bool prices = model.kind == view::TradeKind::Barter;
        if (single) {
            if (result.column == 0) result.player = DrawPanel(view::Side::Player, model.player, model.playerItems, *model.playerList, true, nav, prices, model.interactive);
            else result.other = DrawPanel(view::Side::Other, model.other, model.otherItems, *model.otherList, true, nav, prices, model.interactive);
        } else {
            result.player = DrawPanel(view::Side::Player, model.player, model.playerItems, *model.playerList, result.column == 0, nav, prices, model.interactive);
            result.other = DrawPanel(view::Side::Other, model.other, model.otherItems, *model.otherList, result.column == 1, nav, prices, model.interactive);
            if (result.player.selectionChanged || result.player.activated >= 0) result.column = 0;
            if (result.other.selectionChanged || result.other.activated >= 0) result.column = 1;
        }
        const auto items = result.column == 0 ? model.playerItems : model.otherItems;
        const int selected = (result.column == 0 ? model.playerList : model.otherList)->Selected();
        if (selected >= 0 && selected < static_cast<int>(items.size())) {
            const auto& item = items[static_cast<std::size_t>(selected)];
            const float center = single ? Width() - 235 : Width() / 2;
            const float width = FamilyNow() == Family::Fallout4 ? 250.0F : CardWidth(center);
            const float top = DrawCard(item, center, 638, ink, single ? 430.0F : 250.0F, model.interactive, width, CardMode::Interactive);
            const float effects = std::min(120.0F, EffectsHeight(item.effects, width));
            Effects(item.effects, center, top - 9, ink, width, 120, model.interactive);
            if (single) result.preview = Preview((Place(view::Side::Player).list.x + Place(view::Side::Player).list.width + center - width / 2) / 2, 150, 590);
            else result.preview = Preview(center, 110, 290);  // fixed: a tall card draws over the model, never moves it
        }
        Text(Width() / 2, 644, 14, model.status, ink, Align::Center, false, Width() - 80);
        result.hint = Hints(model.hints, 681, ink, model.interactive);
        return result;
    }

    int InspectCard(const view::Item& item, std::span<const view::Hint> hints) {
        const auto ink = Ink();
        const float starWidth = item.name.empty() ? 0 : std::max(0, item.legendaryStars) * 19.0F;
        const float titleWidth = std::min(Width() - 80 - starWidth, TextWidth(28, item.name));
        const float titleX = (Width() - titleWidth - starWidth) / 2;
        Text(titleX, 23, 28, item.name, ink, Align::Left, true, titleWidth);
        if (!item.name.empty()) Stars(item.legendaryStars, titleX + titleWidth + 12, 40, 17, ink);
        const float center = std::min(190.0F, Width() / 4);
        const float top = DrawCard(item, center, 625, ink, 420, true, FLT_MAX, CardMode::Stats);
        Effects(item.effects, center, top - 12, ink);
        const float x = Width() - 365, width = 335;
        frames::Panel(x, 130, width, 495, ink);
        Text(x + width / 2, 142, 20, "CURRENT MODS", ink, Align::Center, true, width - 24);
        float full = 0;
        for (const auto& mod : item.mods) full += WrappedHeight(15, width - 30 - std::max(0, mod.stars) * 15, mod.text) + 10;
        auto* storage = ImGui::GetStateStorage();
        const auto id = ImGui::GetID("##inventory-mod-scroll");
        float offset = storage->GetFloat(id);
        if (Hover({x, 182, width, 430})) offset -= ImGui::GetIO().MouseWheel * 28;
        offset = std::clamp(offset, 0.0F, std::max(0.0F, full - 430));
        storage->SetFloat(id, offset);
        kit::Canvas()->PushClipRect(At(x + 10, 180), At(x + width - 10, 615), true);
        float y = 183 - offset;
        for (const auto& mod : item.mods) {
            const int stars = std::max(0, mod.stars);
            Stars(stars, x + 20, y + 8, 11, ink);
            y = Wrapped(x + 15 + stars * 15, y, 15, width - 30 - stars * 15, mod.text, ink, false, Align::Left) + 10;
        }
        kit::Canvas()->PopClipRect();
        return Hints(hints, 673, ink);
    }

    view::PipBoyResult InventoryList(const view::PipBoyView& model, const view::Nav& nav) {
        view::PipBoyResult result;
        const auto ink = Ink(model.tint);
        constexpr std::array<const char*, 4> pages{"STAT", "INV", "DATA", "RADIO"};
        Text(Width() / 2, 14, 25, pages[static_cast<std::size_t>(std::clamp(model.page, 0, 3))], ink, Align::Center);
        PipFooter(model, ink);
        if (FamilyNow() == Family::Starfield) {
            auto& state = Browse(true, model.tab);
            if (!model.interactive) state.categories = false;
            const bool returning = !state.categories && model.interactive && nav.cancel;
            if (returning) state.categories = true;
            if (state.categories) {
                const auto category = CategoryPage(state, model.tabs, {}, model.hints, nav, model.interactive, returning, model.tint);
                result.tab = category.tab;
                result.hint = category.hint;
                result.inputConsumed = category.consumed;
                return result;
            }
            result.tab = Cycle(model.tabs, model.tab, {}, model.interactive);
        } else result.tab = Tabs(model.tabs, model.tab, {}, model.interactive, model.tint);
        const auto items = model.componentView ? model.components : model.items;
        const Rect list = FamilyNow() == Family::Skyrim ? Rect{235, 128, 220, 455} :
                          FamilyNow() == Family::Starfield ? Rect{52.0F / 1.5F, 128.0F / 1.5F, 426.5F / 1.5F, 500} :
                          Rect{40, 135, Width() * 0.43F, 465};
        frames::Panel(list.x - 5, list.y - 6, list.width + 10, list.height + 12, Fade(ink, 0.6F));
        result.list = List(list, items, *model.list, nav, true, model.interactive, false, ink);
        const float center = Width() - 205;
        if (model.componentView) {
            float full = 0;
            for (const auto& owner : model.owners) full += WrappedHeight(16, 290, owner) + 5;
            const Rect box{center - 150, 160, 300, 450};
            float y = box.y - Scroll("##inventory-owners", box, full, ink, model.interactive, true);
            kit::Canvas()->PushClipRect(At(box.x, box.y), At(box.x + box.width - 6, box.y + box.height), true);
            for (const auto& owner : model.owners) y = Wrapped(box.x, y, 16, 290, owner, ink, false, Align::Left) + 5;
            kit::Canvas()->PopClipRect();
        } else {
            if (model.doll) Doll(*model.doll, center, 145, ink);
            const int selected = model.list->Selected();
            if (selected >= 0 && selected < static_cast<int>(items.size())) {
                const auto& item = items[static_cast<std::size_t>(selected)];
                const float top = DrawCard(item, center, 619, ink, model.doll ? 220.0F : 400.0F, model.interactive, FLT_MAX, CardMode::Interactive);
                if (!model.doll) Effects(item.effects, center, top - 9, ink);
                else Effects(item.effects, center, 124, ink);
            }
        }
        result.hint = Hints(model.hints, 676, ink, model.interactive);
        return result;
    }

    view::ModalResult Quantity(const view::QuantityModal& modal, rules::Quantity& quantity, const view::Nav& nav) {
        return QuantityDraw(modal, quantity, nav, 0, false);
    }

    view::ModalResult PipBoyQuantity(std::string_view header, rules::Quantity& quantity, const view::Nav& nav, ImU32 tint) {
        return QuantityDraw({header}, quantity, nav, tint, true);
    }

    int Confirm(const view::ConfirmModal& modal, int& selected, const view::Nav& nav) {
        const int count = static_cast<int>(modal.buttons.size());
        const auto available = [&](int i) { return i != modal.disabled; };
        selected = MoveOption(selected, nav.dy, count, available);
        const float width = ModalWidth();
        const float body = WrappedHeight(18, width - 50, modal.item) + 5 + WrappedHeight(15, width - 50, modal.warning) + (modal.value.empty() ? 0 : 33);
        const float bodyHeight = std::min(body, 550 - std::min(count, 9) * 32.0F);
        const auto box = Modal(width, 75 + bodyHeight + std::min(count, 9) * 32.0F);
        const auto ink = Ink();
        Text(Width() / 2, box.y + 12, 22, modal.header, ink, Align::Center, true, width - 32);
        const Rect content{box.x + 20, box.y + 51, width - 40, bodyHeight};
        float y = content.y - Scroll("##confirm-body", content, body, ink, true, true);
        kit::Canvas()->PushClipRect(At(content.x, content.y), At(content.x + content.width - 5, content.y + content.height), true);
        y = Wrapped(Width() / 2, y, 18, width - 50, modal.item, ink, false, Align::Center);
        y = Wrapped(Width() / 2, y + 5, 15, width - 50, modal.warning, ink, false, Align::Center);
        if (!modal.value.empty()) {
            Icon(modal.currency, Width() / 2 - 65, y + 12, 20, ink);
            Text(Width() / 2, y, 22, modal.value, ink, Align::Center);
            y += 33;
        }
        kit::Canvas()->PopClipRect();
        y = content.y + content.height + 8;
        const int visible = std::max(1, static_cast<int>((box.y + box.height - 15 - y) / 32));
        const int first = std::max(0, selected - visible + 1);
        for (int i = first; i < count && i < first + visible; ++i) {
            const Rect row{box.x + 20, y, width - 40, 29};
            const auto color = Fade(ink, available(i) ? 1.0F : 0.3F);
            Selection(row, i == selected, true, color, true);
            const auto text = FamilyNow() == Family::Fallout4 && i == selected ? Fade(IM_COL32_BLACK, available(i) ? 1.0F : 0.3F) : color;
            Text(Width() / 2, y + 5, 17, modal.buttons[static_cast<std::size_t>(i)], text, Align::Center, false, width - 55);
            if (available(i) && Click(row)) { selected = i; return i; }
            y += 32;
        }
        if (nav.cancel) return count - 1;
        if (nav.accept && selected >= 0 && available(selected)) return selected;
        return -1;
    }

    int Repair(const view::RepairModal& modal, int& selected, const view::Nav& nav) {
        const int count = static_cast<int>(modal.options.size());
        const auto available = [&](int i) { return modal.options[static_cast<std::size_t>(i)].enabled; };
        selected = MoveOption(selected, nav.dy, count, available);
        const auto* option = selected >= 0 ? &modal.options[static_cast<std::size_t>(selected)] : nullptr;
        const float width = ModalWidth();
        float costs = 0;
        for (const auto& choice : modal.options) {
            float height = 0;
            for (const auto& cost : choice.costs) height += WrappedHeight(14, width - 50, cost) + 3;
            costs = std::max(costs, height);
        }
        const auto box = Modal(width, 190 + std::min(count, 7) * 32.0F + costs + WrappedHeight(14, width - 40, modal.notice));
        const auto ink = Ink();
        Text(Width() / 2, box.y + 12, 22, modal.header, ink, Align::Center, true, width - 32);
        Condition(box.x + 30, box.y + 68, (width - 110) / 2, modal.before, ink);
        Text(Width() / 2, box.y + 51, 22, ">", ink, Align::Center);
        float y = box.y + 99;
        const int first = std::max(0, selected - 6);
        for (int i = first; i < count && i < first + 7; ++i) {
            const auto& choice = modal.options[static_cast<std::size_t>(i)];
            const Rect row{box.x + 20, y, width - 40, 29};
            const auto color = Fade(ink, choice.enabled ? 1.0F : 0.3F);
            Selection(row, i == selected, true, color, true);
            const auto text = FamilyNow() == Family::Fallout4 && i == selected ? Fade(IM_COL32_BLACK, choice.enabled ? 1.0F : 0.3F) : color;
            Text(row.x + 8, y + 4, 16, choice.label, text, Align::Left, false, row.width - 75);
            Text(row.x + row.width - 8, y + 4, 15, choice.target, text, Align::Right, false, 65);
            if (choice.enabled && Click(row)) selected = i;
            y += 32;
        }
        option = selected >= 0 ? &modal.options[static_cast<std::size_t>(selected)] : nullptr;
        Condition(Width() / 2 + 25, box.y + 68, (width - 110) / 2, option ? option->after : modal.before, ink);
        costs = 0;
        if (option) for (const auto& cost : option->costs) costs += WrappedHeight(14, width - 50, cost) + 3;
        const Rect content{box.x + 20, y, width - 40, std::max(1.0F, box.y + box.height - 48 - y)};
        const float full = costs + 7 + WrappedHeight(14, width - 50, modal.notice);
        y -= Scroll("##repair-costs", content, full, ink, true, true);
        kit::Canvas()->PushClipRect(At(content.x, content.y), At(content.x + content.width - 5, content.y + content.height), true);
        if (option) for (const auto& cost : option->costs) y = Wrapped(box.x + 25, y + 3, 14, width - 50, cost, Fade(ink, option->costsShort ? 0.4F : 1.0F), false, Align::Left);
        Wrapped(Width() / 2, y + 7, 14, width - 50, modal.notice, ink, false, Align::Center);
        kit::Canvas()->PopClipRect();
        const std::array<view::Hint, 2> hints{{{FUSIONUI_PAD_A, "Space", modal.confirm, option && option->enabled}, {FUSIONUI_PAD_B, "Tab", modal.cancel}}};
        const int clicked = Hints(hints, box.y + box.height - 39, ink);
        if (nav.cancel || clicked == 1) return count;
        if ((nav.accept || ImGui::IsKeyPressed(ImGuiKey_Space, false) || clicked == 0) && option && option->enabled) return selected;
        return -1;
    }

    view::ListResult InventoryPanel(view::Side side, const view::Panel& panel, std::span<const view::Item> items,
                                    view::ListModel& model, bool focused, const view::Nav& nav, bool prices) {
        return DrawPanel(side, panel, items, model, focused, nav, prices, true);
    }

    int TabBar(std::span<const std::string> tabs, int selected, std::span<const bool> enabled) {
        int direction = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_Z) || common::PadPressed(FUSIONUI_PAD_LB)) --direction;
        if (ImGui::IsKeyPressed(ImGuiKey_C) || common::PadPressed(FUSIONUI_PAD_RB)) ++direction;
        const int moved = rows::NextTab(selected, direction, static_cast<int>(tabs.size()), enabled);
        if (FamilyNow() == Family::Skyrim || FamilyNow() == Family::Starfield) {
            const int clicked = Categories(tabs, selected, enabled, true);
            return clicked >= 0 ? clicked : moved;
        }
        const float width = (Width() - 100) / static_cast<float>(std::max<std::size_t>(1, tabs.size()));
        for (std::size_t i = 0; i < tabs.size(); ++i) {
            const float x = 50 + static_cast<float>(i) * width;
            const bool active = static_cast<int>(i) == selected, available = Enabled(enabled, static_cast<int>(i));
            const auto ink = Fade(Ink(), available ? active ? 1.0F : 0.6F : 0.25F);
            Text(x + width / 2, 62, active ? 20.0F : 17.0F, tabs[i], ink, Align::Center, active, width - 8);
            if (active) Line(x + 4, 91, x + width - 4, 91, ink, Classic() ? 2.0F : 1.0F);
            if (available && Click({x, 55, width, 39})) return static_cast<int>(i);
        }
        return moved;
    }

    float ItemCard(const view::Item& item, float centerX, float bottom, view::CardLook look) {
        return DrawCard(item, centerX / hudDraw::Scale(), bottom / hudDraw::Scale(), Ink(), look == view::CardLook::Inspect ? 480.0F : 450.0F) * hudDraw::Scale();
    }

    void EffectLines(std::span<const view::Effect> effects, float centerX, float bottom) {
        Effects(effects, centerX / hudDraw::Scale(), bottom / hudDraw::Scale(), Ink());
    }

    void WorkbenchItems(const view::WorkbenchList& list) {
        const auto ink = Ink();
        const float scale = hudDraw::Scale(), x = list.x / scale, width = list.width / scale;
        // FallUI's workbench list: narrow numeric columns, and a name with its subtitle in one stock row.
        constexpr float kCellWidth = 30;
        auto metrics = rows::Rows(FamilyNow());
        if (!list.title.empty() || !list.headings.empty()) {
            const float y = list.headingTop / scale;
            Text(x + 12, y, 12, list.title, Fade(ink, 0.7F), Align::Left, false, width / 2);
            float right = x + width - 10;
            for (auto heading = list.headings.rbegin(); heading != list.headings.rend(); ++heading, right -= kCellWidth)
                Text(right, y, 12, *heading, Fade(ink, 0.7F), Align::Right, false, kCellWidth - 4);
        }
        for (const auto& row : list.rows) {
            const Rect target{x, row.top / scale, width, row.height / scale};
            metrics.font = std::min(rows::Rows(FamilyNow()).font, target.height * (row.subtitle.empty() ? 0.68F : 0.6F));
            const float lines = metrics.font * (row.subtitle.empty() ? 1.0F : 1.62F);
            Selection(target, row.selected, true, ink, true);
            ItemRow(target, row.item, row.selected, ink, false, metrics, target.y + (target.height - lines) / 2 - metrics.font * 0.1F,
                    row.subtitle, kCellWidth, false);
        }
    }

    void ButtonHintBar(std::span<const view::ButtonHint> hints, float centerX, float plateTop, ImU32 ink) {
        std::vector<view::Hint> buttons;
        for (const auto& hint : hints) buttons.push_back({hint.padButton, hint.key.c_str(), hint.label.c_str(), hint.enabled});
        Hints(buttons, plateTop / hudDraw::Scale(), ink, false, centerX / hudDraw::Scale());
    }
}
