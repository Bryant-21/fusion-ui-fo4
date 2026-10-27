#include "styles/common/Menus.h"
#include "styles/Frames.h"
#include "fusionui/Common.h"

#include <algorithm>
#include <span>

namespace fusionui::styles::menus {
    namespace {
        using namespace hudDraw;
        using namespace frames;
        using namespace screens::hud;

        float Lines(float x, float y, float width, std::span<const std::string> lines, ImU32 ink, float size = 15) {
            for (const auto& text : lines) y = Wrapped(x, y, size, width, text, ink, false, Align::Left) + 2;
            return y;
        }

        int Action(float x, float y, float width, std::string_view text, ImU32 ink, const char* key, std::uint32_t pad) {
            const auto min = At(x, y), max = At(x + width, y + 29);
            const bool hover = ImGui::IsMouseHoveringRect(min, max);
            Panel(x, y, width, 29, Fade(ink, hover ? 1.0F : 0.65F));
            if (common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD)
                common::PadGlyph(kit::Canvas(), At(x + 17, y + 14), Px(9), pad, ink);
            else Text(x + 8, y + 6, 13, key, ink, Align::Left, false, 48);
            Text(x + 59, y + 6, 15, text, ink, Align::Left, false, width - 67);
            return hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        }
    }

    int DrawDailyOps(const DailyOpsReport& report, const Palette& palette, float alpha) {
        Anchor(Edge::Center);
        const auto ink = Fade(palette.hud, alpha);
        Panel(210, 78, 860, 570, ink);
        Text(640, 96, 30, "DAILY OPS", ink, Align::Center);
        if (!report.rewardsPage) {
            Text(640, 146, 17, report.timeLabel + "  " + report.time, ink, Align::Center, true, 800);
            float y = 183;
            for (const auto& [label, value] : report.info) {
                Text(246, y, 13, label, Fade(ink, 0.65F), Align::Left, false, 180);
                y = Wrapped(435, y, 15, 590, value, ink, true, Align::Left) + 6;
            }
            y = std::max(340.0F, y + 12);
            for (std::size_t i = 0; i < report.ranks.size(); ++i) {
                const auto& rank = report.ranks[i];
                const float x = 245 + 270 * static_cast<float>(i);
                const auto tint = Fade(ink, rank.state == DailyOpsRank::State::Locked ? 0.45F : 1.0F);
                Panel(x, y, 250, 160, tint);
                Icon(kDailyOpsRankIcons[i], x + 125, y + 40, 52, tint);
                if (rank.state == DailyOpsRank::State::Locked) Icon("item.locked", x + 125, y + 40, 22, ink);
                Text(x + 125, y + 78, 19, rank.title, tint, Align::Center, true, 230);
                Text(x + 125, y + 108, 15, rank.time, tint, Align::Center, false, 230);
                Wrapped(x + 125, y + 134, 11, 230, rank.note, tint, false, Align::Center);
            }
        } else {
            struct Scroll { float offset{}, maximum{}, age{}; int frame{-1}; };
            auto& scroll = common::ContextSlot<Scroll>();
            if (scroll.frame + 1 != ImGui::GetFrameCount() || report.age < scroll.age) scroll.offset = 0;
            scroll.frame = ImGui::GetFrameCount();
            scroll.age = report.age;
            if (ImGui::IsMouseHoveringRect(At(235, 185), At(1045, 565))) scroll.offset -= ImGui::GetIO().MouseWheel * 40;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown)) scroll.offset += 35;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp)) scroll.offset -= 35;
            scroll.offset = std::clamp(scroll.offset, 0.0F, scroll.maximum);
            float bottom = 193;
            const auto column = [&](float x, std::string_view title, const std::vector<std::string>& lines) {
                Text(x, 151, 19, title, ink, Align::Left, true, 242);
                Line(x, 181, x + 242, 181, ink);
                kit::Canvas()->PushClipRect(At(x, 187), At(x + 247, 565), true);
                bottom = std::max(bottom, Lines(x, 193 - scroll.offset, 242, lines, ink, 12) + scroll.offset);
                kit::Canvas()->PopClipRect();
            };
            column(245, report.rewardsTitle, report.rewards);
            column(515, report.rareTitle, report.rare);
            Text(785, 151, 19, report.unlocksTitle, ink, Align::Left, true, 242);
            float y = 193 - scroll.offset;
            kit::Canvas()->PushClipRect(At(780, 187), At(1032, 565), true);
            for (std::size_t i = 0; i < report.unlocks.size(); ++i) {
                const auto& unlock = report.unlocks[i];
                const auto tint = Fade(ink, unlock.earned ? 1.0F : 0.5F);
                Icon(unlock.earned ? kDailyOpsRankIcons[i] : std::string_view("item.locked"), 796, y + 8, 20, tint);
                Text(814, y, 15, unlock.rank, tint, Align::Left, true, 213);
                y = Lines(785, y + 28, 242, unlock.lines, tint, 12) + 17;
            }
            kit::Canvas()->PopClipRect();
            bottom = std::max(bottom, y + scroll.offset);
            const float maximum = std::max(0.0F, bottom - 565);
            scroll.maximum = maximum;
            scroll.offset = std::clamp(scroll.offset, 0.0F, maximum);
            if (maximum > 0) {
                Line(1048, 190, 1048, 565, Fade(ink, 0.3F), 2);
                const float height = 375 * 375 / (375 + maximum);
                const float top = 190 + (375 - height) * scroll.offset / maximum;
                Box(1046, top, 4, height, ink);
            }
            Text(640, 580, 12, report.tooltip, ink, Align::Center, false, 800);
        }
        if (!report.rewardsPage && Action(365, 604, 310, report.viewRewards, ink, "Enter", FUSIONUI_PAD_A)) return 0;
        if (Action(report.rewardsPage ? 485.0F : 695.0F, 604, report.rewardsPage ? 310.0F : 220.0F, report.exit, ink, "Esc", FUSIONUI_PAD_B)) return 1;
        return -1;
    }
}
