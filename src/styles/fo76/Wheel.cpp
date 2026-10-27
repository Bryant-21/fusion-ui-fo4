#include "styles/fo76/Wheel.h"
#include "styles/fo76/ButtonHints.h"
#include "styles/fo76/Inventory.h"
#include "core/IconRuns.h"
#include "core/InventoryRows.h"
#include "fusionui/fo76/Theme.h"
#include "styles/fo76/HudDraw.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace fusionui::fo76::wheel {
    namespace {
        using namespace hud::draw;
        namespace cw = core::wheel;

        // radialmenu.swf, favorites mode, on its 1920 x 1080 stage.
        constexpr float kCx = 959.65F, kCy = 538.75F;            // wheel disk (shape 522) centre
        constexpr float kDiskR = 250.5F;                         // #242A2B at 80 % under a #182A40 radial gradient
        constexpr float kDiscX = 960.0F, kDiscY = 539.3F, kDiscR = 146.4F;  // Background_mc centre disc
        constexpr float kWedgeIn = 151.5F, kWedgeOut = 248.7F;   // Tier1Backer bitmap, entry 2 at 0 degrees
        constexpr float kHalfIn = 0.2478F, kHalfOut = 0.2566F;   // 14.2 / 14.7 degrees: straight sides
        constexpr float kOutline = 3.5F;                         // selection outline (shape 490)
        constexpr float kIconR = 199.5F, kIconBox = 50.0F;       // Icon_mc: the art measures ~50 across in the reference render
        constexpr float kHotkeyR = 276.4F, kHotkeyCap = 15.6F;   // Hotkey_tf: $MAIN_Font_Bold 22
        constexpr float kEquipR = 31.7F, kEquipW = 2.2F, kEquipHaloR = 35.5F, kEquipHaloW = 2.45F;  // EquippedState_mc
        constexpr float kListDim = 177.0F / 256.0F;              // "radialInventory": wheel layers at 177 / 256
        constexpr ImU32 kBacker = IM_COL32(0x18, 0x1C, 0x1D, 0xFF), kGradient = IM_COL32(0x18, 0x2A, 0x40, 0xFF);
        constexpr ImU32 kHighlight = IM_COL32(0xF0, 0xC6, 0x58, 0xFF);

        ImU32 Alpha(ImU32 color, float alpha) { return Fade(color, alpha); }
        ImU32 Scale(ImU32 color, float k) {
            const auto ch = [&](int shift) { return static_cast<ImU32>(static_cast<float>((color >> shift) & 0xFF) * k + 0.5F) << shift; };
            return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | (color & IM_COL32_A_MASK);
        }
        ImVec2 Polar(float r, float a) { return fo4::Stage(kCx + r * std::cos(a), kCy + r * std::sin(a)); }

        // Annular sector with straight sides, as a quad strip; colours run from the inner to the outer arc.
        void Wedge(float theta, float r0, float r1, float half0, float half1, ImU32 inner, ImU32 outer) {
            constexpr int kSteps = 12;
            auto* list = fo4::Canvas();
            const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            list->PrimReserve(kSteps * 6, (kSteps + 1) * 2);
            const auto base = static_cast<ImDrawIdx>(list->_VtxCurrentIdx);
            for (int k = 0; k <= kSteps; ++k) {
                const float t = -1.0F + 2.0F * static_cast<float>(k) / kSteps;
                list->PrimWriteVtx(Polar(r0, theta + t * half0), uv, inner);
                list->PrimWriteVtx(Polar(r1, theta + t * half1), uv, outer);
            }
            for (int k = 0; k < kSteps; ++k) {
                const auto i = static_cast<ImDrawIdx>(base + 2 * k);
                list->PrimWriteIdx(i);
                list->PrimWriteIdx(static_cast<ImDrawIdx>(i + 1));
                list->PrimWriteIdx(static_cast<ImDrawIdx>(i + 3));
                list->PrimWriteIdx(i);
                list->PrimWriteIdx(static_cast<ImDrawIdx>(i + 3));
                list->PrimWriteIdx(static_cast<ImDrawIdx>(i + 2));
            }
        }

        void Outline(float theta, ImU32 color, float thickness = kOutline) {
            constexpr int kSteps = 16;
            const float r0 = kWedgeIn + thickness / 2, r1 = kWedgeOut - thickness / 2;
            const float h0 = kHalfIn - thickness / 2 / r0, h1 = kHalfOut - thickness / 2 / r1;
            ImVec2 points[2 * (kSteps + 1)];
            for (int k = 0; k <= kSteps; ++k) {
                const float t = -1.0F + 2.0F * static_cast<float>(k) / kSteps;
                points[k] = Polar(r0, theta + t * h0);
                points[2 * kSteps + 1 - k] = Polar(r1, theta + t * h1);
            }
            fo4::Canvas()->AddPolyline(points, 2 * (kSteps + 1), color, ImDrawFlags_Closed, fo4::Px(thickness));
        }

        // Fan from the centre colour to the rim colour (Flash radial gradients).
        void RadialFill(float cx, float cy, float r, ImU32 center, ImU32 rim) {
            constexpr int kSteps = 96;
            auto* list = fo4::Canvas();
            const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
            list->PrimReserve(kSteps * 3, kSteps + 1);
            const auto base = static_cast<ImDrawIdx>(list->_VtxCurrentIdx);
            list->PrimWriteVtx(fo4::Stage(cx, cy), uv, center);
            for (int k = 0; k < kSteps; ++k) {
                const float a = 6.2831853F * static_cast<float>(k) / kSteps;
                list->PrimWriteVtx(fo4::Stage(cx + r * std::cos(a), cy + r * std::sin(a)), uv, rim);
            }
            for (int k = 0; k < kSteps; ++k) {
                list->PrimWriteIdx(base);
                list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 1 + k));
                list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 1 + (k + 1) % kSteps));
            }
        }

        // The slot's art (silhouette, else its icon key) with radialCategoryIconContainer's gold glow.
        void ItemArt(const Slot& slot, float cx, float cy, float box, ImU32 tint, ImU32 glowColor, float glow, float alpha) {
            const auto art = [&](float dx, float dy, ImU32 color, float a) {
                if (!slot.runs.empty()) core::iconruns::DrawRuns(fo4::Canvas(), slot.runs, cx + dx, cy + dy, box, Fade(color, a), fo4::Stage);
                else Icon(slot.icon.empty() ? "item.type.misc" : slot.icon, cx + dx, cy + dy, box, color, a);
            };
            if (glow > 0.0F) {
                const DropShadow halo{glowColor, 0.0F, 0.0F, 4.0F, glow};
                const float px = fo4::Px(1.0F);
                ShadowLayers(halo, alpha, [&](ImVec2 offset, ImU32 color) {
                    art(offset.x / px, offset.y / px, color | IM_COL32_A_MASK, static_cast<float>(color >> IM_COL32_A_SHIFT) / 255.0F);
                });
            }
            art(0, 0, tint, alpha);
        }

        // FO76's "hot" ring roll-on (RadialMenuRingInner frames 1-6): entries 179 -> 256.
        float RingAlpha(float openAge) {
            static constexpr float kHot[] = {179, 194, 210, 225, 241, 256};
            const auto frame = std::min<std::size_t>(static_cast<std::size_t>(std::max(openAge, 0.0F) * 30.0F), 5);
            return kHot[frame] / 256.0F;
        }

        void Ring(const Model& m, const Palette& p, float alpha) {
            const bool keyboard = common::ActiveDevice() != FUSIONUI_DEVICE_GAMEPAD;
            for (int i = 0; i < cw::kSlots; ++i) {
                const float theta = cw::SlotAngle(i);
                const auto& slot = m.slots[static_cast<std::size_t>(i)];
                const bool selected = i == m.state.selected;
                const float age = m.state.ages[static_cast<std::size_t>(i)];
                if (slot.name.empty()) {
                    // Backer_mc frame 2: the 10 px black frame bitmap at 128 / 256 under the backer at 192 / 256.
                    Outline(theta, Alpha(IM_COL32(16, 16, 16, 255), 0.5F * alpha), 10.0F);
                    Wedge(theta, kWedgeIn, kWedgeOut, kHalfIn, kHalfOut, Alpha(kBacker, 0.75F * alpha), Alpha(kBacker, 0.75F * alpha));
                } else {
                    Wedge(theta, kWedgeIn, kWedgeOut, kHalfIn, kHalfOut, Alpha(kBacker, alpha), Alpha(kBacker, alpha));
                }
                if (const float h = cw::HighlightAlpha(selected, age) * alpha; h > 0.0F) {
                    // BackgroundHighlight_mc: the gold gradient bitmap at 84 / 256, then the outline.
                    constexpr float kFill = 84.0F / 256.0F;
                    Wedge(theta, kWedgeIn, kWedgeOut, kHalfIn, kHalfOut, Alpha(theme::Hud(kHighlight), 196.0F / 255.0F * kFill * h),
                          Alpha(theme::Hud(kHighlight), 68.0F / 255.0F * kFill * h));
                    Outline(theta, Alpha(p.accent, h));
                }
                const float ix = kCx + kIconR * std::cos(theta), iy = kCy + kIconR * std::sin(theta);
                if (slot.equipped && !slot.name.empty()) {
                    fo4::Canvas()->AddCircle(fo4::Stage(ix, iy), fo4::Px(kEquipHaloR), Alpha(p.accent, 0.36F * alpha), 48, fo4::Px(kEquipHaloW));
                    fo4::Canvas()->AddCircle(fo4::Stage(ix, iy), fo4::Px(kEquipR), Alpha(theme::Hud(IM_COL32(0xF2, 0xC8, 0x5A, 0xFF)), alpha), 48,
                                             fo4::Px(kEquipW));
                }
                if (!slot.name.empty()) {
                    const auto pose = cw::IconPoseAt(selected, age);
                    ItemArt(slot, ix, iy, kIconBox, Scale(p.ink, pose.brightness), p.accent, pose.glow, alpha);
                }
                // Hotkey_tf shows only for keyboard and mouse (showKeyLabels).
                if (!keyboard) continue;
                const float hx = kCx + kHotkeyR * std::cos(theta), hy = kCy + kHotkeyR * std::sin(theta);
                FilteredCaps(kShadow, hx, hy - kHotkeyCap / 2, kHotkeyCap, cw::HotkeyLabel(i), p.ink, alpha, true, Align::Center);
            }
        }

        // CenterInfo_mc: icon in the disc, CND meter under it, name and ammo below the wheel.
        void CenterInfo(const Model& m, const Palette& p, float alpha) {
            const int s = m.state.selected;
            if (s < 0 || s >= cw::kSlots || m.slots[static_cast<std::size_t>(s)].name.empty()) return;
            const auto& slot = m.slots[static_cast<std::size_t>(s)];
            // IconContainer_mc at clipScale 1.7: the art measures ~115 across in the reference render.
            ItemArt(slot, 959.8F, 540.9F, slot.runs.empty() ? 92.0F : 118.0F, p.ink, p.accent, 0.0F, alpha);
            if (slot.condition >= 0.0F) {
                // ConditionBar_mc at 0.61: dark backing, "CND", the meter frame and its fill.
                Box(885.9F, 625.6F, 1037.3F, 648.6F, Alpha(kBacker, 0.3F * alpha));
                FilteredCaps(kShadow, 892.4F, 633.5F, 9.5F, "CND", p.ink, alpha);
                Box(933.7F, 630.1F, 1031.3F, 644.7F, Alpha(kBacker, 0.35F * alpha));
                fo4::Canvas()->AddRect(fo4::Stage(933.7F, 630.1F), fo4::Stage(1031.3F, 644.7F), Alpha(p.ink, alpha), 0.0F, 0, fo4::Px(1.0F));
                const auto fill = core::inventory::ConditionBar(slot.condition);
                const float x0 = 935.7F, x1 = 1029.3F, w = x1 - x0;
                if (fill.dim > 0.0F) Box(x0, 632.1F, x0 + w * fill.dim - 1, 642.7F, Alpha(p.ink, 0.45F * alpha));
                if (fill.bright > 0.0F) Box(x0, 632.1F, x0 + w * fill.bright - 1, 642.7F, Alpha(p.ink, alpha));
            }
            const auto title = slot.count > 1 ? std::format("{} ({})", slot.name, slot.count) : slot.name;
            FilteredCaps(kShadow, 954.7F, 836.0F, 32.7F, title, p.accent, alpha, true, Align::Center);
            if (!slot.ammo.empty())
                FilteredCaps(kShadow, 961.25F, 885.2F, 18.5F, std::format("{} ({})", slot.ammo, slot.ammoCount), p.ink, alpha, true,
                             Align::Center);
        }

        // radialTab: the gold blade with its slanted end, width = label + its height + 70 (updateFillWidth).
        void Tab(const Model& m, const Palette& p, float alpha) {
            constexpr float kTop = 1016.6F, kBottom = 1056.1F, kCap = 18.5F;
            // The corner shade under the tab: black 0.85 at the bottom-left corner, clear at the other two points.
            {
                const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
                auto* list = fo4::Canvas();
                list->PrimReserve(3, 3);
                const auto base = static_cast<ImDrawIdx>(list->_VtxCurrentIdx);
                list->PrimWriteVtx(fo4::Stage(0.8F, 551.75F), uv, IM_COL32(0, 0, 0, 0));
                list->PrimWriteVtx(fo4::Stage(529.35F, 1080.25F), uv, IM_COL32(0, 0, 0, 0));
                list->PrimWriteVtx(fo4::Stage(0.9F, 1080.35F), uv, Alpha(IM_COL32(0, 0, 0, 255), 0.85F * alpha));
                list->PrimWriteIdx(base);
                list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 1));
                list->PrimWriteIdx(static_cast<ImDrawIdx>(base + 2));
            }
            const float width = fo4::TextWidth(26.0F, m.tab, true) + 31.0F + 70.0F;  // 31: the 26 pt field's textHeight
            const float slant = 54.7F * width / 215.2F;
            const ImVec2 blade[4] = {fo4::Stage(0.8F, kTop), fo4::Stage(0.8F + width, kTop), fo4::Stage(0.8F + width - slant, kBottom),
                                     fo4::Stage(0.8F, kBottom)};
            fo4::Canvas()->AddConvexPolyFilled(blade, 4, Alpha(p.accent, alpha));
            Caps(24.9F, 1028.4F, kCap, m.tab, Alpha(kBacker, alpha), true);
        }

        std::vector<ButtonHint> HintRow(const Model& m, const cw::Facts& facts) {
            const bool pad = common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD;
            const int s = m.state.selected;
            const bool repairable = s >= 0 && s < cw::kSlots && m.slots[static_cast<std::size_t>(s)].condition >= 0.0F;
            std::vector<ButtonHint> row;
            for (const auto hint : cw::Hints(m.state, facts, pad, repairable)) {
                switch (hint) {
                case cw::Hint::Tent: row.push_back({"T)", "Survival Tent", true, FUSIONUI_PAD_Y}); break;
                case cw::Hint::Scroll: row.push_back({"Mousewheel)", "Scroll"}); break;
                case cw::Hint::Select: row.push_back({"Space)", "Select", true, static_cast<std::uint32_t>(m.state.listOpen ? FUSIONUI_PAD_A : FUSIONUI_PAD_RS)}); break;
                case cw::Hint::Use: row.push_back({"Space)", "Use", true, FUSIONUI_PAD_A}); break;
                case cw::Hint::Change: row.push_back({"C)", "Change", true, FUSIONUI_PAD_RB}); break;
                case cw::Hint::Inspect: row.push_back({"X)", "Inspect", true, FUSIONUI_PAD_RS}); break;
                case cw::Hint::InspectRepair: row.push_back({"X)", "Inspect/Repair", true, FUSIONUI_PAD_RS}); break;
                case cw::Hint::Back: row.push_back({"Tab)", "Back", true, FUSIONUI_PAD_B}); break;
                }
            }
            return row;
        }

        // PlayerInventory_mc in "radialInventory": FO76's SecureTradePlayerInventory panel at the trade
        // menu's right-hand panel position.
        void QuickInventory(const Model& m) {
            std::vector<Item> items;
            items.reserve(m.rows.size());
            for (const auto& row : m.rows)
                items.push_back({.name = row.text, .count = row.count, .equipped = row.equipped, .favorite = row.slot >= 0});
            ListModel list;
            list.SetVisibleRows(13);
            list.SetCount(static_cast<int>(items.size()));
            list.Select(m.state.listIndex);
            fusionui::fo76::InventoryPanel(Side::Other, {.title = m.listTitle}, items, list, true, Nav{});
            if (items.empty()) fo4::Text(1345.0F, 294.0F, 25.0F, m.noItems, fo4::Fg());
        }
    }

    core::wheel::Facts FactsOf(const Model& model) {
        cw::Facts facts;
        for (std::size_t i = 0; i < model.slots.size(); ++i) facts.occupied[i] = !model.slots[i].name.empty() && model.slots[i].count > 0;
        facts.change = model.change;
        facts.inspect = model.inspect;
        facts.tent = model.tent;
        facts.rows = model.rows;
        return facts;
    }

    void Draw(const Model& m, float alpha) {
        const float layers = alpha * (m.state.listOpen ? kListDim : 1.0F);
        const Palette p{theme::Hud(m.palette.ink), theme::Hud(m.palette.accent)};
        // ResolutionDarkeners: the whole screen under black at 0.3.
        fo4::Canvas()->AddRectFilled({0.0F, 0.0F}, ImGui::GetIO().DisplaySize, Alpha(IM_COL32(0, 0, 0, 255), 0.3F * alpha));
        fo4::hud::Anchor(fo4::hud::Edge::Center);
        fo4::Canvas()->AddCircleFilled(fo4::Stage(kCx, kCy), fo4::Px(kDiskR), Alpha(IM_COL32(0x24, 0x2A, 0x2B, 204), alpha), 128);
        RadialFill(kCx, kCy, kDiskR, Alpha(kGradient, 127.0F / 255.0F * alpha), Alpha(kGradient, 25.0F / 255.0F * alpha));
        // Background_mc rolls on at frame 3 and CenterInfo_mc with it.
        if (m.state.openAge >= 2.0F / 30.0F) {
            fo4::Canvas()->AddCircleFilled(fo4::Stage(kDiscX, kDiscY), fo4::Px(kDiscR), Alpha(kBacker, 0.5F * layers), 96);
            CenterInfo(m, p, layers);
        }
        Ring(m, p, layers * RingAlpha(m.state.openAge));
        const auto facts = fusionui::fo76::wheel::FactsOf(m);
        const auto hints = HintRow(m, facts);
        fusionui::fo76::ButtonHintBar(hints, 960.8F, 1016.35F, Alpha(p.ink, alpha));
        if (m.state.listOpen) QuickInventory(m);
        fo4::hud::Anchor(fo4::hud::Edge::Left);
        Tab(m, p, alpha);
        fo4::hud::Anchor(fo4::hud::Edge::Center);
    }
}
