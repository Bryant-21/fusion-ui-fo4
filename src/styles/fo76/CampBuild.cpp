#include "styles/fo76/CampBuild.h"
#include "fusionui/Icons.h"
#include "fusionui/fo76/Filters.h"
#include "fusionui/fo76/Theme.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

namespace fusionui::fo76::camp {
    namespace {
        using fo4::Align;

        // Roboto Condensed cap metrics (see fo76/Hud.cpp): text is placed by cap top and cap height.
        constexpr float kCapEm = 1456.0F / 2048.0F, kCapTopEm = (1900.0F - 1456.0F) / 2048.0F;
        constexpr ImU32 kInk = IM_COL32(30, 30, 30, 255);  // text on a selection
        constexpr ImU32 kPanel = IM_COL32(0, 0, 0, 205);   // 20 over FO76's grey stage
        constexpr ImU32 kCardBg = IM_COL32(0, 0, 0, 215);
        constexpr ImU32 kShadowInk = IM_COL32(0, 0, 0, 255);
        // Card row plates are neutral (untinted by the HUD colour): FO76's Yellow #49493F, Red #471F13 and Green
        // #005400 frames over the card's black.
        constexpr ImU32 kYellowPlate = IM_COL32(73, 73, 63, 255), kRedPlate = IM_COL32(44, 27, 22, 255), kGreenPlate = IM_COL32(0, 42, 0, 230);
        // The budget and message text's shadow: a hard black copy 3 units down-right.
        constexpr DropShadow kTextShadow{IM_COL32(0, 0, 0, 255), 45.0F, 4.243F, 0.0F, 1.0F};

        // Measured FO76 layout (16:9 frame; Layout moves the side panels for the other aspects).
        constexpr float kListX0 = 50, kListX1 = 329, kStripX1 = 95, kPanelTop = 30, kPanelBottom = 1010;
        constexpr float kRowsTop = 148, kRowsBottom = 980, kMainRow = 59, kSubRow = 45, kSubGap = 4;
        constexpr float kSubText = 122, kSubWrap = 200, kSubLine = 35;
        constexpr float kTileTop = 58, kTileSize = 100, kTilePitch = 102.75F;
        constexpr int kVisibleTiles = 9;
        // Tile labels and keyboard keys are Roboto (not condensed), em sizes from FO76's renders.
        constexpr float kTileLabelEm = 12.5F, kKeyEm = 26.0F;
        // VariantsPanel (LeftContainer + 152): 326 wide, a 99-unit header, then rows of 100-unit tiles 103 apart.
        constexpr float kVariantsX = 202, kVariantsW = 326, kVariantsTop = 58, kVariantsHeader = 99, kVariantPitch = 103;
        constexpr float kVariantGridX = 212, kVariantGridY = 161;
        // WorkshopItemCard: 290 wide (319 expanded, 23 further left), at most 941.8 tall.
        constexpr float kCardX0 = 1580, kCardW = 290, kCardExpandedW = 319, kCardExpandedDx = -23, kCardPad = 10;
        constexpr float kCardMaxH = 941.8F, kCardSpacing = 14, kCardRow = 36, kCardRowH = 33;
        constexpr float kBudgetX = 785, kBudgetW = 350;
        constexpr float kQuickRow = 41.5F, kQuickRowH = 38;
        constexpr float kBlueprintX = 780, kBlueprintW = 397, kBlueprintTop = 221.5F, kBlueprintBottom = 858, kBlueprintRow = 73;
        constexpr int kBlueprintRows = 7;

        ImU32 Fade(ImU32 color, float alpha) {
            const auto a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(alpha, 0.0F, 1.0F);
            return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT);
        }
        ImU32 Mix(ImU32 a, ImU32 b, float t) {
            const auto ch = [&](int shift) {
                const float x = static_cast<float>((a >> shift) & 0xFF), y = static_cast<float>((b >> shift) & 0xFF);
                return static_cast<ImU32>(x + (y - x) * t + 0.5F) << shift;
            };
            return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | ch(IM_COL32_A_SHIFT);
        }

        std::string Upper(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }

        void Caps(float x, float capTop, float capHeight, std::string_view text, ImU32 color, bool bold = false,
                  Align align = Align::Left, float spacing = 0.0F) {
            const float size = capHeight / kCapEm;
            fo4::Text(x, capTop - kCapTopEm * size, size, text, color, bold, spacing, align);
        }
        float CapsWidth(float capHeight, std::string_view text, bool bold = false) { return fo4::TextWidth(capHeight / kCapEm, text, bold); }
        ImFont* Wide() { return common::CurrentFonts().wide; }
        // Roboto text by cap top, em size `em`.
        void WideText(float x, float capTop, float em, std::string_view text, ImU32 color, Align align = Align::Left) {
            fo4::TextIn(Wide(), x, capTop - kCapTopEm * em, em, text, color, align);
        }
        void ShadowCaps(float x, float capTop, float capHeight, std::string_view text, ImU32 color, Align align = Align::Left) {
            const float unit = fo4::Px(1.0F);
            ShadowLayers(kTextShadow, 1.0F, [&](ImVec2 offset, ImU32 tint) {
                Caps(x + offset.x / unit, capTop + offset.y / unit, capHeight, text, tint, true, align);
            });
            Caps(x, capTop, capHeight, text, color, true, align);
        }

        // Greedy word wrap into at most `lines` lines of `width` stage units.
        std::vector<std::string> Wrap(std::string_view text, float capHeight, float width, bool bold, int lines) {
            std::vector<std::string> out;
            std::string line;
            std::size_t at = 0;
            while (at < text.size()) {
                const auto space = text.find(' ', at);
                const auto word = text.substr(at, space == std::string_view::npos ? std::string_view::npos : space - at);
                at = space == std::string_view::npos ? text.size() : space + 1;
                const auto candidate = line.empty() ? std::string(word) : line + " " + std::string(word);
                if (!line.empty() && CapsWidth(capHeight, candidate, bold) > width && static_cast<int>(out.size()) + 1 < lines) {
                    out.push_back(line);
                    line = std::string(word);
                } else {
                    line = candidate;
                }
            }
            if (!line.empty() || out.empty()) out.push_back(line);
            return out;
        }

        // Boxes are [x0, x1) x [y0, y1) in stage units.
        void Box(float x0, float y0, float x1, float y1, ImU32 color) { fo4::Canvas()->AddRectFilled(fo4::Stage(x0, y0), fo4::Stage(x1, y1), color); }
        void Line(float x0, float y0, float x1, float y1, ImU32 color, float width = 2) {
            fo4::Canvas()->AddLine(fo4::Stage(x0, y0), fo4::Stage(x1, y1), color, fo4::Px(width));
        }
        void Icon(std::string_view key, float cx, float cy, float size, ImU32 tint) {
            icons::Draw(fo4::Canvas(), key, fo4::Stage(cx, cy), fo4::Px(size), tint);
        }
        // FO76's tile badges are opaque with a black 2-unit stroke and a 90% black drop shadow (shape 83's layers).
        void BadgeIcon(std::string_view key, float cx, float cy, float size, ImU32 tint) {
            const auto ink = IM_COL32(0, 0, 0, 230);
            Icon(key, cx + 2, cy + 2, size, ink);
            for (const float dx : {-1.0F, 1.0F})
                for (const float dy : {-1.0F, 1.0F}) Icon(key, cx + dx, cy + dy, size, ink);
            Icon(key, cx, cy, size, tint);
        }

        // FO76's selection: accent fill lightening toward the left, 2-unit accent border.
        void Selection(float x0, float y0, float x1, float y1, ImU32 accent, ImU32 light) {
            Box(x0, y0, x1, y1, accent);
            const auto mid = Mix(accent, light, 0.5F);
            fo4::Canvas()->AddRectFilledMultiColor(fo4::Stage(x0 + 2, y0 + 2), fo4::Stage(x1 - 2, y1 - 2), mid, accent, accent, mid);
        }
        void Selection(float x0, float y0, float x1, float y1, const Palette& p) { Selection(x0, y0, x1, y1, p.accent, p.text); }

        // Horizontal rule fading out to the right (under the list header and the card title).
        void Rule(float x0, float x1, float y, float height, ImU32 color) {
            fo4::Canvas()->AddRectFilledMultiColor(fo4::Stage(x0, y), fo4::Stage(x1, y + height), color, Fade(color, 0.0F), Fade(color, 0.0F), color);
        }

        void Chevron(float cx, float top, bool up, ImU32 color) {
            const float y0 = up ? top + 13 : top, y1 = up ? top : top + 13;
            auto* list = fo4::Canvas();
            list->PathLineTo(fo4::Stage(cx - 9, y0));
            list->PathLineTo(fo4::Stage(cx, y1));
            list->PathLineTo(fo4::Stage(cx + 9, y0));
            list->PathStroke(color, 0, fo4::Px(5));
        }

        // FO76's missing-image glyph: corner arrows, edge dashes and a centre cross.
        void Placeholder(float cx, float cy, ImU32 color) {
            constexpr float c = 40, leg = 9, diag = 17, dash = 10, cross = 9;
            for (const float sx : {-1.0F, 1.0F})
                for (const float sy : {-1.0F, 1.0F}) {
                    const float x = cx + sx * c, y = cy + sy * c;
                    Line(x, y, x - sx * diag, y - sy * diag, color);
                    Line(x, y, x - sx * leg, y, color);
                    Line(x, y, x, y - sy * leg, color);
                }
            for (const float s : {-1.0F, 1.0F}) {
                Line(cx + s * c, cy - dash, cx + s * c, cy + dash, color);
                Line(cx - dash, cy + s * (c - 2), cx + dash, cy + s * (c - 2), color);
            }
            Line(cx - cross, cy - cross, cx + cross, cy + cross, color);
            Line(cx - cross, cy + cross, cx + cross, cy - cross, color);
        }

        bool Hovered(float x0, float y0, float x1, float y1) {
            const auto a = fo4::Stage(x0, y0), b = fo4::Stage(x1, y1);
            return ImGui::IsMouseHoveringRect(a, b, false);
        }
        bool Clicked(float x0, float y0, float x1, float y1) { return ImGui::IsMouseClicked(ImGuiMouseButton_Left) && Hovered(x0, y0, x1, y1); }

        // A sub-category label as its row shows it: wrapped to two lines when long.
        std::vector<std::string> SubLines(std::string_view label) { return Wrap(label, 20, kSubWrap, true, 2); }
        float SubHeight(std::string_view label) { return SubLines(label).size() > 1 ? kSubRow + kSubLine - 6 : kSubRow; }

        // The movie's two-line header: either label overflows its text field (BUILD 93 units, MODIFY 140).
        bool TwoLineHeader(const Model& m) { return CapsWidth(27, m.buildLabel, true) > 93 || CapsWidth(27, m.modifyLabel, true) > 140; }

        void ModeKey(const Model& m, float x, float capTop, ImU32 color) {
            if (m.gamepad && m.modePad != kNoPad) common::PadGlyph(fo4::Canvas(), fo4::Stage(x + 14, capTop + 9), fo4::Px(13), m.modePad, color);
            else WideText(x, capTop + 2, kKeyEm, m.modeKey, color);
        }

        // Category list. `expanded`: the full panel with labels; else the icon strip beside the tiles.
        // Returns a clicked row (category, sub) or nothing.
        std::optional<core::camp::Row> Categories(const Model& m, const core::camp::NavShape& shape, bool expanded, float dx) {
            const auto& p = m.palette;
            const float x0 = kListX0 + dx, x1 = (expanded ? kListX1 : kStripX1) + dx;
            Box(x0, kPanelTop, x1, kPanelBottom, kPanel);
            const bool twoLine = TwoLineHeader(m);
            if (expanded) {
                const auto build = m.modify ? Fade(p.accent, 0.3F) : p.accent, modify = m.modify ? p.accent : Fade(p.accent, 0.3F);
                if (twoLine) {
                    ModeKey(m, x0 + 13, 62, p.text);
                    Caps(x0 + 50, 39, 25, m.buildLabel, build, true);
                    Caps(x0 + 50, 81, 25, m.modifyLabel, modify, true);
                } else {
                    ModeKey(m, x0 + 13, 52, p.text);
                    const float buildW = CapsWidth(27, m.buildLabel, true);
                    Caps(x0 + 50, 48, 27, m.buildLabel, build, true);
                    Caps(x0 + 50 + buildW + 12, 48, 27, m.modifyLabel, modify, true);
                }
                Rule(x0, x1, twoLine ? 117.0F : 96.0F, 2, Fade(p.accent, 0.5F));
                Chevron(x0 + 140, 128, true, Fade(p.accent, 0.3F));
                Chevron(x0 + 140, 990, false, Fade(p.accent, 0.3F));
            } else {
                ModeKey(m, x0 + 13, 61, p.text);
                Rule(x0, x1, 117, 2, Fade(p.accent, 0.5F));
            }
            const auto rows = core::camp::RowsOf(shape, m.nav.category, m.nav.sub >= 0);
            std::optional<core::camp::Row> clicked;
            fo4::Canvas()->PushClipRect(fo4::Stage(x0, kRowsTop), fo4::Stage(x1, kRowsBottom + 2), true);
            float y = kRowsTop;
            for (int i = expanded ? m.nav.firstRow : 0; i < static_cast<int>(rows.size()) && y < kRowsBottom; ++i) {
                const auto row = rows[i];
                if (!expanded && row.sub >= 0) continue;
                const auto& category = m.categories[row.category];
                const bool main = row.sub < 0;
                const float height = main ? kMainRow : core::camp::SubRowHeight(shape, row.category, row.sub) - kSubGap;
                const bool selected = row.category == m.nav.category && row.sub == m.nav.sub;
                const float alpha = category.enabled ? 1.0F : 0.35F;
                if (expanded && selected) Selection(x0, y, x1, y + height, p);
                if (main) {
                    const auto tint = expanded ? (selected ? kInk : p.accent) : (row.category == m.nav.category ? p.accent : Fade(p.accent, 0.3F));
                    Icon(category.icon, x0 + 21.5F, y + 28.5F, 24, Fade(tint, alpha));
                    if (expanded) {
                        // A long mod name shrinks to the panel rather than running under the tiles.
                        const auto label = Upper(category.label);
                        const float room = x1 - x0 - 50, width = CapsWidth(20, label, true);
                        const float size = width > room ? std::max(12.0F, 20 * room / width) : 20.0F;
                        Caps(x0 + 40, y + 19 + (20 - size) / 2, size, label, Fade(selected ? kInk : p.text, alpha), true);
                    }
                } else {
                    float line = y + 12;
                    for (const auto& text : SubLines(category.subs[row.sub])) {
                        Caps(x0 + kSubText - kListX0, line, 20, text, Fade(selected ? kInk : p.text, alpha), true);
                        line += kSubLine;
                    }
                }
                if (Clicked(x0, y, x1, y + height)) clicked = row;
                y += main ? kMainRow : height + kSubGap;
            }
            fo4::Canvas()->PopClipRect();
            return clicked;
        }

        // FO76's adornments: the variant indicator top-left, the restriction badge and the premium icon
        // bottom-right (the badge moves up when both show).
        void Adornments(const Tile& tile, float x0, float y0, const Palette& p) {
            if (tile.hasVariants) Icon("camp.badge.variants", x0 + 9, y0 + 9.5F, 15, p.text);
            const bool premium = tile.premium != Premium::None;
            const auto badge = p.hostile;
            switch (tile.state) {
            case TileState::MissingMaterials: BadgeIcon("camp.badge.missing", x0 + 80.5F, y0 + (premium ? 52 : 82), 26, badge); break;
            case TileState::Unavailable: BadgeIcon("camp.badge.unavailable", x0 + 80.5F, y0 + (premium ? 52 : 80.5F), 27, badge); break;
            case TileState::Locked: BadgeIcon("camp.badge.locked", x0 + 83.5F, y0 + 49.5F, 27, badge); break;
            case TileState::Buildable: break;
            }
            if (premium)
                Icon(tile.premium == Premium::AtomicShop ? "camp.badge.premium" : "camp.badge.fo1st", x0 + 80, y0 + 81, 24,
                     tile.entitled ? p.accent : p.hostile);
        }

        // One tile: plate or selection, thumbnail or placeholder, label and adornments. `gray`: FO76's
        // GrayedOut / RollOnGray while the variant grid has focus.
        void DrawTile(const Tile& tile, float x0, float y, bool selected, bool gray, bool label, const Palette& p) {
            const float cx = x0 + kTileSize / 2, x1 = x0 + kTileSize, y1 = y + kTileSize;
            const auto grey = IM_COL32(150, 150, 150, 255);
            if (selected) Selection(x0, y, x1, y1, gray ? Mix(p.accent, grey, 0.6F) : p.accent, gray ? Mix(p.text, grey, 0.6F) : p.text);
            else Box(x0, y, x1, y1, kPanel);
            const float dim = gray && !selected ? 0.45F : 1.0F;
            if (tile.thumbnail) {
                const auto tint = tile.state == TileState::Buildable ? IM_COL32_WHITE : grey;
                fo4::Canvas()->AddImage(tile.thumbnail, fo4::Stage(x0 + 4, y + 4), fo4::Stage(x1 - 4, y1 - 4), {0, 0}, {1, 1}, Fade(tint, dim));
            } else {
                Placeholder(cx, y + 50, selected ? Fade(kInk, 0.55F) : Fade(p.text, 0.3F * dim));
            }
            if (label) {
                fo4::Canvas()->PushClipRect(fo4::Stage(x0, y), fo4::Stage(x1, y1), true);
                WideText(x0 + 3, y + 6, kTileLabelEm, tile.name, Fade(IM_COL32(254, 254, 254, 255), dim));
                fo4::Canvas()->PopClipRect();
            }
            Adornments(tile, x0, y, p);
        }

        // Tile column at x0. Returns a clicked tile or -1.
        int Tiles(const Model& m, float x0, bool focused, bool gray) {
            const auto& p = m.palette;
            const float cx = x0 + kTileSize / 2;
            if (m.items.empty()) return -1;
            Chevron(cx, 40, true, Fade(p.accent, 0.3F));
            Chevron(cx, 987, false, Fade(p.accent, 0.3F));
            int clicked = -1;
            const int last = std::min(static_cast<int>(m.items.size()), m.nav.firstItem + kVisibleTiles);
            for (int i = m.nav.firstItem; i < last; ++i) {
                const float y = kTileTop + kTilePitch * static_cast<float>(i - m.nav.firstItem);
                DrawTile(m.items[i], x0, y, focused && i == m.nav.item, gray, true, p);
                if (Clicked(x0, y, x0 + kTileSize, y + kTileSize)) clicked = i;
            }
            return clicked;
        }

        std::string Count(std::string format, int index, int count) {
            for (const auto& [token, value] : {std::pair{"{1}", index}, std::pair{"{2}", count}})
                if (const auto at = format.find(token); at != std::string::npos) format.replace(at, 3, std::to_string(value));
            return format;
        }

        // FO76's VariantsPanel beside the tile column: the group (or category) name, the item and variant
        // counts, and the 3-wide grid. Returns a clicked variant or -1.
        int Variants(const Model& m, float dx) {
            const auto& p = m.palette;
            const bool grid = m.nav.focus == Focus::Variants;
            const int count = static_cast<int>(m.variants.size());
            const bool hasGrid = count > 1;
            const int cols = 3, rows = (count + cols - 1) / cols;
            const int shown = hasGrid ? std::min(rows, m.variantsMinimized ? 1 : 8) : 0;
            const float x0 = kVariantsX + dx, top = kVariantsTop;
            const float bottom = top + kVariantsHeader + kVariantPitch * static_cast<float>(shown);
            Box(x0, top, x0 + kVariantsW, bottom, kPanel);
            Chevron(x0 + kVariantsW / 2, top - 18, true, Fade(p.accent, 0.3F));
            Chevron(x0 + kVariantsW / 2, bottom + 4, false, Fade(p.accent, 0.3F));
            const float tx = x0 + 12;
            const auto variantCount = hasGrid ? Count(m.variantCount, std::max(grid ? m.nav.variant + 1 : 1, 1), count) : m.variantNone;
            if (grid) {
                // Header "Default": the group's name, then the variant counter.
                if (!m.variantGroup.empty()) Caps(tx, 77, 20, m.variantGroup, p.text, true);
                Caps(tx, 115, 20, variantCount, Fade(p.text, 0.55F));
            } else {
                // Header "Item_Default": the category, "Floors: 3/12", then the variant counter.
                const auto& category = m.categories.empty() ? Category{} : m.categories[std::clamp(m.nav.category, 0, static_cast<int>(m.categories.size()) - 1)];
                const auto& entry = m.nav.sub >= 0 && m.nav.sub < static_cast<int>(category.subs.size()) ? category.subs[m.nav.sub] : category.label;
                const auto items = m.items.empty() ? m.none : std::format("{}/{}", std::clamp(m.nav.item, 0, static_cast<int>(m.items.size()) - 1) + 1, m.items.size());
                Caps(tx, 66, 20, category.label, p.text, true);
                Caps(tx, 96, 20, std::format("{}: {}", entry, items), Fade(p.text, 0.55F));
                if (!variantCount.empty()) Caps(tx, 128, 20, variantCount, Fade(p.text, 0.55F));
            }
            int clicked = -1;
            if (!hasGrid) return clicked;
            const int firstRow = m.variantsMinimized ? m.nav.variant / cols : std::clamp(m.nav.firstVariantRow, 0, std::max(0, rows - shown));
            for (int r = 0; r < shown; ++r)
                for (int c = 0; c < cols; ++c) {
                    const int i = (firstRow + r) * cols + c;
                    if (i >= count) break;
                    const float x = kVariantGridX + dx + kVariantPitch * static_cast<float>(c), y = kVariantGridY + kVariantPitch * static_cast<float>(r);
                    DrawTile(m.variants[i], x, y, grid && i == m.nav.variant, false, false, p);
                    if (Clicked(x, y, x + kTileSize, y + kTileSize)) clicked = i;
                }
            return clicked;
        }

        // FO76's Budget_Container: built fill from the left, the stored meter from the right end, "[n% STORED]".
        // BudgetBlueprint_Container replaces it while a blueprint is being placed.
        void Budget(const Model& m) {
            const auto& p = m.palette;
            const bool blueprint = m.blueprintBudget >= 0;
            ShadowCaps(783, 34, 27, Upper(blueprint ? m.blueprintBudgetLabel : m.budgetLabel), p.accent);
            if (!blueprint) {
                const auto percent = static_cast<int>(std::floor(std::clamp(m.budgetStored, 0.0F, 1.0F) * 100.0F));
                ShadowCaps(1136, 38, 22, std::format("[{}% {}", percent, Upper(m.storedLabel)), p.text, Align::Right);
            }
            Box(784, 73, 1140, 91, kShadowInk);
            Box(782, 71, 1138, 89, blueprint ? p.blueprint : Mix(IM_COL32(30, 30, 30, 255), p.accent, 0.6F));
            Box(783, 72, 1137, 88, IM_COL32(30, 30, 30, 255));
            const float fill = std::clamp(blueprint ? m.blueprintBudget : m.budget, 0.0F, 1.0F);
            if (fill > 0) Box(kBudgetX, 74, kBudgetX + kBudgetW * fill, 86, p.text);
            const float stored = blueprint ? 0.0F : std::clamp(m.budgetStored, 0.0F, 1.0F);
            if (stored > 0) Box(kBudgetX + kBudgetW * (1 - stored), 74, kBudgetX + kBudgetW, 86, p.stored);
            if (!m.message.empty()) ShadowCaps(960, 112, 20, m.message, p.accent, Align::Center);
        }

        // The card's content as FO76's refreshData lays it out, in card-local y (0 = the card's top).
        struct CardLine {
            enum class Kind { Banner, Name, Count, Rule, Header, Row, Text } kind;
            float y;
            std::string text, value;
            const CardRow* row = nullptr;
        };
        struct CardLayout {
            std::vector<CardLine> lines;
            float height = 0;      // shown background height
            float total = 0;       // content height (scrolls when expanded)
            bool expandable = false;
        };

        CardLayout LayCard(const Card& c, bool expanded) {
            CardLayout out;
            auto& lines = out.lines;
            float y = 0;
            if (!c.banner.empty()) {
                lines.push_back({CardLine::Kind::Banner, 18, c.banner});
                y = 44;
            }
            lines.push_back({CardLine::Kind::Name, y + 18, c.name});
            y += 56;
            float count = y + 9;
            if (!c.built.empty()) {
                lines.push_back({CardLine::Kind::Count, count, c.builtLabel, c.built});
                count += 49;
            }
            if (c.stored > 0) {
                lines.push_back({CardLine::Kind::Count, count, c.storedLabel, std::to_string(c.stored)});
                count += 49;
            }
            if (count > y + 9) y = count - 9;
            lines.push_back({CardLine::Kind::Rule, y});
            float bottom = y + 13;
            y += kCardSpacing;
            static const CardRow kMore{};
            for (const auto& section : c.sections) {
                if (section.rows.empty()) continue;
                lines.push_back({CardLine::Kind::Header, y + 8, section.title});
                float row = y + 8 + 30;
                std::size_t limit = section.rows.size();
                if (section.fits && !expanded) {
                    const auto fit = static_cast<std::size_t>(std::max(1.0F, std::floor((kCardMaxH - row - kCardSpacing) / kCardRow)));
                    if (limit > fit) {
                        limit = fit;
                        out.expandable = true;
                    }
                }
                for (std::size_t i = 0; i < limit; ++i) {
                    const bool more = limit < section.rows.size() && i + 1 == limit;
                    lines.push_back({CardLine::Kind::Row, row, more ? c.moreLabel : section.rows[i].label, more ? "+++" : section.rows[i].value,
                                     more ? &kMore : &section.rows[i]});
                    row += kCardRow;
                }
                bottom = row + 15;
                y = row + kCardSpacing;
            }
            if (!c.description.empty()) {
                const auto wrapped = Wrap(c.description, 17, (expanded ? kCardExpandedW : kCardW) - 2 * kCardPad - 6, false, 1000);
                bool cut = false;
                for (std::size_t i = 0; i < wrapped.size(); ++i) {
                    const float line = y + 4 + 25.0F * static_cast<float>(i);
                    if (!expanded && line + 25 > kCardMaxH) {
                        if (!lines.empty() && lines.back().kind == CardLine::Kind::Text) lines.back().text += "...";
                        cut = true;
                        break;
                    }
                    lines.push_back({CardLine::Kind::Text, line, wrapped[i]});
                }
                bottom = y + 4 + 25.0F * static_cast<float>(wrapped.size()) + kCardSpacing;
                out.expandable = out.expandable || cut;
            }
            out.total = bottom;
            out.height = expanded ? kCardMaxH : std::min(bottom, kCardMaxH);
            out.expandable = out.expandable || bottom > kCardMaxH;
            return out;
        }

        void ItemCard(const Model& m, float dx) {
            const auto& c = m.card;
            const auto& p = m.palette;
            if (c.name.empty()) return;
            const bool expanded = m.cardExpanded;
            const auto layout = LayCard(c, expanded);
            const float left = kCardX0 + dx + (expanded ? kCardExpandedDx : 0.0F), width = expanded ? kCardExpandedW : kCardW;
            const float x0 = left + kCardPad, x1 = left + width - kCardPad - (expanded ? 12.0F : 0.0F);
            const float scroll = expanded ? std::clamp(m.cardScroll, 0.0F, std::max(0.0F, layout.total - layout.height)) : 0.0F;
            Box(left, kPanelTop, left + width, kPanelTop + layout.height, kCardBg);
            fo4::Canvas()->PushClipRect(fo4::Stage(left, kPanelTop), fo4::Stage(left + width, kPanelTop + layout.height), true);
            std::vector<float> tagged;  // drawn outside the card, after the clip
            for (const auto& line : layout.lines) {
                const float y = kPanelTop + line.y - scroll;
                switch (line.kind) {
                case CardLine::Kind::Banner: Caps((x0 + x1) / 2, y, 19, line.text, p.hostile, true, Align::Center); break;
                case CardLine::Kind::Name: Caps(x0, y, 19, line.text, p.accent, true); break;
                case CardLine::Kind::Count:
                    Caps(x0, y, 19, line.text, p.accent);
                    Caps(x1 - 1, y, 19, line.value, p.accent, false, Align::Right);
                    break;
                case CardLine::Kind::Rule: Rule(left, left + width - 4, y, 3, Fade(p.accent, 0.5F)); break;
                case CardLine::Kind::Header: Caps(x0, y, 19, line.text, p.accent, true); break;
                case CardLine::Kind::Row: {
                    const auto& r = *line.row;
                    // FO76's frames: Yellow (#49493F plate), Red (#471F13 at 50 %), Green (#005400 at 50 %).
                    const auto plate = r.missing ? kRedPlate : r.publicMaterials ? kGreenPlate : kYellowPlate;
                    Box(x0, y, x1, y + kCardRowH, plate);
                    const auto ink = r.missing ? p.hostile : r.publicMaterials ? p.friendly : p.text;
                    float labelX = x0 + 4;
                    if (r.transferLocked) {
                        Icon("camp.card.transferlock", x0 + 14.9F, y + 15.7F, 18, ink);
                        labelX = x0 + 26;
                    }
                    if (!r.icon.empty()) {
                        Icon(r.icon, labelX + 8, y + 16.5F, 20, ink);
                        labelX += 20;
                    }
                    Caps(labelX, y + 7, 19.5F, line.text, ink);
                    Caps(x1 - 6, y + 7, 19.5F, line.value, ink, false, Align::Right);
                    if (r.tagged && y + kCardRowH <= kPanelTop + layout.height) tagged.push_back(y);
                    break;
                }
                case CardLine::Kind::Text: Caps(x0, y, 17, line.text, p.text); break;
                }
            }
            fo4::Canvas()->PopClipRect();
            for (const float y : tagged) {
                // Tag for Search: FO76's magnifier left of the row (x -38), with its hard shadow.
                const float unit = fo4::Px(1.0F);
                ShadowLayers(kHard, 1.0F, [&](ImVec2 offset, ImU32 tint) {
                    Icon("camp.card.tagged", x0 - 26 + offset.x / unit, y + 20 + offset.y / unit, 24, tint);
                });
                Icon("camp.card.tagged", x0 - 26, y + 20, 24, p.text);
            }
            if (expanded && layout.total > layout.height) {
                // ScrollVert: a track and a thumb sized by the visible share.
                const float tx = left + width - 12, track0 = kPanelTop + 6, track1 = kPanelTop + layout.height - 6;
                Box(tx, track0, tx + 4, track1, Fade(p.accent, 0.25F));
                const float share = layout.height / layout.total, span = (track1 - track0) * share;
                const float at = track0 + (track1 - track0 - span) * (scroll / (layout.total - layout.height));
                Box(tx - 1, at, tx + 5, at + span, p.accent);
            }
        }

        // FO76's QuickMenuPanel: rows 41.5 apart, the selected one on FO76's selection, the backer as wide
        // as the widest label + 40.
        void QuickMenuPanel(const Model& m, ImVec2 at) {
            const auto& q = m.quickMenu;
            const auto& p = m.palette;
            if (q.options.empty()) return;
            float widest = 0;
            for (const auto& o : q.options) widest = std::max(widest, CapsWidth(20, o.label));
            const float x = at.x, y = at.y, rows = static_cast<float>(q.options.size());
            Box(x - 9, y - 8, x - 9 + widest + 40, y + 4 + kQuickRow * rows + 4, IM_COL32(0, 0, 0, 150));
            for (std::size_t i = 0; i < q.options.size(); ++i) {
                const auto& o = q.options[i];
                const float top = y + 4 + kQuickRow * static_cast<float>(i);
                const bool selected = static_cast<int>(i) == q.selected;
                if (selected) Selection(x - 1, top, x - 1 + widest + 20, top + kQuickRowH, p);
                else Box(x - 1, top, x - 1 + widest + 20, top + kQuickRowH, Fade(p.accent, 0.12F));
                const auto ink = selected ? kInk : p.accent;
                Caps(x + 3, top + 10, 20, o.label, o.enabled ? ink : Fade(ink, 0.4F));
            }
        }

        // FO76's SaveBlueprint_mc: a titled one-column list, seven rows of 356 x 68 entries.
        void BlueprintSaveList(const Model& m) {
            const auto& b = m.blueprints;
            const auto& p = m.palette;
            if (b.rows.empty()) return;
            constexpr float x0 = kBlueprintX, x1 = kBlueprintX + kBlueprintW;
            Box(x0, kBlueprintTop, x1, kBlueprintBottom, IM_COL32(0, 0, 0, 240));
            Caps((x0 + x1) / 2, 241, 28, Upper(b.title), p.accent, true, Align::Center);
            Rule(x0, x1 - 30, 287, 2, Fade(p.accent, 0.5F));
            const int count = static_cast<int>(b.rows.size());
            const int selected = std::clamp(b.selected, 0, count - 1);
            const int first = std::clamp(selected - kBlueprintRows + 1, 0, std::max(0, count - kBlueprintRows));
            if (first > 0) Chevron((x0 + x1) / 2, 300, true, Fade(p.accent, 0.5F));
            if (first + kBlueprintRows < count) Chevron((x0 + x1) / 2, 834, false, Fade(p.accent, 0.5F));
            for (int i = first; i < std::min(count, first + kBlueprintRows); ++i) {
                const float top = 322 + kBlueprintRow * static_cast<float>(i - first), rx = x0 + 10;
                const bool on = i == selected;
                if (on) Selection(rx, top, rx + 356, top + 68, p);
                else Box(rx, top, rx + 356, top + 68, IM_COL32(52, 52, 45, 255));
                Box(rx + 2, top + 2, rx + 66, top + 66, IM_COL32(28, 74, 92, 255));
                Icon("camp.category.blueprint", rx + 34, top + 34, 44, IM_COL32(170, 220, 235, 255));
                Caps(rx + 73, top + 24, 20, b.rows[i], on ? kInk : p.text);
            }
        }

        void HintBar(const Model& m) {
            const auto& hints = m.hints;
            if (hints.empty()) return;
            constexpr float kKeyGap = 7, kHintGap = 28, kPad = 8, kLabelCap = 20, kGlyph = 13;
            const auto pad = [&](const Hint& h) { return m.gamepad && h.padButton != kNoPad; };
            const auto keyWidth = [&](const Hint& h) {
                return pad(h) ? common::PadGlyphWidth(fo4::Px(kGlyph), h.padButton) / fo4::Px(1.0F) : fo4::TextWidthIn(Wide(), kKeyEm, h.key);
            };
            float width = 2 * kPad - kHintGap;
            for (const auto& h : hints) width += keyWidth(h) + kKeyGap + CapsWidth(kLabelCap, Upper(h.label), true) + kHintGap;
            float x = 960 - width / 2;
            Box(x, 1027, x + width, 1069, IM_COL32(0, 0, 0, 102));
            x += kPad;
            const auto hud = common::HudColor();
            for (const auto& h : hints) {
                const auto ink = h.enabled ? hud : Fade(hud, 0.5F);
                const float kw = keyWidth(h);
                if (pad(h)) common::PadGlyph(fo4::Canvas(), fo4::Stage(x + kw / 2, 1048), fo4::Px(kGlyph), h.padButton, ink);
                else WideText(x, 1039, kKeyEm, h.key, ink);
                x += kw + kKeyGap;
                const auto label = Upper(h.label);
                Caps(x, 1036, kLabelCap, label, ink, true);
                x += CapsWidth(kLabelCap, label, true) + kHintGap;
            }
        }
    }

    Input ReadInput() {
        const auto pressed = [](std::initializer_list<ImGuiKey> keys) {
            return std::ranges::any_of(keys, [](ImGuiKey k) { return ImGui::IsKeyPressed(k, false); });
        };
        Input in{.nav = fo4::ReadNav()};
        if (pressed({ImGuiKey_Z})) in.cycle -= 1;
        if (pressed({ImGuiKey_C, ImGuiKey_GamepadL1})) in.cycle += 1;
        in.toggleMode = pressed({ImGuiKey_Q, ImGuiKey_GamepadR1});
        return in;
    }

    core::camp::NavShape ShapeOf(const Model& m) {
        core::camp::NavShape shape;
        for (const auto& c : m.categories) {
            shape.subCounts.push_back(static_cast<int>(c.subs.size()));
            auto& heights = shape.subRows.emplace_back();
            for (const auto& sub : c.subs) heights.push_back(SubHeight(sub) + kSubGap);
        }
        shape.items = static_cast<int>(m.items.size());
        shape.visibleItems = kVisibleTiles;
        shape.rowsViewport = kRowsBottom - kRowsTop;
        shape.subRow = kSubRow + kSubGap;
        shape.variants = static_cast<int>(m.variants.size());
        shape.visibleVariantRows = m.variantsMinimized ? 1 : 8;
        shape.selectedToTop = !m.modify;
        return shape;
    }

    Layout LayoutFor(float width, float height) {
        if (height <= 0 || width / height < 2.2F) return {};
        // workshopmenunew.swf's 21:9 frame (32:9 uses the same, centred).
        return {.left = -325.6F, .card = 323.7F, .quickMenu = {859.4F, 443.8F}};
    }

    float CardBottom(const Card& card, bool expanded) { return kPanelTop + LayCard(card, expanded).height; }

    std::pair<float, float> CardSpan(const Layout& layout, bool expanded) {
        const float left = kCardX0 + layout.card + (expanded ? kCardExpandedDx : 0.0F);
        return {left, left + (expanded ? kCardExpandedW : kCardW)};
    }

    bool CardExpandable(const Card& card) { return !card.name.empty() && LayCard(card, false).expandable; }
    float CardScrollLimit(const Card& card) { const auto layout = LayCard(card, true); return std::max(0.0F, layout.total - layout.height); }

    Palette Tinted(const Palette& p, ImU32 hud) {
        const auto scale = [&](ImU32 color) { return theme::HudTint(color, hud); };
        return {.text = scale(p.text), .accent = scale(p.accent), .hostile = scale(p.hostile), .stored = scale(p.stored),
                .blueprint = scale(p.blueprint), .friendly = scale(p.friendly)};
    }

    Result Menu(const Model& model, const Input& in) {
        // Draw what the owner's state settles to with the measured shape (wrapped rows, the variant grid), at the
        // player's HUD colour.
        auto shown = model;
        const auto shape = fusionui::fo76::camp::ShapeOf(model);
        shown.nav = core::camp::Step(model.nav, shape, {}).state;
        shown.palette = fusionui::fo76::camp::Tinted(model.palette, common::HudColor());
        const auto& m = shown;
        const auto& io = ImGui::GetIO();
        const auto layout = fusionui::fo76::camp::LayoutFor(io.DisplaySize.x, io.DisplaySize.y);

        const bool listFocus = m.nav.focus != Focus::Categories;
        const bool expanded = !listFocus || m.keepExpanded;
        const auto clickedRow = Categories(m, shape, expanded, layout.left);
        // FO76 has no item list while a main category with sub-categories is highlighted but not opened.
        const bool closedParent = m.nav.focus == Focus::Categories && m.nav.sub < 0 && m.nav.category < static_cast<int>(m.categories.size()) &&
                                  !m.categories[m.nav.category].subs.empty();
        int clickedTile = -1, clickedVariant = -1;
        if (!closedParent)
            clickedTile = Tiles(m, (expanded ? 333.0F : 99.0F) + layout.left, listFocus, m.nav.focus == Focus::Variants);
        if (listFocus && !expanded) clickedVariant = Variants(m, layout.left);
        Budget(m);
        HintBar(m);
        ItemCard(m, layout.card);
        BlueprintSaveList(m);
        QuickMenuPanel(m, layout.quickMenu);
        if (!m.cameraHelp.empty()) {
            const auto [left, right] = fusionui::fo76::camp::CardSpan(layout, m.cardExpanded);
            const float top = (m.card.name.empty() ? 30.0F : fusionui::fo76::camp::CardBottom(m.card, m.cardExpanded)) + 20;
            const float end = top + 16 + 34.0F * static_cast<float>(m.cameraHelp.size());
            fo4::Canvas()->AddRectFilled(fo4::Stage(left, top), fo4::Stage(right, end), IM_COL32(0, 0, 0, 215));
            for (std::size_t i = 0; i < m.cameraHelp.size(); ++i)
                fo4::Text(left + 20, top + 12 + 34.0F * static_cast<float>(i), 20, m.cameraHelp[i], m.palette.text);
        }
        if (m.cameraDot) {
            auto* list = ImGui::GetForegroundDrawList();
            const ImVec2 center{io.DisplaySize.x / 2, io.DisplaySize.y / 2};
            const float unit = io.DisplaySize.y / 1080;
            list->AddCircleFilled(center, 4 * unit, IM_COL32(0, 0, 0, 255));
            list->AddCircleFilled(center, 2.5F * unit, IM_COL32(255, 255, 255, 255));
        }

        core::camp::NavInput nav{.dy = in.nav.dy, .dx = in.nav.dx, .accept = in.nav.accept, .cancel = in.nav.cancel, .cycle = in.cycle};
        auto state = m.nav;
        if (clickedRow) {
            state.category = clickedRow->category;
            state.sub = clickedRow->sub;
            state.focus = Focus::Categories;
            if (state.category != m.nav.category || state.sub != m.nav.sub) state.item = state.firstItem = 0;
            nav = {};
        } else if (clickedTile >= 0) {
            nav = {.accept = m.nav.focus == Focus::Items && clickedTile == m.nav.item};
            state.item = clickedTile;
            state.focus = Focus::Items;
        } else if (clickedVariant >= 0) {
            nav = {.accept = m.nav.focus == Focus::Variants && clickedVariant == m.nav.variant};
            state.variant = clickedVariant;
            state.focus = Focus::Variants;
        }
        const auto step = core::camp::Step(state, shape, nav);
        return {.nav = step.state,
                .categoryChanged = step.state.category != model.nav.category || step.state.sub != model.nav.sub,
                .itemChanged = step.state.item != model.nav.item,
                .variantChanged = step.state.variant != model.nav.variant && step.state.item == model.nav.item,
                .focusChanged = step.state.focus != model.nav.focus,
                .activate = step.activate,
                .back = step.back,
                .toggleMode = in.toggleMode};
    }
}
