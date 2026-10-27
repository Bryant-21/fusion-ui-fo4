#include "styles/fo76/Inventory.h"
#include "fusionui/Icons.h"
#include "core/IconRuns.h"
#include "core/InventoryRows.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

// Numbers are FO76 stage units read from FO76's own movies (securetrade.swf, examinemenu.swf,
// newpipboy_invpage.swf, newpipboymenu.swf); see tmp/native-ui-audit/menus/findings.md.
namespace fusionui::fo76 {
    using fo4::Align;
    using fo4::Canvas;
    using fo4::Px;
    using fo4::Stage;
    using fo4::Text;
    using fo4::TextWidth;
    namespace inv = core::inventory;

    namespace {
        struct TradeInput { bool enabled{true}; };
        constexpr ImU32 kBlack = IM_COL32(0, 0, 0, 255);

        void Rect(float x0, float y0, float x1, float y1, ImU32 color) { Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), color); }

        // Pip-Boy art is authored white / grey and multiplied by the Pip-Boy colour.
        ImU32 Tint(ImU32 tint, float grey, float alpha = 1.0F) {
            const auto ch = [&](int shift) { return static_cast<ImU32>(static_cast<float>((tint >> shift) & 0xFF) * grey + 0.5F) << shift; };
            return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) |
                   static_cast<ImU32>(std::clamp(alpha, 0.0F, 1.0F) * 255.0F + 0.5F) << IM_COL32_A_SHIFT;
        }

        void Icon(std::string_view key, float cx, float cy, float size, ImU32 tint) {
            if (!key.empty()) icons::Draw(Canvas(), key, Stage(cx, cy), Px(size), tint);
        }

        // Stars left to right from `x`, centred on `cy`; returns the x after the last one.
        float Stars(int count, float x, float cy, float size, float pitch, ImU32 tint, bool shadow) {
            for (int i = 0; i < count; ++i) {
                const float cx = x + size / 2 + static_cast<float>(i) * pitch;
                if (shadow) ShadowIcon("legendary.star", cx, cy, size, tint);
                else Icon("legendary.star", cx, cy, size, tint);
            }
            return x + (count > 0 ? static_cast<float>(count - 1) * pitch + size : 0.0F);
        }
        float StarsWidth(int count, float size, float pitch) { return count > 0 ? static_cast<float>(count - 1) * pitch + size : 0.0F; }

        bool Hovered(float x0, float y0, float x1, float y1) { return ImGui::IsMouseHoveringRect(Stage(x0, y0), Stage(x1, y1)); }

        bool MouseMoved() {
            const auto d = ImGui::GetIO().MouseDelta;
            return d.x != 0.0F || d.y != 0.0F;
        }

        std::string Upper(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }

        // Eases a per-widget value toward `target` (the sliding tab selector and carousel).
        float Slide(ImGuiID id, float target) {
            auto* storage = ImGui::GetStateStorage();
            float* value = storage->GetFloatRef(id, target);
            const float dt = std::min(ImGui::GetIO().DeltaTime, 0.1F);
            *value += (target - *value) * std::min(1.0F, dt * 14.0F);
            if (std::abs(target - *value) < 0.25F) *value = target;
            return *value;
        }

        // Keyboard / pad / wheel / mouse handling of a list, shared by the trade panels and the Pip-Boy list.
        struct ListBox {
            float x0, x1, top, pitch, rowH;
            int rows;
        };
        ListResult ListInput(const ListBox& b, int count, ListModel& model, bool focused, const Nav& nav) {
            ListResult result;
            model.SetVisibleRows(b.rows);
            model.SetCount(count);
            const int before = model.Selected();
            if (!common::ContextSlot<TradeInput>().enabled) return result;
            if (focused) {
                if (nav.dy != 0) model.Move(nav.dy, false);
                if (nav.accept && model.Selected() >= 0) result.activated = model.Selected();
            }
            if (Hovered(b.x0, b.top, b.x1, b.top + static_cast<float>(b.rows) * b.pitch)) {
                const float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0F) model.Move(wheel > 0 ? -1 : 1, false);
            }
            for (int i = model.First(); i < model.Last(); ++i) {
                const float top = b.top + static_cast<float>(i - model.First()) * b.pitch;
                if (!Hovered(b.x0, top, b.x1, top + b.rowH)) continue;
                if (MouseMoved()) model.Select(i);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    model.Select(i);
                    result.activated = i;
                }
            }
            result.selectionChanged = model.Selected() != before;
            return result;
        }

        // ---- trade rows (PlayerListEntry / OfferListEntry) ---------------------------------------------
        // ItemList_mc sits 12.6 in and at y 299.6; a row's plate is 443 x 35.4 from (2.6, -8.5), 13 rows with
        // 6.5 between them. Text 24 at 19.7; price 24 bold right at 423.6, currency icon 420.5..442.9.
        constexpr float kListDx = 12.6F, kListY = 299.6F, kTradeRowH = 35.4F, kTradePitch = 41.9F;
        constexpr int kTradeRows = 13;
        constexpr float kPanelTop = 227.0F, kPanelBottom = 909.3F, kPanelW = 473.4F;

        float PanelX(Side side) { return side == Side::Player ? 131.2F : 1314.7F; }

        // Tales' optional list header strip takes the first row slot.
        bool HasHeader(const Panel& panel) { return !panel.headings.empty() || panel.search || !panel.pager.empty(); }

        ListBox TradeListBox(Side side, bool header = false) {
            const float x = PanelX(side) + kListDx;
            const float top = kListY - 8.5F + (header ? kTradePitch : 0.0F);
            return {x + 2.6F, x + 445.9F, top, kTradePitch, kTradeRowH, kTradeRows - (header ? 1 : 0)};
        }

        // Tales' columns: three cells right-aligned at the row's end (left of a barter price).
        constexpr float kCellW = 52.0F;
        float CellsRight(float listX, bool prices) { return listX + (prices ? 350.0F : 438.0F); }

        // FO76's list text shrinks to fit ("Shrink To Fit") before it is cut with "...".
        struct Fitted {
            std::string text;
            float size;
        };
        Fitted Fit(std::string_view s, float size, float width, bool bold) {
            constexpr float kMinScale = 0.72F;
            const float full = TextWidth(size, s, bold, -0.1F);
            if (full <= width) return {std::string(s), size};
            const float fitted = std::max(size * kMinScale, size * width / full);
            if (TextWidth(fitted, s, bold, -0.1F) <= width) return {std::string(s), fitted};
            std::string text(s);
            while (!text.empty() && TextWidth(fitted, text + "...", bold, -0.1F) > width) {
                text.pop_back();
                while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) text.pop_back();
                if (!text.empty() && static_cast<unsigned char>(text.back()) >= 0xC0) text.pop_back();
            }
            return {text + "...", fitted};
        }

        void TradeRow(float listX, float top, const Item& item, bool selected, bool prices, float width = 445.9F, float cellW = kCellW) {
            const float y0 = top, cy = top + kTradeRowH / 2;
            if (selected) SelectionPlate(listX + 2.6F, y0, listX + width, y0 + kTradeRowH);
            // PlayerListEntry: an item above the player's level stays COLOR_TEXT_UNAVAILABLE even when selected.
            const ImU32 ink = item.disabled ? theme::Unavailable : selected ? kBlack : theme::Hud(theme::Ink);
            const auto text = [&](float x, float size, std::string_view s, bool bold, Align align = Align::Left) {
                const float y = y0 + 2.5F + (24.0F - size) * 0.55F;
                if (selected) Text(x, y, size, s, ink, bold, -0.1F, align);
                else ShadowText(x, y, size, s, ink, bold, -0.1F, align);
            };
            const auto icon = [&](std::string_view key, float cx, float size, ImU32 tint) {
                if (selected) Icon(key, cx, cy, size, tint);
                else ShadowIcon(key, cx, cy, size, tint);
            };
            const inv::RowFlags flags{item.equipped, item.favorite, item.tagged, item.quest, item.setItem, item.setActive, item.locked};
            const auto lead = inv::LeadIcon(flags);
            if (lead == "item.equipped") Rect(listX + 8.7F, cy - 4.0F, listX + 16.6F, cy + 4.0F, ink);
            else icon(lead, listX + 12.6F, 16.0F, ink);

            // The right end: a barter price, then Tales' cells; the name gets what is left.
            float right = listX + width - 7.9F;
            float meterRight = listX + width - 27.9F;
            if (prices && item.value >= 0) {
                const auto price = std::to_string(item.value);
                text(listX + width - 27.9F, 24.0F, price, true, Align::Right);
                icon(item.currency, listX + width - 14.2F, 17.0F, ink);
                meterRight -= TextWidth(24.0F, price, true, -0.1F);
                right = meterRight - 10.0F;
            }
            if (!item.cells.empty()) {
                float cellRight = CellsRight(listX, prices) + width - 445.9F;
                for (std::size_t c = item.cells.size(); c-- > 0; cellRight -= cellW) {
                    const auto cell = Fit(item.cells[c], 20.0F, cellW - 6.0F, false);
                    text(cellRight, cell.size, cell.text, false, Align::Right);
                }
                right = meterRight = std::min(right, cellRight - 4.0F);
            }

            const auto label = inv::MakeRowLabel(item.name, item.count, item.legendaryStars);
            float x = listX + 19.7F;
            if (!item.iconRuns.empty() || !item.iconKey.empty()) {
                constexpr float kItemIcon = 28.0F;
                const ImU32 tint = item.iconTint != 0 && !selected ? item.iconTint : ink;
                if (!item.iconRuns.empty()) core::iconruns::DrawRuns(Canvas(), item.iconRuns, x + kItemIcon / 2, cy, kItemIcon, tint, Stage);
                else icon(item.iconKey, x + kItemIcon / 2, kItemIcon, tint);
                x += kItemIcon + 6.0F;
            }
            const auto trailing = inv::TrailingIcons(flags);
            const float tail = (label.stars > 0 ? 5.0F + StarsWidth(label.stars, 13.0F, 15.0F) : 0.0F) + (item.unique ? 20.0F : 0.0F) +
                               TextWidth(24.0F, label.suffix, false, -0.1F) + 15.0F + 24.0F * static_cast<float>(trailing.size());
            const auto name = Fit(label.name, 24.0F, std::max(24.0F, right - x - tail), false);
            text(x, name.size, name.text, false);
            x += TextWidth(name.size, name.text, false, -0.1F);
            if (label.stars > 0) x = Stars(label.stars, x + 5.0F, cy, 13.0F, 15.0F, ink, !selected);
            if (item.unique) {
                icon("item.unique", x + 12.0F, 16.0F, ink);
                x += 20.0F;
            }
            if (!label.suffix.empty()) {
                text(x, 24.0F, label.suffix, false);
                x += TextWidth(24.0F, label.suffix, false, -0.1F);
            }
            // PipBoyInvListEntry: icons start TEXT_PADDING (15) after the text, ICON_OFFSET (10) apart.
            x += 15.0F;
            for (const auto key : trailing) {
                icon(key, x + 7.0F, 14.0F, ink);
                x += 14.0F + 10.0F;
            }
            // ConditionBar_mc sits CONDITION_SPACING (6) left of the price. shortcut: the row symbol is in FO76's
            // shared library, so without a price the bar right-aligns to the price column.
            if (item.rowMeter >= 0.0F) ConditionMeter(meterRight - 6.0F - 160.0F, cy - 12.0F, item.rowMeter);
        }

        // Tales' header strip: the search field on the left, then "Columns n/m >", then the headings over the cells.
        void TradeHeader(Side side, const Panel& panel, bool prices, ListResult& result) {
            const float listX = PanelX(side) + kListDx, y0 = kListY - 8.5F, cy = y0 + kTradeRowH / 2;
            const ImU32 ink = theme::Hud(theme::Ink);
            const bool input = common::ContextSlot<TradeInput>().enabled;
            float searchRight = listX + 438.0F;
            if (!panel.headings.empty()) {
                float right = CellsRight(listX, prices);
                for (std::size_t c = panel.headings.size(); c-- > 0; right -= kCellW) {
                    const bool sorted = static_cast<int>(c) == panel.sortedHeading;
                    const bool hover = input && Hovered(right - kCellW, y0, right, y0 + kTradeRowH);
                    const auto label = Fit(panel.headings[c], 18.0F, kCellW - (sorted ? 16.0F : 4.0F), true);
                    const ImU32 color = hover || sorted ? theme::Hud(theme::Gold) : ink;
                    // Tracked like the tab bar: untracked, the T's bar runs into the W of "WT" at this size.
                    constexpr float kTrack = 0.7F;
                    ShadowText(right, cy - label.size * 0.6F, label.size, label.text, color, true, kTrack, Align::Right);
                    if (sorted) {
                        const float ax = right - TextWidth(label.size, label.text, true, kTrack) - 9.0F, h = panel.descending ? 4.0F : -4.0F;
                        Canvas()->AddTriangleFilled(Stage(ax - 5.0F, cy - h), Stage(ax + 5.0F, cy - h), Stage(ax, cy + h), color);
                    }
                    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) result.heading = static_cast<int>(c);
                }
                searchRight = right - 4.0F;
            }
            if (!panel.pager.empty()) {
                const float w = TextWidth(18.0F, panel.pager, true) + 8.0F;
                const bool hover = input && Hovered(searchRight - w, y0, searchRight, y0 + kTradeRowH);
                ShadowText(searchRight - 4.0F, cy - 11.0F, 18.0F, panel.pager, hover ? theme::Hud(theme::Gold) : ink, true, 0.0F, Align::Right);
                if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) result.pager = true;
                searchRight -= w + 6.0F;
            }
            if (!panel.search) return;
            // A black field framed in the text colour, as Tales' Flash search box.
            const float x0 = listX + 4.0F, x1 = std::max(x0 + 60.0F, searchRight), fy0 = y0 + 3.0F, fy1 = y0 + kTradeRowH - 3.0F;
            Rect(x0, fy0, x1, fy1, IM_COL32(0, 0, 0, 200));
            Canvas()->AddRect(Stage(x0, fy0), Stage(x1, fy1), ink, 0.0F, 0, std::max(1.0F, Px(1.5F)));
            auto& text = *panel.search;
            char buffer[97]{};
            text.copy(buffer, sizeof(buffer) - 1);
            ImGui::PushID(side == Side::Player ? "fo76-search-player" : "fo76-search-other");
            ImGui::PushFont(nullptr, Px(20.0F));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ink);
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme::Unavailable);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {Px(6.0F), std::max(0.0F, (Px(fy1 - fy0) - ImGui::GetFontSize()) / 2)});
            ImGui::SetCursorScreenPos(Stage(x0, fy0));
            ImGui::SetNextItemWidth(Px(x1 - x0));
            if (panel.focusSearch && input) ImGui::SetKeyboardFocusHere();
            ImGui::BeginDisabled(!input);
            if (ImGui::InputTextWithHint("##search", panel.searchHint.c_str(), buffer, sizeof(buffer), ImGuiInputTextFlags_EscapeClearsAll))
                text = buffer;
            ImGui::EndDisabled();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            ImGui::PopFont();
            ImGui::PopID();
        }

        void TradeFooter(Side side, const Panel& panel) {
            // PlayerWeight_tf / PlayerCurrency_tf: 21 bold, letter spacing 0.2, with their icons.
            const float x0 = PanelX(side);
            if (!panel.weight.empty()) {
                ShadowIcon("item.weight", x0 + 24.2F, 881.9F, 15.6F, theme::Hud(theme::Ink));
                ShadowText(x0 + 35.2F, 869.0F, 21.0F, panel.weight, theme::Hud(theme::Ink), true, 0.2F);
            }
            if (!panel.currencyAmount.empty()) {
                ShadowIcon(panel.currency, x0 + 446.8F, 883.0F, 24.0F, theme::Hud(theme::Ink));
                ShadowText(x0 + 430.6F, 869.0F, 21.0F, panel.currencyAmount, theme::Hud(theme::Ink), true, 0.2F, Align::Right);
            }
            if (!panel.footnote.empty())
                ShadowText(x0 + kPanelW / 2, 871.0F, 18.0F, panel.footnote, theme::Hud(theme::Ink), true, 0.2F, Align::Center);
        }

        // ---- item card (ItemCardContainer_mc / ItemCard) ----------------------------------------------
        // Rows 277 x 33.4 with no gap at 20 % #070707 inside a 289-wide box; a 37.7 header strip with
        // "ITEM STATS" (24 bold gold). Label 24 at 6.1 in, value right 7.3 from the edge, comparison 35 out.
        constexpr float kCardW = 289.0F, kRowW = 277.0F, kCardRowH = 33.4F, kCardHeader = 37.7F, kCardPad = 10.0F;
        constexpr float kCardText = 24.0F;

        struct CardBox {
            float left, right, firstRow;
            [[nodiscard]] float RowTop(int i) const { return firstRow + static_cast<float>(i) * kCardRowH; }
        };

        // Greedy word wrap at `width` for fo4::Text at `size`.
        std::vector<std::string> Wrap(std::string_view text, float size, float width) {
            std::vector<std::string> lines;
            std::string line;
            std::size_t at = 0;
            while (at < text.size()) {
                const auto end = std::min(text.find(' ', at), text.size());
                const auto word = text.substr(at, end - at);
                const auto joined = line.empty() ? std::string(word) : line + " " + std::string(word);
                if (!line.empty() && TextWidth(size, joined) > width) {
                    lines.push_back(std::move(line));
                    line = std::string(word);
                } else line = joined;
                at = end + 1;
            }
            if (!line.empty()) lines.push_back(std::move(line));
            return lines;
        }

        CardBox DrawCardBox(float centerX, float bottom, int rows, ImU32 fill, float extra = 0.0F) {
            const float top = bottom - kCardPad - static_cast<float>(rows) * kCardRowH - kCardHeader - 1.0F - extra;
            Rect(centerX - kCardW / 2, top, centerX + kCardW / 2, bottom, fill);
            Rect(centerX - kCardW / 2 + 1.9F, top + 1.0F, centerX + kCardW / 2 - 1.9F, top + 1.0F + kCardHeader, theme::CardHeader);
            const DropShadow halo{theme::DarkOnGold, 45.0F, 2.0F, 1.0F, 1.0F};
            ShadowText(centerX, top + 1.0F + 6.0F, 24.0F, "ITEM STATS", theme::Hud(theme::Gold), true, -0.4F, Align::Center, halo);
            const CardBox box{centerX - kRowW / 2, centerX + kRowW / 2, top + 1.0F + kCardHeader + extra};
            // Each row its own band with a thin gap to the next (ItemCard_StandardEntry backers, as FO76 shows them).
            for (int i = 0; i < rows; ++i) Rect(box.left, box.RowTop(i), box.right, box.RowTop(i) + kCardRowH - 2.0F, theme::CardRow);
            return box;
        }

        void StatLine(const CardBox& box, float top, const StatRow& row) {
            float labelX = box.left + 6.1F;
            const float textY = top + 2.5F, cy = top + kCardRowH / 2;
            if (!row.labelIcon.empty()) {
                ShadowIcon(row.labelIcon, labelX + 9.0F, cy, 18.0F, theme::Hud(theme::Ink));
                labelX += 22.0F;
            }
            ShadowText(labelX, textY, kCardText, row.label, theme::Hud(theme::Ink));
            const float valueRight = box.right - 7.3F;
            ShadowText(valueRight, textY, kCardText, row.value, theme::Hud(theme::Ink), false, 0.0F, Align::Right);
            if (!row.valueIcon.empty())
                ShadowIcon(row.valueIcon, valueRight - TextWidth(kCardText, row.value) - 14.0F, cy, 22.0F, theme::Hud(theme::Ink));
            if (row.compare != 0)
                ShadowText(box.right + 20.0F, textY, kCardText, inv::CompareMarks(row.compare), row.compare > 0 ? theme::Hud(theme::Ink) : theme::Unavailable);
        }

        // ---- inspect --------------------------------------------------------------------------------
        void ModsPanel(std::span<const Effect> mods) {
            // CenterShadedBG_mc "inspect": a shaded panel 1323.7..1613.9 x 519.7..930.2, "CURRENT MODS" 24 bold
            // gold at 524.7 with no plate, then CurrentModListEntry rows (277 x 33.4, text 24) from 564.6.
            Rect(1323.7F, 519.7F, 1613.9F, 930.2F, theme::InspectBox);
            const DropShadow halo{theme::DarkOnGold, 45.0F, 2.0F, 1.0F, 1.0F};
            ShadowText(1468.8F, 526.7F, 24.0F, "CURRENT MODS", theme::Hud(theme::Gold), true, -0.4F, Align::Center, halo);
            constexpr float kLeft = 1331.1F, kTop = 564.6F, kPitch = 35.0F;
            for (std::size_t i = 0; i < mods.size(); ++i) {
                const float top = kTop + static_cast<float>(i) * kPitch;
                if (top + kCardRowH > 930.0F) break;
                Rect(kLeft, top, kLeft + kRowW, top + kCardRowH, theme::CardRow);
                ShadowText(kLeft + 11.5F, top + 2.5F, 24.0F, mods[i].text, theme::Hud(theme::Ink), false, -0.1F);
                Stars(mods[i].stars, kLeft + 11.5F + TextWidth(24.0F, mods[i].text, false, -0.1F) + 8.0F, top + kCardRowH / 2, 13.0F, 15.0F,
                      theme::Hud(theme::Ink), true);
            }
        }

        // ---- Pip-Boy page ---------------------------------------------------------------------------
        constexpr float kPipListX = 42.5F, kPipListY = 163.8F, kPipRowH = 40.4F, kPipPitch = 45.4F;
        constexpr int kPipRows = 9;

        void PipRow(const Item& item, float top, bool selected, ImU32 tint, float x = kPipListX) {
            // PipBoyInvListEntry: plate -2.4..487.4 x 40.4; frames unselected / unselectedCantEquip /
            // selectedCantEquip / selected; text 28 at 24.2; equip square 11 x 11 at 8.6.
            const float cy = top + kPipRowH / 2;
            if (selected) Rect(x - 2.4F, top, x + 487.4F, top + kPipRowH, Tint(tint, item.disabled ? 0.5F : 1.0F));
            const ImU32 ink = selected ? kBlack : Tint(tint, item.disabled ? 0.5F : 1.0F);
            const inv::RowFlags flags{item.equipped, item.favorite, item.tagged, item.quest, item.setItem, item.setActive, item.locked};
            const auto lead = inv::LeadIcon(flags);
            if (lead == "item.equipped") Rect(x + 8.6F, cy - 5.5F, x + 19.6F, cy + 5.5F, ink);
            else Icon(lead, x + 14.2F, cy, 22.0F, ink);
            const auto label = inv::MakeRowLabel(item.name, item.count, item.legendaryStars);
            float tx = x + 24.2F;
            Text(tx, top + 3.0F, 28.0F, label.name, ink);
            tx += TextWidth(28.0F, label.name);
            if (label.stars > 0) tx = Stars(label.stars, tx + 6.0F, cy, 16.0F, 18.0F, ink, false);
            if (item.unique) {
                Icon("item.unique", tx + 15.0F, cy, 20.0F, ink);
                tx += 26.0F;
            }
            if (!label.suffix.empty()) {
                Text(tx, top + 3.0F, 28.0F, label.suffix, ink);
                tx += TextWidth(28.0F, label.suffix);
            }
            tx += 15.0F;
            for (const auto key : inv::TrailingIcons(flags)) {
                Icon(key, tx + 10.0F, cy, 20.0F, ink);
                tx += 20.0F + 10.0F;
            }
        }

        void ScrollArrow(float x, float y, bool up, ImU32 color) {
            const ImVec2 p[3]{Stage(x, up ? y + 14.0F : y), Stage(x + 7.0F, up ? y : y + 14.0F), Stage(x + 14.0F, up ? y + 14.0F : y)};
            Canvas()->AddTriangleFilled(p[0], p[1], p[2], color);
        }

        // ItemCardScrollable: ItemCard_StandardEntry rows 242 x 33.4 (#333, opaque, tinted) right edge at 795,
        // growing up from 595.4; label 28 regular, value 28 bold; no heading, no picture.
        // NewPaperDoll (sprite 477) at (676.4, 255) scale 0.98: every paperdoll.* icon shares one square frame
        // (centre (-0.65, -1.75), side 230.1 in the doll's units); PaperDollResistEntry icons and 18 pt values.
        void DrawPaperDoll(const PaperDoll& doll, ImU32 tint) {
            constexpr float kScale = 0.98F, kCx = 676.4F - 0.65F * kScale, kCy = 255.0F - 1.75F * kScale, kSide = 230.1F * kScale;
            const ImU32 ink = Tint(tint, 1.0F);
            const char* base = doll.base == PaperDoll::Base::VaultSuit ? "paperdoll.vaultsuit"
                               : doll.base == PaperDoll::Base::Clothes ? "paperdoll.clothes"
                                                                        : "paperdoll.body";
            Icon(base, kCx, kCy, kSide, Tint(tint, 0.3F));
            const std::array<std::pair<bool, const char*>, 8> parts{{{doll.torso, "paperdoll.torso"}, {doll.rightArm, "paperdoll.arm.right"},
                {doll.leftArm, "paperdoll.arm.left"}, {doll.rightLeg, "paperdoll.leg.right"}, {doll.leftLeg, "paperdoll.leg.left"},
                {doll.goggles, "paperdoll.goggles"}, {doll.helmet, "paperdoll.helmet"}, {doll.gasMask, "paperdoll.gasmask"}}};
            for (const auto& [shown, key] : parts)
                if (shown) Icon(key, kCx, kCy, kSide, ink);
            struct Entry { float iconX, iconY, textX, textY; int value; };
            const std::array<Entry, 6> entries{{{664.5F, 127.3F, 676.4F, 115.2F, doll.head},
                {588.9F, 203.0F, 600.8F, 190.9F, doll.rightArmResist}, {734.5F, 203.3F, 746.4F, 191.2F, doll.leftArmResist},
                {588.8F, 301.2F, 600.6F, 289.1F, doll.rightLegResist}, {731.6F, 300.9F, 743.5F, 288.9F, doll.leftLegResist},
                {660.4F, 377.1F, 672.3F, 365.0F, doll.chest}}};
            for (const auto& e : entries) {
                Icon(doll.resistIcon, e.iconX, e.iconY, 18.0F * kScale, ink);
                Text(e.textX + 2.0F, e.textY + 2.0F, 18.0F * kScale, std::to_string(e.value), ink);
            }
        }

        void PipCard(const Item& item, ImU32 tint, bool mini = false) {
            constexpr float kRight = 795.1F, kBottom = 595.4F, kW = 242.1F, kPitch = 35.4F;
            struct Row {
                std::string label, value, icon;
                int compare = 0;
                int kind = 0;  // 0 stat, 1 condition, 2 legendary
            };
            std::vector<Row> rows;
            if (item.legendaryStars > 0) rows.push_back({"Legendary", {}, {}, 0, 2});
            if (item.condition >= 0.0F) rows.push_back({"CND", {}, {}, 0, 1});
            for (const auto& s : item.stats) rows.push_back({s.label, s.value, s.valueIcon, s.compare, 0});
            if (item.weight >= 0.0F) rows.push_back({"Weight", inv::FormatNumber(item.weight), {}, 0, 0});
            if (item.value >= 0) rows.push_back({"Value", std::to_string(item.value), item.currency, 0, 0});
            // The scroll mask: 300 tall, or ItemCardScrollableMini's 200 under the paper doll.
            const int shown = std::min<int>(static_cast<int>(rows.size()), mini ? 5 : 8);
            const ImU32 ink = Tint(tint, 1.0F);
            for (int i = 0; i < shown; ++i) {
                const auto& r = rows[static_cast<std::size_t>(i)];
                const float top = kBottom - static_cast<float>(shown - i) * kPitch;
                Rect(kRight - kW, top, kRight, top + 33.4F, Tint(tint, 0.2F));
                Text(kRight - kW + 5.5F, top + 2.0F, 28.0F, r.label, ink);
                if (r.kind == 1) {
                    Rect(kRight - 166.4F + 0.0F, top + 4.7F, kRight - 6.4F, top + 28.7F, kBlack);
                    const auto fill = inv::ConditionBar(item.condition);
                    const float x0 = kRight - 165.6F, x1 = kRight - 6.7F;
                    if (fill.dim <= 0.0F) Rect(x0, top + 5.1F, x1, top + 28.3F, ink);
                    else {
                        Rect(x1 - (x1 - x0) * fill.dim, top + 9.5F, x1, top + 24.0F, ink);
                        if (fill.bright > 0.0F) Rect(x1 - (x1 - x0) * fill.bright, top + 14.0F, x1, top + 19.5F, Tint(tint, 0.5F));
                    }
                } else if (r.kind == 2) {
                    const float w = StarsWidth(item.legendaryStars, 18.0F, 20.0F);
                    Stars(item.legendaryStars, kRight - 5.7F - w, top + 16.7F, 18.0F, 20.0F, ink, false);
                } else {
                    Text(kRight - 5.7F, top + 2.0F, 28.0F, r.value, ink, true, 0.0F, Align::Right);
                    if (!r.icon.empty()) Icon(r.icon, kRight - 5.7F - TextWidth(28.0F, r.value, true) - 16.0F, top + 16.7F, 24.0F, ink);
                }
                if (r.compare != 0) Text(kRight + 10.0F, top + 2.0F, 28.0F, inv::CompareMarks(r.compare), ink);
            }
            if (static_cast<int>(rows.size()) > shown) ScrollArrow(556.5F, 602.4F, false, ink);
            if (mini) return;
            // Effects: the description entry over the card.
            float y = kBottom - static_cast<float>(shown) * kPitch - 8.0F - static_cast<float>(item.effects.size()) * 26.0F;
            for (const auto& e : item.effects) {
                const ImU32 lineInk = e.inactive ? Tint(ink, 0.5F) : ink;
                const float iw = e.icon.empty() ? 0.0F : 22.0F;
                const float left = kRight - kW / 2 - (TextWidth(20.0F, e.text) + iw) / 2;
                Text(left, y, 20.0F, e.text, lineInk);
                if (iw > 0) Icon(e.icon, left + TextWidth(20.0F, e.text) + 13.0F, y + 12.0F, 16.0F, lineInk);
                y += 26.0F;
            }
        }
    }

    ListResult InventoryPanel(Side side, const Panel& panel, std::span<const Item> items, ListModel& model, bool focused,
                              const Nav& nav, bool prices) {
        const float x0 = PanelX(side);
        Rect(x0, kPanelTop, x0 + kPanelW, kPanelBottom, theme::MenuPanel);
        ShadowText(x0 + 10.7F, 233.8F, 34.0F, Upper(panel.title), theme::Hud(theme::Gold), true, 0.2F);
        const bool header = HasHeader(panel);
        const auto box = TradeListBox(side, header);
        // Every row slot sits on its own faint band, empty ones too (PlayerListEntry's backer, as FO76 draws them).
        for (int i = header ? -1 : 0; i < box.rows; ++i) {
            const float top = box.top + static_cast<float>(i) * box.pitch;
            Rect(box.x0, top, box.x1, top + box.rowH, theme::CardRow);
        }
        ListResult header_;
        if (header) TradeHeader(side, panel, prices, header_);
        auto result = ListInput(box, static_cast<int>(items.size()), model, focused, nav);
        result.heading = header_.heading;
        result.pager = header_.pager;
        for (int i = model.First(); i < model.Last(); ++i) {
            const float top = box.top + static_cast<float>(i - model.First()) * box.pitch;
            TradeRow(x0 + kListDx, top, items[static_cast<std::size_t>(i)], i == model.Selected(), prices);
        }
        // shortcut: the list's ScrollUp / ScrollDown come from FO76's shared BSScrollingList library, not
        // securetrade.swf, so they sit centred in the gaps above the first and below the last row plate.
        const float mid = x0 + kListDx + 221.5F - 7.0F;
        if (model.First() > 0) ScrollArrow(mid, 275.0F, true, theme::Hud(theme::Ink));
        if (model.Last() < static_cast<int>(items.size())) ScrollArrow(mid, 836.0F, false, theme::Hud(theme::Ink));
        TradeFooter(side, panel);
        return result;
    }

    int TabBar(std::span<const std::string> tabs, int selected, std::span<const bool> enabled) {
        // CategoryBar_mc: backer 59.2..1857.7 x 61.3..115.8 (#111114, 75 %), labels 18.75 bold #FFFFCB (empty
        // tabs #566167), a gold selector with a glow (strength 0.5) that slides to the selected tab, bumper
        // hints at 96 and 1799.5.
        constexpr float kTop = 61.3F, kBottom = 115.8F, kSize = 18.75F, kLabelY = 77.0F;
        // Tracking keeps Roboto Condensed Bold caps apart at small sizes (E against L smeared at 1440p), as FO76's are.
        constexpr float kTrack = 0.7F;
        Rect(59.2F, kTop, 1857.7F, kBottom, theme::TabBack);
        const int count = static_cast<int>(tabs.size());
        const auto on = [&](int i) { return enabled.empty() || (static_cast<std::size_t>(i) < enabled.size() && enabled[static_cast<std::size_t>(i)]); };
        const float pitch = std::min(200.0F, 1520.0F / static_cast<float>(std::max(count, 1)));
        const auto centre = [&](int i) { return 960.0F + (static_cast<float>(i) - static_cast<float>(count - 1) / 2) * pitch; };
        int chosen = -1;
        if (selected >= 0 && selected < count) {
            const auto label = Upper(tabs[static_cast<std::size_t>(selected)]);
            const float cx = Slide(ImGui::GetID("##fo76-tab-x"), centre(selected));
            const float half = Slide(ImGui::GetID("##fo76-tab-w"), TextWidth(kSize, label, true, kTrack) / 2 + 20.0F);
            SelectionPlate(cx - half, 66.0F, cx + half, 111.0F, 0.5F);
        }
        for (int i = 0; i < count; ++i) {
            const auto label = Upper(tabs[static_cast<std::size_t>(i)]);
            const float cx = centre(i), half = TextWidth(kSize, label, true, kTrack) / 2 + 20.0F;
            const bool hover = on(i) && Hovered(cx - half, kTop, cx + half, kBottom);
            if (i == selected) Text(cx, kLabelY, kSize, label, kBlack, true, kTrack, Align::Center);
            else ShadowText(cx, kLabelY, kSize, label, !on(i) ? theme::Unavailable : hover ? theme::Hud(theme::Gold) : theme::Hud(theme::Ink), true, kTrack, Align::Center);
            if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) chosen = i;
        }
        if (common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD) {
            common::PadGlyph(Canvas(), Stage(108.0F, 88.5F), Px(13.5F), FUSIONUI_PAD_LB, theme::Hud(theme::Ink));
            common::PadGlyph(Canvas(), Stage(1812.0F, 88.5F), Px(13.5F), FUSIONUI_PAD_RB, theme::Hud(theme::Ink));
        } else {
            ShadowText(96.1F, 74.0F, 26.0F, "Z)", theme::Hud(theme::Ink));
            ShadowText(1822.0F, 74.0F, 26.0F, "C)", theme::Hud(theme::Ink), false, 0.0F, Align::Right);
        }
        if (count > 0) {
            const auto step = [&](int dir) {
                for (int n = 1; n <= count; ++n) {
                    const int i = ((selected + dir * n) % count + count) % count;
                    if (on(i)) return i;
                }
                return selected;
            };
            if (ImGui::GetIO().WantTextInput) return -1;
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) chosen = step(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_C, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) chosen = step(1);
        }
        return chosen == selected ? -1 : chosen;
    }

    float ItemCard(const Item& item, float centerX, float bottom, CardLook look) {
        const bool legendary = item.legendaryStars > 0, condition = item.condition >= 0.0F;
        const bool weight = item.weight >= 0.0F, value = item.value >= 0;
        const int rows = (legendary ? 1 : 0) + (condition ? 1 : 0) + static_cast<int>(item.stats.size()) + (weight ? 1 : 0) + (value ? 1 : 0);
        // ItemCard_DescriptionEntry: Label_tf word-wrapped, its background textHeight + 5 tall, above the rows.
        constexpr float kLine = kCardText * 1.15F;
        const auto lines = item.description.empty() ? std::vector<std::string>{} : Wrap(item.description, kCardText, kRowW - 12.2F);
        const float extra = lines.empty() ? 0.0F : static_cast<float>(lines.size()) * kLine + 5.0F;
        const auto box = DrawCardBox(centerX, bottom, rows, look == CardLook::Inspect ? theme::InspectBox : theme::MenuPanel, extra);
        if (!lines.empty()) {
            const float top = box.firstRow - extra;
            Rect(box.left, top, box.right, box.firstRow, theme::CardRow);
            for (std::size_t i = 0; i < lines.size(); ++i)
                ShadowText(box.left + 6.1F, top + 2.5F + static_cast<float>(i) * kLine, kCardText, lines[i], theme::Hud(theme::Ink));
        }
        int r = 0;
        if (legendary) {
            const float top = box.RowTop(r++);
            ShadowText(box.left + 6.1F, top + 2.5F, kCardText, "Legendary", theme::Hud(theme::Ink));
            const float w = StarsWidth(item.legendaryStars, 14.0F, 16.0F);
            Stars(item.legendaryStars, box.right - 7.3F - w, top + kCardRowH / 2, 14.0F, 16.0F, theme::Hud(theme::Ink), true);
        }
        if (condition) {
            const float top = box.RowTop(r++);
            ShadowText(box.left + 6.1F, top + 2.5F, kCardText, "CND", theme::Hud(theme::Ink));
            ConditionMeter(box.left + 108.2F, top + 4.2F, item.condition);
        }
        for (const auto& stat : item.stats) StatLine(box, box.RowTop(r++), stat);
        if (weight) StatLine(box, box.RowTop(r++), {"Weight", inv::FormatNumber(item.weight)});
        if (value) StatLine(box, box.RowTop(r++), {"Value", std::to_string(item.value), {}, item.currency});
        return box.firstRow - extra - kCardHeader - 2.0F;
    }

    void EffectLines(std::span<const Effect> effects, float centerX, float bottom) {
        // Description_tf: 22 bold #FFFFCB, centred, letter spacing 0.2.
        constexpr float kSize = 22.0F, kPitch = 28.0F;
        float y = bottom - static_cast<float>(effects.size()) * kPitch;
        constexpr float kIcon = 20.0F, kIconGap = 6.0F;
        for (const auto& e : effects) {
            const float w = TextWidth(kSize, e.text, true, 0.2F), sw = StarsWidth(e.stars, 13.0F, 15.0F);
            const float iw = e.icon.empty() ? 0.0F : kIconGap + kIcon;
            const float starsEnd = w + (e.stars > 0 ? 8.0F + sw : 0.0F);
            const float left = centerX - (starsEnd + iw) / 2;
            const ImU32 ink = e.inactive ? theme::Unavailable : theme::Hud(theme::Ink);
            ShadowText(left, y, kSize, e.text, ink, true, 0.2F);
            Stars(e.stars, left + w + 8.0F, y + 14.0F, 13.0F, 15.0F, ink, true);
            if (!e.icon.empty()) ShadowIcon(e.icon, left + starsEnd + kIconGap + kIcon / 2, y + 14.0F, kIcon, ink);
            y += kPitch;
        }
    }

    TradeResult TradeMenu(const TradeView& v, const Nav& navIn) {
        auto& input = common::ContextSlot<TradeInput>();
        const bool wasEnabled = input.enabled;
        input.enabled = input.enabled && v.interactive;
        TradeResult result;
        result.column = v.column;
        // While a search field has the keyboard, typed letters are text, not list / panel navigation.
        const Nav nav = ImGui::GetIO().WantTextInput ? Nav{} : navIn;
        if (v.interactive && nav.dx != 0) result.column = nav.dx < 0 ? 0 : 1;
        result.tab = fusionui::fo76::TabBar(v.tabs, v.tab, v.tabEnabled);
        auto& playerList = *v.playerList;
        auto& otherList = *v.otherList;
        const bool prices = v.kind == TradeKind::Barter;
        result.player = fusionui::fo76::InventoryPanel(Side::Player, v.player, v.playerItems, playerList, result.column == 0, nav, prices);
        result.other = fusionui::fo76::InventoryPanel(Side::Other, v.other, v.otherItems, otherList, result.column == 1, nav, prices);
        if (result.player.selectionChanged || result.player.activated >= 0) result.column = 0;
        if (result.other.selectionChanged || result.other.activated >= 0) result.column = 1;

        const auto& items = result.column == 0 ? v.playerItems : v.otherItems;
        const int selected = (result.column == 0 ? playerList : otherList).Selected();
        if (selected >= 0 && selected < static_cast<int>(items.size())) {
            const auto& item = items[static_cast<std::size_t>(selected)];
            const float top = fusionui::fo76::ItemCard(item, 960.4F, 909.6F);
            fusionui::fo76::EffectLines(item.effects, 957.1F, top - 12.0F);
            // The host's 3D model has one fixed place and size under the tab bar (FO76: about 182..446), like the
            // legendary notification's: a tall card draws over it rather than moving or shrinking it.
            constexpr float size = 330.0F;
            const auto at = fo4::Stage(960.4F, 150.0F + size / 2);
            const auto display = ImGui::GetIO().DisplaySize;
            result.preview = screens::PreviewSlot{at.x / display.x, at.y / display.y, std::min(1.0F, fo4::Px(size) / display.y)};
        }
        CornerPlate(v.corner, kTradeCorner);
        result.hint = fo76::HintBar(v.hints, 1011.5F);
        if (!v.status.empty()) ShadowText(960, 925, 26, v.status, theme::Hud(theme::Ink), false, 0, Align::Center);
        if (!v.interactive) { result.tab = result.hint = -1; }
        input.enabled = wasEnabled;
        return result;
    }

    int InspectCard(const Item& item, std::span<const Hint> hints) {
        // ItemName_tf 36 bold gold (spacing -0.4) at 36.8; LegendaryItemDescription_tf 24 #FFFFCB centred from
        // 79; the card (#181C1D box) centred on 457 down to 929.8; CURRENT MODS panel on the right.
        constexpr float kTitle = 36.0F;
        const float titleW = TextWidth(kTitle, item.name, true, -0.4F);
        const DropShadow halo{theme::DarkOnGold, 45.0F, 2.0F, 1.0F, 1.0F};
        ShadowText(960.0F, 38.8F, kTitle, item.name, theme::Hud(theme::Gold), true, -0.4F, Align::Center, halo);
        if (!item.name.empty()) {
            const float end = Stars(item.legendaryStars, 960.0F + titleW / 2 + 14.0F, 38.8F + 22.0F, 22.0F, 26.0F, theme::Hud(theme::Gold), true);
            if (item.unique) ShadowIcon("item.unique", (item.legendaryStars > 0 ? end : 960.0F + titleW / 2) + 22.0F, 38.8F + 22.0F, 28.0F,
                                        theme::Hud(theme::Gold));
        }
        float y = 81.0F;
        for (const auto& e : item.effects) {
            const float w = TextWidth(24.0F, e.text), sw = StarsWidth(e.stars, 14.0F, 16.0F);
            const float starsEnd = w + (e.stars > 0 ? 8.0F + sw : 0.0F);
            const float left = 960.0F - (starsEnd + (e.icon.empty() ? 0.0F : 28.0F)) / 2;
            const ImU32 ink = e.inactive ? theme::Unavailable : theme::Hud(theme::Ink);
            ShadowText(left, y, 24.0F, e.text, ink, false, 0.0F, Align::Left, halo);
            Stars(e.stars, left + w + 8.0F, y + 15.0F, 14.0F, 16.0F, ink, true);
            if (!e.icon.empty()) ShadowIcon(e.icon, left + starsEnd + 18.0F, y + 15.0F, 22.0F, ink);
            y += 29.0F;
        }
        fusionui::fo76::ItemCard(item, 457.2F, 929.8F, CardLook::Inspect);
        if (!item.mods.empty()) ModsPanel(item.mods);
        CornerPlate("Inspect", kInspectCorner);
        return fo76::HintBar(hints, 994.4F);
    }

    ModalResult PipBoyQuantity(std::string_view header, inv::Quantity& q, const Nav& nav, ImU32 tint) {
        // newpipboy_invpage symbol 17 in page units: black box 315.7..561.2 x 247.9..455.9 with a 2 px frame
        // (317.15..559.15 x 249..455) whose top edge is two bracket lines around Header_tf (38 bold, centred on
        // 437.6 from 217.5); Count_tf 60 bold from 270; slider frame 347.6..529.6 x 351..370, #333 fill from
        // 348.7, a 5 x 32 marker at the value, arrows 338.15..347.65 and 529.3..538.8.
        const ImU32 ink = Tint(tint ? tint : fo4::Fg(), 1.0F);
        auto* list = Canvas();
        ModalResult result;
        list->AddRectFilled({0, 0}, ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, 150));
        Rect(315.7F, 247.9F, 561.2F, 455.9F, kBlack);
        const float line = Px(2.0F);
        list->AddLine(Stage(317.15F, 249.0F), Stage(317.15F, 455.0F), ink, line);
        list->AddLine(Stage(317.15F, 455.0F), Stage(559.15F, 455.0F), ink, line);
        list->AddLine(Stage(559.15F, 455.0F), Stage(559.15F, 249.0F), ink, line);
        const auto title = Upper(header);
        const float w = TextWidth(38.0F, title, true), textLeft = 341.65F + (200.0F - w) / 2;
        list->AddLine(Stage(318.15F, 250.05F), Stage(textLeft - 9.0F, 250.05F), ink, line);
        list->AddLine(Stage(textLeft + w + 3.5F, 250.05F), Stage(558.45F, 250.05F), ink, line);
        Text(437.6F, 217.5F, 38.0F, title, ink, true, 0.0F, Align::Center);
        Text(437.6F, 270.0F, 60.0F, std::to_string(q.value), ink, true, 0.0F, Align::Center);

        constexpr float kFill0 = 348.7F, kFill1 = 527.5F;
        const float at = kFill0 + (kFill1 - kFill0) * q.Fraction();
        Rect(kFill0, 351.0F, at, 368.0F, Tint(tint ? tint : fo4::Fg(), 0.2F));
        list->AddRect(Stage(347.6F, 351.0F), Stage(529.6F, 370.0F), ink, 0.0F, 0, line);
        list->AddTriangleFilled(Stage(347.65F, 350.9F), Stage(338.15F, 360.4F), Stage(347.65F, 369.9F), ink);
        list->AddTriangleFilled(Stage(529.3F, 369.9F), Stage(538.8F, 360.4F), Stage(529.3F, 350.9F), ink);
        Rect(at - 2.5F, 343.8F, at + 2.5F, 375.8F, ink);

        int delta = nav.dx;
        const int step = q.Step(), jump = std::max(q.maximum / 4, 1);
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1) || ImGui::IsKeyPressed(ImGuiKey_PageUp)) delta += step;
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1) || ImGui::IsKeyPressed(ImGuiKey_PageDown)) delta -= step;
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadR2)) delta += jump;
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL2)) delta -= jump;
        if (const float wheel = ImGui::GetIO().MouseWheel; wheel != 0.0F) delta += wheel > 0.0F ? 1 : -1;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(Stage(kFill0, 340.0F), Stage(kFill1, 380.0F))) {
            const float fx = (ImGui::GetIO().MousePos.x - Stage(kFill0, 0.0F).x) / Px(kFill1 - kFill0);
            delta += 1 + static_cast<int>(std::lround(std::clamp(fx, 0.0F, 1.0F) * static_cast<float>(q.maximum - 1))) - q.value;
        }
        if (delta != 0) result.changed = q.Adjust(delta);
        if (nav.accept) result.choice = 0;
        else if (nav.cancel) result.choice = 1;
        return result;
    }

    PipBoyResult InventoryList(const PipBoyView& v, const Nav& nav) {
        auto& input = common::ContextSlot<TradeInput>();
        const bool wasEnabled = input.enabled;
        input.enabled = input.enabled && v.interactive;
        PipBoyResult result;
        const ImU32 tint = v.tint ? v.tint : fo4::Fg();
        const ImU32 ink = Tint(tint, 1.0F);
        auto* list = Canvas();

        // Page header (PageHeader_mc): STAT / INV / DATA / RADIO 36 bold on a rule at 63 with end ticks; the
        // page's brackets (Selector_Left / Right) slide to the selected title.
        constexpr std::array<const char*, 4> kPages{"STAT", "INV", "DATA", "RADIO"};
        constexpr std::array<float, 4> kPageX{192.5F, 344.2F, 497.0F, 666.7F};
        const int page = std::clamp(v.page, 0, 3);
        const float pageW = TextWidth(36.0F, kPages[static_cast<std::size_t>(page)], true);
        const float bx = Slide(ImGui::GetID("##pip-page-x"), kPageX[static_cast<std::size_t>(page)]);
        const float bl = bx - pageW / 2 - 15.0F, br = bx + pageW / 2 + 15.0F;
        Rect(38.1F, 63.1F, bl, 65.1F, ink);
        Rect(br, 63.1F, 834.8F, 65.1F, ink);
        Rect(38.1F, 63.1F, 40.1F, 73.2F, ink);
        Rect(832.8F, 63.1F, 834.8F, 73.2F, ink);
        for (const float x : {bl, br - 2.0F}) Rect(x, 35.2F, x + 2.0F, 65.1F, ink);
        Rect(bl, 35.2F, bl + 9.0F, 37.2F, ink);
        Rect(br - 9.0F, 35.2F, br, 37.2F, ink);
        for (std::size_t i = 0; i < kPages.size(); ++i) Text(kPageX[i], 15.0F, 36.0F, kPages[i], ink, true, 0.0F, Align::Center);
        if (common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD) {
            common::PadGlyph(list, Stage(71.0F, 40.0F), Px(14.0F), FUSIONUI_PAD_LT, ink);
            common::PadGlyph(list, Stage(800.8F, 40.0F), Px(14.0F), FUSIONUI_PAD_RT, ink);
        }

        // Sub-tab carousel (TabHeader_mc): 34 regular, the selected tab under the page title, neighbours at
        // 55 % and 28 %, sliding when the tab changes.
        const int count = static_cast<int>(v.tabs.size());
        result.tab = -1;
        if (count > 0) {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) result.tab = (v.tab + count - 1) % count;
            if (ImGui::IsKeyPressed(ImGuiKey_C, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) result.tab = (v.tab + 1) % count;
            const float slide = Slide(ImGui::GetID("##pip-carousel"), static_cast<float>(v.tab));
            const float cx = kPageX[static_cast<std::size_t>(page)];
            // FO76 places neighbours 133.3 and 254.3 out; longer labels push them out so they never overlap.
            const auto width = [&](int i) { return TextWidth(34.0F, Upper(v.tabs[static_cast<std::size_t>(((i % count) + count) % count)])); };
            const auto offsets = [&](int dir) {
                const float w0 = width(v.tab), w1 = width(v.tab + dir), w2 = width(v.tab + 2 * dir);
                const float one = std::max(133.3F, (w0 + w1) / 2 + 24.0F);
                return std::array<float, 3>{0.0F, one, one + std::max(121.0F, (w1 + w2) / 2 + 24.0F)};
            };
            const auto left = offsets(-1), right = offsets(1);
            constexpr std::array<float, 3> kShade{1.0F, 0.55F, 0.28F};
            list->PushClipRect(Stage(38.0F, 68.0F), Stage(835.0F, 125.0F), true);
            for (int i = 0; i < count; ++i) {
                const int slot = inv::CarouselSlot(i, v.tab, count);
                if (slot == 3) continue;
                // Fractional position while sliding: the slot shifted by how far the carousel still has to go.
                const float pos = static_cast<float>(slot) + (static_cast<float>(v.tab) - slide);
                const float a = std::abs(pos);
                if (a > 2.5F) continue;
                const int lo = std::min(static_cast<int>(a), 2), hi = std::min(lo + 1, 2);
                const float t = std::min(a - static_cast<float>(lo), 1.0F);
                const auto& kOffset = pos < 0 ? left : right;
                const float off = kOffset[static_cast<std::size_t>(lo)] + (kOffset[static_cast<std::size_t>(hi)] - kOffset[static_cast<std::size_t>(lo)]) * t;
                const float shade = kShade[static_cast<std::size_t>(lo)] + (kShade[static_cast<std::size_t>(hi)] - kShade[static_cast<std::size_t>(lo)]) * t;
                const float x = cx + (pos < 0 ? -off : off);
                const auto label = Upper(v.tabs[static_cast<std::size_t>(i)]);
                const float half = TextWidth(34.0F, label) / 2;
                Text(x, 72.4F, 34.0F, label, Tint(tint, shade), false, 0.0F, Align::Center);
                if (slot != 0 && Hovered(x - half, 70.0F, x + half, 119.0F) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) result.tab = i;
            }
            list->PopClipRect();
        }

        // Component view: ComponentList_mc (an InvList at 37.5) and ComponentOwnersList_mc at 508, 350 wide,
        // centred 28 pt names, no selection.
        auto& model = *v.list;
        if (v.componentView) {
            const ListBox box{kPipListX - 7.4F, kPipListX + 482.4F, kPipListY, kPipPitch, kPipRowH, kPipRows};
            result.list = ListInput(box, static_cast<int>(v.components.size()), model, true, nav);
            for (int i = model.First(); i < model.Last(); ++i)
                PipRow(v.components[static_cast<std::size_t>(i)], kPipListY + static_cast<float>(i - model.First()) * kPipPitch,
                       i == model.Selected(), tint, kPipListX - 5.0F);
            if (model.First() > 0) ScrollArrow(64.5F, 145.8F, true, ink);
            if (model.Last() < static_cast<int>(v.components.size())) ScrollArrow(63.5F, 567.2F, false, ink);
            const int owners = std::min<int>(static_cast<int>(v.owners.size()), kPipRows);
            for (int i = 0; i < owners; ++i)
                Text(683.0F, kPipListY + static_cast<float>(i) * kPipPitch + 3.0F, 28.0F, v.owners[static_cast<std::size_t>(i)], ink,
                     false, 0.0F, Align::Center);
            if (static_cast<int>(v.owners.size()) > owners) ScrollArrow(837.5F, 567.2F, false, ink);
        } else {
            // Item list (InvList): 9 rows, 45.4 apart, from (42.5, 163.8); scroll arrows when it overflows.
            const ListBox box{kPipListX - 2.4F, kPipListX + 487.4F, kPipListY, kPipPitch, kPipRowH, kPipRows};
            result.list = ListInput(box, static_cast<int>(v.items.size()), model, true, nav);
            for (int i = model.First(); i < model.Last(); ++i)
                PipRow(v.items[static_cast<std::size_t>(i)], kPipListY + static_cast<float>(i - model.First()) * kPipPitch, i == model.Selected(), tint);
            if (model.First() > 0) ScrollArrow(69.5F, 145.8F, true, ink);
            if (model.Last() < static_cast<int>(v.items.size())) ScrollArrow(68.5F, 567.2F, false, ink);

            const int selected = model.Selected();
            if (selected >= 0 && selected < static_cast<int>(v.items.size())) {
                if (v.doll) DrawPaperDoll(*v.doll, tint);
                PipCard(v.items[static_cast<std::size_t>(selected)], tint, v.doll != nullptr);
            }
        }

        // Bottom bar (BottomBar_Info "InvPage_*"): three #333 boxes at 629.8; weight and caps 28 bold with
        // their icons, the DMG / DR widget at the right end.
        const ImU32 boxFill = Tint(tint, 0.2F);
        Rect(37.1F, 629.6F, 234.7F, 674.8F, boxFill);
        Rect(236.1F, 629.6F, 437.0F, 674.8F, boxFill);
        Rect(437.4F, 629.6F, 837.8F, 674.8F, boxFill);
        if (!v.weight.empty()) {
            Icon("item.weight", 65.1F, 652.0F, 30.0F, ink);
            Text(90.1F, 636.0F, 28.0F, v.weight, ink, true);
        }
        if (!v.currencyAmount.empty()) {
            Icon("currency.caps", 264.8F, 652.8F, 32.0F, ink);
            Text(287.6F, 636.0F, 28.0F, v.currencyAmount, ink, true);
        }
        if (!v.damage.empty()) {
            Icon(v.damageIcon, 815.0F, 652.5F, 30.0F, ink);
            Text(792.0F, 636.0F, 28.0F, v.damage, ink, true, 0.0F, Align::Right);
        }
        // FO76 puts the bar at (438, 575); it is centred under the list here so a full hint set clears the card.
        result.hint = fo76::HintBar(v.hints, 580.0F, 285.0F, ink, 0.6F);
        if (!v.interactive) result.tab = result.hint = -1;
        input.enabled = wasEnabled;
        return result;
    }

    void WorkbenchItems(const fusionui::screens::WorkbenchList& list) {
        // FO76's crafting list rows (FO76 has no subtitle line), centred in each stock row.
        constexpr float kCellWidth = 44.0F;
        if (!list.title.empty() || !list.headings.empty()) {
            const ImU32 ink = theme::Hud(theme::Ink);
            const float y = list.headingTop;
            ShadowText(list.x + 19.7F, y, 18.0F, list.title, ink, true, 0.7F);
            float right = list.x + list.width - 7.9F;
            for (std::size_t c = list.headings.size(); c-- > 0; right -= kCellWidth) {
                const auto label = Fit(list.headings[c], 18.0F, kCellWidth - 4.0F, true);
                ShadowText(right, y, label.size, label.text, ink, true, 0.7F, Align::Right);
            }
        }
        for (const auto& row : list.rows)
            TradeRow(list.x, row.top + (row.height - kTradeRowH) / 2, row.item, row.selected, false, list.width, kCellWidth);
    }
}
