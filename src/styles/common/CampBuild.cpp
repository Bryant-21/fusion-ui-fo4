#include "styles/common/CampBuild.h"
#include "styles/Controls.h"
#include "styles/Frames.h"
#include "styles/fo76/CampBuild.h"
#include "core/InventoryStyles.h"
#include <format>

namespace fusionui::styles::camp {
    namespace {
        using namespace hudDraw;
        using namespace controls;
        using modern::theme::Family;
        namespace nav = core::camp;
        Family CurrentFamily() { return modern::theme::CurrentAppearance().family; }
        bool Fo4() { return CurrentFamily() == Family::Fallout4; }
        bool Skyrim() { return CurrentFamily() == Family::Skyrim; }
        bool Starfield() { return CurrentFamily() == Family::Starfield; }
        constexpr float kStage = 1.5F;

        struct Geometry { Rect categories, items, card; float main, sub, item; int columns, variantRows; };
        Geometry Places(const view::Layout& layout, bool expanded = false, bool split = false) {
            const float left = layout.left / kStage, right = layout.card / kStage;
            const float wide = 1280 + (layout.card - layout.left) / kStage;
            if (Fo4()) return {{48 + left, 550, split ? (wide - 112) * 0.35F : wide - 96, 95},
                {48 + left + (split ? (wide - 112) * 0.35F + 16 : 0), 550, split ? (wide - 112) * 0.65F : wide - 96, 95},
                expanded ? Rect{730 + right, 60, 500, 590} : Rect{800.45F + right, 224.6F, 300, 275},
                100, 100, 100, std::max(1, static_cast<int>((split ? (wide - 112) * 0.65F : wide - 96) / 100)), 1};
            if (Skyrim()) return {{35 + left, 60, 150, 540}, {235 + left, 100, 240, 510},
                expanded ? Rect{830 + right, 60, 400, 590} : Rect{900 + right, 85, 330, 500}, 40, 30, 35, 2, 5};
            if (Starfield()) return {{1584.0F / kStage + right, 105.6F / kStage, 280.0F / kStage, 500},
                {1584.0F / kStage + right, 105.6F / kStage, 280.0F / kStage, 500},
                expanded ? Rect{550 + right, 60, 480, 590} : Rect{550 + right, 110, 280, 480}, 68.0F / kStage, 68.0F / kStage, 68.0F / kStage, 2, 5};
            return {{50 + left, 100, 220, 510}, {300 + left, 100, 240, 510},
                expanded ? Rect{780 + right, 60, 450, 590} : Rect{900 + right, 100, 330, 490}, 35, 30, 32, 3, 5};
        }

        struct CardLine {
            enum class Kind { Name, Banner, Count, Header, Row, Text, More } kind;
            float y, height, font;
            std::string text, value;
            const view::CardRow* row{};
        };
        struct CardLayout { std::vector<CardLine> lines; float height{}, total{}; bool cut{}; };
        CardLayout LayCard(const view::Card& card, bool expanded) {
            CardLayout result;
            if (card.name.empty()) return result;
            const auto geometry = Places({}, expanded).card;
            const float width = geometry.width - 24;
            float y = 12;
            const auto text = [&](CardLine::Kind kind, std::string_view label, float font, bool bold) {
                if (label.empty()) return;
                const float height = WrappedHeight(font, width, label, bold) + 7;
                result.lines.push_back({kind, y, height, font, std::string(label)});
                y += height;
            };
            if (!Fo4() || expanded) text(CardLine::Kind::Name, card.name, 22, true);
            text(CardLine::Kind::Banner, card.banner, 18, true);
            if (!card.built.empty()) text(CardLine::Kind::Count, card.builtLabel + " " + card.built, 16, false);
            if (card.stored > 0) text(CardLine::Kind::Count, std::format("{} {}", card.storedLabel, card.stored), 16, false);
            for (const auto& section : card.sections) {
                if (section.rows.empty()) continue;
                text(CardLine::Kind::Header, section.title, 16, true);
                const std::size_t limit = !expanded && section.fits ? std::min<std::size_t>(8, section.rows.size()) : section.rows.size();
                for (std::size_t i = 0; i < limit; ++i) {
                    const auto& row = section.rows[i];
                    const float font = Fo4() ? 21.0F : 17.0F;
                    const float badges = (row.tagged ? 18.0F : 0) + (row.transferLocked ? 18.0F : 0) + (row.icon.empty() ? 0 : 24);
                    const float height = std::max(Fo4() ? 25.425F : 28.0F, WrappedHeight(font, width * 0.65F - badges, row.label) + 3);
                    result.lines.push_back({CardLine::Kind::Row, y, height, font, row.label, row.value, &row});
                    y += height;
                }
                if (limit != section.rows.size()) { text(CardLine::Kind::More, card.moreLabel, 16, false); result.cut = true; }
            }
            text(CardLine::Kind::Text, card.description, 16, false);
            result.total = y + 5;
            result.height = std::min(result.total, geometry.height);
            if (result.total > result.height) result.cut = true;
            return result;
        }

        void Mark(Rect box, ImU32 ink) {
            if (Fo4()) {
                for (const float x : {box.x, box.x + box.width}) {
                    const float direction = x == box.x ? 1.0F : -1.0F;
                    Line(x, box.y, x, box.y + box.height, ink, 2);
                    Line(x, box.y, x + direction * 8, box.y, ink, 2);
                    Line(x, box.y + box.height, x + direction * 8, box.y + box.height, ink, 2);
                }
            } else {
                Box(box.x, box.y, box.width, box.height, Fade(ink, 0.12F));
                Line(box.x, box.y + box.height - 1, box.x + box.width, box.y + box.height - 1, ink);
                if (Skyrim()) frames::Diamond(box.x + box.width, box.y + box.height / 2, 3, ink);
                else kit::Canvas()->AddRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height), ink, 0, 0, Px(1));
            }
        }

        bool Tile(const view::Tile& tile, Rect box, bool selected, const view::Palette& palette, bool horizontal, bool interactive) {
            kit::Canvas()->PushClipRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height), true);
            const auto ink = Fade(palette.text, tile.state == view::TileState::Unavailable ? 0.3F : 1.0F);
            if (selected) Mark(box, palette.accent);
            const float size = horizontal ? std::min(52.0F, box.height - 38) : std::min(36.0F, box.height - 8);
            const float cx = horizontal ? box.x + box.width / 2 : box.x + size / 2 + 5, cy = box.y + (horizontal ? size / 2 + 3 : box.height / 2);
            if (tile.thumbnail) kit::Canvas()->AddImage(tile.thumbnail, At(cx - size / 2, cy - size / 2), At(cx + size / 2, cy + size / 2), {0, 0}, {1, 1}, Fade(IM_COL32_WHITE, tile.state == view::TileState::Unavailable ? 0.3F : 1.0F));
            else Icon("hud.loot", cx, cy, size * 0.65F, ink);
            if (horizontal) Wrapped(box.x + box.width / 2, box.y + size + 7, 14, box.width - 12, tile.name, ink, true, kit::Align::Center);
            else {
                const float badges = (tile.state == view::TileState::Buildable ? 0.0F : 17.0F) + (tile.premium == view::Premium::None ? 0.0F : 18.0F) + (tile.hasVariants ? 18.0F : 0.0F);
                const float x = box.x + size + 12, width = box.width - size - 22 - badges;
                const float font = Starfield() ? 12.0F : 16.0F;
                kit::Canvas()->PushClipRect(At(x, box.y + 3), At(x + width, box.y + box.height - 3), true);
                if (selected) Wrapped(x, box.y + 5, font, width, tile.name, ink, true, kit::Align::Left);
                else Text(x, box.y + (box.height - font) / 2, font, tile.name, ink);
                kit::Canvas()->PopClipRect();
            }
            float badgeX = box.x + box.width - 10;
            if (tile.state == view::TileState::Locked) { Icon("item.locked", badgeX, box.y + 10, 13, palette.hostile); badgeX -= 17; }
            if (tile.state == view::TileState::MissingMaterials) { Icon("camp.badge.missing", badgeX, box.y + 10, 13, palette.hostile); badgeX -= 17; }
            if (tile.state == view::TileState::Unavailable) { Line(badgeX - 5, box.y + 5, badgeX + 5, box.y + 15, palette.hostile); Line(badgeX + 5, box.y + 5, badgeX - 5, box.y + 15, palette.hostile); badgeX -= 17; }
            if (tile.premium != view::Premium::None) { Icon(tile.premium == view::Premium::Fallout1st ? "camp.badge.fo1st" : "camp.badge.premium", badgeX, box.y + 10, 14, tile.entitled ? palette.accent : palette.hostile); badgeX -= 18; }
            if (tile.hasVariants) Icon("camp.badge.variants", badgeX, box.y + box.height - 10, 14, palette.accent);
            kit::Canvas()->PopClipRect();
            return interactive && Click(box);
        }

        std::optional<nav::Row> Categories(const view::Model& model, const nav::NavShape& shape, Rect box, bool horizontal, bool interactive) {
            const auto rows = nav::RowsOf(shape, model.nav.category, model.nav.sub >= 0);
            std::optional<nav::Row> clicked;
            float cursor = horizontal ? box.x : box.y;
            kit::Canvas()->PushClipRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height), true);
            for (int i = model.nav.firstRow; i < static_cast<int>(rows.size()); ++i) {
                const auto row = rows[static_cast<std::size_t>(i)];
                const auto& category = model.categories[static_cast<std::size_t>(row.category)];
                const bool active = row.category == model.nav.category && row.sub == model.nav.sub;
                const float size = row.sub < 0 ? (Skyrim() && row.category == model.nav.category ? 80.0F : shape.mainRow / kStage) : nav::SubRowHeight(shape, row.category, row.sub) / kStage;
                const Rect target = horizontal ? Rect{cursor, box.y, size, box.height} : Rect{box.x, cursor, box.width, size};
                if (horizontal ? target.x + target.width > box.x + box.width + 0.01F : target.y + target.height > box.y + box.height + 0.01F) break;
                const auto ink = Fade(active ? model.palette.accent : model.palette.text, category.enabled ? active ? 1.0F : 0.65F : 0.25F);
                if (active) Mark(target, ink);
                const auto& label = row.sub < 0 ? category.label : category.subs[static_cast<std::size_t>(row.sub)];
                if (horizontal) {
                    Icon(category.icon, target.x + target.width / 2, target.y + 25, 35, ink);
                    Wrapped(target.x + target.width / 2, target.y + 51, 14, target.width - 10, label, ink, active, kit::Align::Center);
                } else {
                    const float icon = Skyrim() ? 0.0F : 24.0F;
                    if (icon > 0) Icon(category.icon, target.x + 17, target.y + size / 2, icon, ink);
                    const float font = Skyrim() ? active && row.sub < 0 ? 26.0F : 20.0F : Starfield() ? 12.0F : 18.0F;
                    const float x = target.x + (icon > 0 ? 36 : 6) + (row.sub >= 0 ? 10 : 0), width = target.x + target.width - 8 - x;
                    const float height = WrappedHeight(font, width, label, active);
                    Wrapped(x, target.y + std::max(3.0F, (size - height) / 2), font, width, label, ink, active, kit::Align::Left);
                }
                if (interactive && category.enabled && Click(target)) clicked = row;
                cursor += size;
            }
            kit::Canvas()->PopClipRect();
            return clicked;
        }

        void Card(const view::Model& model, const view::Layout& layout) {
            const auto& card = model.card;
            if (card.name.empty()) return;
            const auto box = Places(layout, model.cardExpanded).card;
            const auto content = LayCard(card, model.cardExpanded);
            if (Fo4() && !model.cardExpanded) Text(634.7F, 135.95F, 22, card.name, model.palette.accent, kit::Align::Center, true, 520, 0.3F);
            frames::Panel(box.x, box.y, box.width, content.height, model.palette.text);
            const float offset = model.cardExpanded ? std::clamp(model.cardScroll / kStage, 0.0F, std::max(0.0F, content.total - content.height)) : 0;
            const float footer = content.cut && !model.cardExpanded ? 28.0F : 5.0F;
            kit::Canvas()->PushClipRect(At(box.x + 6, box.y + 5), At(box.x + box.width - 6, box.y + content.height - footer), true);
            for (const auto& line : content.lines) {
                float x = box.x + 12, y = box.y + line.y - offset;
                auto ink = line.kind == CardLine::Kind::Name || line.kind == CardLine::Kind::Header || line.kind == CardLine::Kind::Banner ? model.palette.accent : model.palette.text;
                if (line.row) {
                    ink = line.row->missing ? model.palette.hostile : line.row->publicMaterials ? model.palette.friendly : model.palette.text;
                    if (!line.row->icon.empty()) { Icon(line.row->icon, x + 8, y + 10, 17, ink); x += 25; }
                    float right = box.x + box.width - 12;
                    if (line.row->tagged) { Icon("item.tagged", right - 6, y + 10, 13, ink); right -= 18; }
                    if (line.row->transferLocked) { Icon("item.locked", right - 6, y + 10, 13, ink); right -= 18; }
                    Text(right, y, line.font, line.value, ink, kit::Align::Right, true, box.width * 0.3F);
                    Wrapped(x, y, line.font, std::max(30.0F, box.width * 0.65F - (x - box.x) - (box.x + box.width - 12 - right)), line.text, ink, false, kit::Align::Left);
                } else Wrapped(x, y, line.font, box.width - 24, line.text, ink,
                               line.kind == CardLine::Kind::Name || line.kind == CardLine::Kind::Header || line.kind == CardLine::Kind::Banner, kit::Align::Left);
            }
            kit::Canvas()->PopClipRect();
            if (content.cut && !model.cardExpanded) Text(box.x + box.width - 12, box.y + content.height - 21, 14, card.moreLabel, model.palette.accent, kit::Align::Right, true);
            if (content.total > content.height && model.cardExpanded) {
                const float thumb = (content.height - 15) * content.height / content.total;
                Box(box.x + box.width - 4, box.y + 7 + (content.height - 15 - thumb) * offset / (content.total - content.height), 2, thumb, model.palette.text);
            }
        }

        void CameraHelp(const view::Model& model, const view::Layout& layout) {
            if (!model.cameraHelp.empty()) {
                const auto box = Places(layout, model.cardExpanded).card;
                const auto content = LayCard(model.card, model.cardExpanded);
                const float height = 14 + static_cast<float>(model.cameraHelp.size()) * 25;
                const float top = Fo4() ? std::max(60.0F, box.y - height - 10) : std::min(box.y + content.height + 10, 650 - height);
                frames::Panel(box.x, top, box.width, height, model.palette.text);
                for (std::size_t i = 0; i < model.cameraHelp.size(); ++i) Text(box.x + 12, top + 7 + static_cast<float>(i) * 25, 16, model.cameraHelp[i], model.palette.text, kit::Align::Left, false, box.width - 24);
            }
        }

        void Popups(const view::Model& model, const view::Layout& layout) {
            if (!model.quickMenu.options.empty()) {
                float width = 220;
                for (const auto& row : model.quickMenu.options) width = std::max(width, TextWidth(18, row.label) + 40);
                width = std::min(width, 500.0F);
                const int count = static_cast<int>(model.quickMenu.options.size()), visible = std::min(18, count);
                const float height = 20 + visible * 30.0F, x = layout.quickMenu.x / kStage - width / 2, top = (720 - height) / 2;
                const int first = std::clamp(model.quickMenu.selected - visible + 1, 0, count - visible);
                Box(x, top, width, height, IM_COL32(0, 0, 0, 230)); frames::Panel(x, top, width, height, model.palette.text);
                for (int i = first; i < first + visible; ++i) {
                    const auto& row = model.quickMenu.options[static_cast<std::size_t>(i)];
                    const Rect rect{x + 10, top + 10 + (i - first) * 30, width - 20, 28};
                    if (i == model.quickMenu.selected) Mark(rect, model.palette.accent);
                    Text(rect.x + rect.width / 2, rect.y + 4, 18, row.label, Fade(model.palette.text, row.enabled ? 1.0F : 0.3F), kit::Align::Center, false, rect.width - 10);
                }
            }
            if (!model.blueprints.rows.empty()) {
                const float width = 420, top = 160, x = 640 - width / 2;
                frames::Panel(x, top, width, 450, model.palette.text);
                Text(640, top + 15, 22, model.blueprints.title, model.palette.accent, kit::Align::Center, true, width - 24);
                const int count = static_cast<int>(model.blueprints.rows.size()), first = std::clamp(model.blueprints.selected - 9, 0, std::max(0, count - 10));
                for (int i = first; i < count && i < first + 10; ++i) {
                    const Rect row{x + 15, top + 61 + (i - first) * 36, width - 30, 34};
                    if (i == model.blueprints.selected) Mark(row, model.palette.accent);
                    Text(row.x + 10, row.y + 5, 18, model.blueprints.rows[static_cast<std::size_t>(i)], model.palette.text, kit::Align::Left, false, row.width - 20);
                }
            }
        }
    }

    view::Layout LayoutFor(float width, float height) {
        const float scale = width > 0 && height > 0 ? std::min(width / 1920, height / 1080) : 0;
        const float extra = scale > 0 ? (width / scale - 1920) / 2 : 0;
        view::Layout layout{.left = -extra, .card = extra, .quickMenu = {960, 540}, .hintTop = 1008};
        if (Fo4()) layout.preview = std::array{1500.0F + extra, 97.5F, 1845.0F + extra, 315.0F};
        else if (Skyrim()) layout.preview = std::array{750.0F, 225.0F, 1275.0F, 915.0F};
        else if (Starfield()) layout.preview = std::array{375.0F, 225.0F, 780.0F, 915.0F};
        else layout.preview = std::array{840.0F, 225.0F, 1245.0F, 915.0F};
        return layout;
    }

    nav::NavShape ShapeOf(const view::Model& model) {
        const auto display = ImGui::GetIO().DisplaySize;
        const auto geometry = Places(fusionui::styles::camp::LayoutFor(display.x, display.y), false,
                                     model.keepExpanded && model.nav.focus != view::Focus::Categories);
        nav::NavShape shape;
        for (const auto& category : model.categories) shape.subCounts.push_back(static_cast<int>(category.subs.size()));
        shape.items = static_cast<int>(model.items.size());
        shape.variants = static_cast<int>(model.variants.size());
        shape.horizontalStrips = Fo4();
        shape.mainRow = geometry.main * kStage; shape.subRow = geometry.sub * kStage;
        shape.mainSelectedRow = Skyrim() ? 80 * kStage : 0;
        if (!Fo4()) for (std::size_t i = 0; i < model.categories.size(); ++i) {
            std::vector<float> heights;
            for (std::size_t s = 0; s < model.categories[i].subs.size(); ++s) {
                const bool active = static_cast<int>(i) == model.nav.category && static_cast<int>(s) == model.nav.sub;
                const float font = Skyrim() ? 20.0F : Starfield() ? 12.0F : 18.0F;
                const float width = geometry.categories.width - (Skyrim() ? 24 : 54);
                heights.push_back(std::max(geometry.sub, WrappedHeight(font, width, model.categories[i].subs[s], active) + 10) * kStage);
            }
            shape.subRows.push_back(std::move(heights));
        }
        shape.rowsViewport = (Fo4() ? geometry.categories.width : geometry.categories.height) * kStage;
        shape.visibleItems = Fo4() ? std::max(1, static_cast<int>(geometry.items.width / geometry.item)) : Skyrim() ? core::inventory::styles::VisibleRows({35, 65, 20}, geometry.items.height) : std::max(1, static_cast<int>(geometry.items.height / geometry.item));
        shape.variantColumns = geometry.columns; shape.visibleVariantRows = model.variantsMinimized ? 1 : geometry.variantRows;
        return shape;
    }

    float CardBottom(const view::Card& card, bool expanded) { return (Places({}, expanded).card.y + LayCard(card, expanded).height) * kStage; }
    std::pair<float, float> CardSpan(const view::Layout& layout, bool expanded) { const auto box = Places(layout, expanded).card; return {box.x * kStage, (box.x + box.width) * kStage}; }
    bool CardExpandable(const view::Card& card) { return LayCard(card, false).cut; }
    float CardScrollLimit(const view::Card& card) { const auto layout = LayCard(card, true); return std::max(0.0F, layout.total - layout.height) * kStage; }

    view::Result Menu(const view::Model& model, const view::Input& input) {
        auto shown = model;
        const auto shape = fusionui::styles::camp::ShapeOf(model);
        shown.nav = nav::Step(model.nav, shape, {}).state;
        shown.palette = fo76::camp::Tinted(model.palette, common::HudColor());
        const auto& m = shown;
        const auto display = ImGui::GetIO().DisplaySize;
        const auto layout = fusionui::styles::camp::LayoutFor(display.x, display.y);
        const auto geometry = Places(layout, false, m.keepExpanded && m.nav.focus != view::Focus::Categories);
        const bool categories = m.nav.focus == view::Focus::Categories;
        const bool interactive = m.quickMenu.options.empty() && m.blueprints.rows.empty();
        auto state = m.nav;
        nav::NavInput move{.dy = input.nav.dy, .dx = input.nav.dx, .accept = input.nav.accept, .cancel = input.nav.cancel, .cycle = input.cycle};
        if (!interactive) move = {};
        bool modeClick = false;
        const float left = 35 + layout.left / kStage;
        Text(left, 16, 20, m.buildLabel, Fade(m.palette.accent, m.modify ? 0.55F : 1.0F), kit::Align::Left, true, 150);
        Text(left + 165, 16, 20, m.modifyLabel, Fade(m.palette.accent, m.modify ? 1.0F : 0.55F), kit::Align::Left, true, 150);
        if (m.gamepad && m.modePad != view::kNoPad) common::PadGlyph(kit::Canvas(), At(left + 330, 29), Px(11), m.modePad, m.palette.text);
        else Text(left + 315, 17, 16, m.modeKey, m.palette.text);
        modeClick = interactive && Click({left, 10, 300, 34});
        const bool blueprint = m.blueprintBudget >= 0;
        Text(640, 15, 14, blueprint ? m.blueprintBudgetLabel : m.budgetLabel, m.palette.text, kit::Align::Center, true, 320);
        frames::Bar(490, 39, 300, 5, blueprint ? m.blueprintBudget : m.budget, blueprint ? m.palette.blueprint : m.palette.text);
        if (!blueprint && m.budgetStored > 0) {
            const float stored = std::clamp(m.budgetStored, 0.0F, 1.0F);
            Box(790 - 300 * stored, 39, 300 * stored, 5, m.palette.stored);
            Text(800, 29, 12, m.storedLabel, m.palette.stored, kit::Align::Left, false, 220);
        }
        if (!m.message.empty()) {
            float font = 16;
            const float needed = WrappedHeight(font, 700, m.message);
            if (needed > 55) font *= 55 / needed;
            Wrapped(640, 65, font, 700, m.message, m.palette.hostile, false, kit::Align::Center);
        }
        const bool showCategories = !Fo4() && !Starfield() ? true : categories || m.keepExpanded;
        if (showCategories) {
            auto box = geometry.categories;
            if (Starfield() && !categories) box.x -= box.width + 18;
            if (!Fo4()) frames::Panel(box.x - 5, box.y - 5, box.width + 10, box.height + 10, Fade(m.palette.text, 0.65F));
            if (const auto row = Categories(m, shape, box, Fo4(), interactive)) {
                state.category = row->category; state.sub = row->sub; state.focus = view::Focus::Categories; move = {};
                if (state.category != m.nav.category || state.sub != m.nav.sub) state.item = state.firstItem = 0;
            }
        }
        if (Fo4()) {
            std::string path;
            if (m.nav.category >= 0 && m.nav.category < static_cast<int>(m.categories.size())) {
                const auto& category = m.categories[static_cast<std::size_t>(m.nav.category)]; path = category.label;
                if (m.nav.sub >= 0 && m.nav.sub < static_cast<int>(category.subs.size())) path += " / " + category.subs[static_cast<std::size_t>(m.nav.sub)];
            }
            Text(48 + layout.left / kStage, 516.85F, 22, path, m.palette.text, kit::Align::Left, true, geometry.items.width, 0.3F);
        }
        const bool variants = m.nav.focus == view::Focus::Variants && m.variants.size() > 1;
        const bool showItems = !categories || (!Fo4() && !Starfield());
        if (showItems) {
            const auto box = geometry.items;
            if (!Fo4()) frames::Panel(box.x - 5, box.y - 5, box.width + 10, box.height + 10, m.palette.text);
            if (variants) {
                if (!Fo4()) Text(box.x + box.width / 2, box.y + 4, 16, m.variantGroup, m.palette.accent, kit::Align::Center, true, box.width - 10);
                const float cell = std::min(Fo4() ? 92.0F : 90.0F, box.width / shape.variantColumns - 4), pitch = cell + 4;
                const float top = Fo4() ? box.y : box.y + 34;
                const int first = m.nav.firstVariantRow * shape.variantColumns, last = std::min(static_cast<int>(m.variants.size()), first + shape.variantColumns * shape.visibleVariantRows);
                for (int i = first; i < last; ++i) {
                    const Rect target{box.x + static_cast<float>((i - first) % shape.variantColumns) * pitch, top + static_cast<float>((i - first) / shape.variantColumns) * pitch, cell, cell};
                    if (Tile(m.variants[static_cast<std::size_t>(i)], target, i == m.nav.variant, m.palette, true, interactive)) { state.variant = i; state.focus = view::Focus::Variants; move = {.accept = i == m.nav.variant}; }
                }
            } else if (Fo4()) {
                const int last = std::min(static_cast<int>(m.items.size()), m.nav.firstItem + shape.visibleItems);
                if (m.items.empty()) Text(box.x, box.y + 25, 18, m.none, m.palette.text);
                for (int i = m.nav.firstItem; i < last; ++i) {
                    const Rect target{box.x + static_cast<float>(i - m.nav.firstItem) * geometry.item, box.y, geometry.item - 4, box.height};
                    if (Tile(m.items[static_cast<std::size_t>(i)], target, i == m.nav.item, m.palette, true, interactive)) { state.item = i; state.focus = view::Focus::Items; move = {.accept = i == m.nav.item && m.nav.focus == view::Focus::Items}; }
                }
            } else {
                const auto metrics = Skyrim() ? core::inventory::styles::Metrics{35, 65, 20} : core::inventory::styles::Metrics{geometry.item, geometry.item, 16};
                if (m.items.empty()) Text(box.x + 10, box.y + 15, 18, m.none, m.palette.text, kit::Align::Left, false, box.width - 20);
                for (const auto& row : core::inventory::styles::RowBoxes(metrics, m.nav.firstItem, static_cast<int>(m.items.size()), m.nav.item, box.y, box.height)) {
                    if (Tile(m.items[static_cast<std::size_t>(row.index)], {box.x, row.top, box.width, row.height - 1}, row.index == m.nav.item, m.palette, false, interactive)) { state.item = row.index; state.focus = view::Focus::Items; move = {.accept = row.index == m.nav.item && m.nav.focus == view::Focus::Items}; }
                }
            }
            std::string count = m.variants.size() > 1 ? m.variantCount : m.variantNone;
            for (const auto [key, value] : {std::pair{"{1}", std::to_string(std::max(1, m.nav.variant + 1))}, std::pair{"{2}", std::to_string(m.variants.size())}})
                if (const auto at = count.find(key); at != std::string::npos) count.replace(at, 3, value);
            if (!Fo4()) Text(box.x, box.y + box.height + 10, 13, count, m.palette.text, kit::Align::Left, false, box.width);
        }
        Card(m, layout);
        CameraHelp(m, layout);
        std::vector<fo4::Hint> hints;
        for (const auto& hint : m.hints) hints.push_back({hint.padButton, hint.key.c_str(), hint.label.c_str(), hint.enabled});
        controls::Hints(hints, layout.hintTop / kStage, m.palette.text, false, -1, kit::Align::Center, m.gamepad);
        Popups(m, layout);
        if (m.cameraDot) {
            auto* canvas = ImGui::GetForegroundDrawList(); const ImVec2 center{display.x / 2, display.y / 2};
            canvas->AddCircleFilled(center, Px(2.66F), IM_COL32_BLACK); canvas->AddCircleFilled(center, Px(1.66F), m.palette.text);
        }
        const auto step = nav::Step(state, shape, move);
        return {.nav = step.state, .categoryChanged = step.state.category != model.nav.category || step.state.sub != model.nav.sub,
            .itemChanged = step.state.item != model.nav.item, .variantChanged = step.state.variant != model.nav.variant && step.state.item == model.nav.item,
            .focusChanged = step.state.focus != model.nav.focus, .activate = step.activate, .back = step.back, .toggleMode = (interactive && input.toggleMode) || modeClick};
    }
}
