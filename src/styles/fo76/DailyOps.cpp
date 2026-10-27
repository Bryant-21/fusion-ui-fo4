#include "styles/fo76/DailyOps.h"
#include "styles/fo76/HudDraw.h"
#include "fusionui/fo76/Theme.h"

#include <algorithm>
#include <array>
#include <vector>

namespace fusionui::fo76::dailyops {
    namespace {
        using namespace hud::draw;
        using screens::hud::DailyOpsRank;
        using screens::hud::DailyOpsRankAt;
        using screens::hud::kDailyOpsRankIcons;
        using fo4::hud::Anchor;
        using fo4::hud::Edge;

        constexpr ImU32 kBlack = IM_COL32(0, 0, 0, 255);
        constexpr ImU32 kWhite = IM_COL32(255, 255, 255, 255);

        // A Flash text field placed by its top edge and font size (2-unit gutter, then the ascender).
        void Field(float x, float top, float size, std::string_view text, ImU32 color, bool bold, Align align, Fx fx = {}) {
            Caps(x, top + 2.0F + kCapTopEm * size, size * kCapEm, text, color, bold, align, 0.0F, fx);
        }

        template <std::size_t N>
        void Poly(const std::array<ImVec2, N>& stage, float dx, ImU32 color) {
            std::array<ImVec2, N> px;
            for (std::size_t i = 0; i < N; ++i) px[i] = fo4::Stage(stage[i].x + dx, stage[i].y);
            fo4::Canvas()->AddConvexPolyFilled(px.data(), static_cast<int>(N), color);
        }
        template <std::size_t N>
        void Outline(const std::array<ImVec2, N>& stage, float dx, ImU32 color, float thickness) {
            std::array<ImVec2, N> px;
            for (std::size_t i = 0; i < N; ++i) px[i] = fo4::Stage(stage[i].x + dx, stage[i].y);
            fo4::Canvas()->AddPolyline(px.data(), static_cast<int>(N), color, ImDrawFlags_Closed, fo4::Px(thickness));
        }
    }

    int Draw(const DailyOpsReport& report, const Palette&, float alpha) {
        Anchor(Edge::Center);
        auto* list = fo4::Canvas();
        const float frame = report.age * 30.0F;
        // Background (bitmap 12): slanted dark panel, blue rim, the skull watermark bottom right.
        const std::array<ImVec2, 4> panel{ImVec2{63.9F, 79.8F}, {1663.9F, 79.8F}, {1852.9F, 960.8F}, {249.9F, 960.8F}};
        Poly(panel, 0, Fade(IM_COL32(16, 19, 22, 231), alpha));
        Outline(panel, 0, Fade(IM_COL32(53, 75, 102, 255), alpha), 2.0F);
        Icon("dailyops.logo", 1690, 845, 210, IM_COL32(43, 127, 192, 255), alpha * 0.45F);
        if (icons::HasArt("dailyops.wordmark")) Icon("dailyops.wordmark", 940.85F, 189.55F, 428.0F, kWhite, alpha);
        else Field(940.85F, 150, 64, "DAILY OPS", Fade(kWhite, alpha), true, Align::Center);
        constexpr DropShadow kCopy{kBlack, 45.0F, 2.0F, 0.0F, 1.0F};
        const auto rank = [&](int i, const DailyOpsRank& r, int unearnedBefore, float a) {
            constexpr float kCx[] = {531.0F, 936.1F, 1354.0F};
            const float cx = kCx[i];
            const auto pose = DailyOpsRankAt(r.state, unearnedBefore, frame);
            // Earned layout at pose 1, the measured locked layout at the locked scales; old ranks are darkened
            // (-33 a channel) and so is everything still locked.
            const float u = (pose.title - 0.596F) / (1.0F - 0.596F), v = (pose.time - 0.85F) / 0.15F;
            const float dim = r.state == DailyOpsRank::State::Previously ? 0.75F : 0.75F + 0.25F * std::clamp(u, 0.0F, 1.0F);
            const auto blue = Fade(Mix(IM_COL32(43, 127, 192, 255), kBlack, 1.0F - dim), a);
            const DropShadow glow{pose.glowColor, 0.0F, 0.0F, pose.glowBlur, pose.glowStrength};
            const float iconSize = 200.0F + 60.0F * (pose.icon - 0.716F) / (1.0F - 0.716F);
            Icon(kDailyOpsRankIcons[i], cx, 600.0F, iconSize, blue, 1.0F, pose.glowStrength > 0 ? Fx{&glow} : Fx{});
            if (pose.lockAlpha > 0)
                Icon("item.locked", cx + 4 - pose.lockShift, 594.3F - pose.lockShift, 64 * pose.lock, kWhite, a * pose.lockAlpha, kGlowOnly);
            if (r.state != DailyOpsRank::State::Locked && pose.note > 0) {
                // "RANK ACHIEVED" / "PREVIOUSLY EARNED" over the shield, 70 px in two lines with a black halo.
                constexpr DropShadow kRankHalo{kBlack, 0.0F, 0.0F, 8.0F, 1.0F};
                const auto lines = Wrap(r.note, 70 * kCapEm, 220, true);
                float top = 600.0F - 42.0F * static_cast<float>(lines.size());
                for (const auto& line : lines) {
                    Field(cx, top, 70, line, Fade(kWhite, a * pose.note), true, Align::Center, {&kRankHalo});
                    top += 84.0F;
                }
            }
            Field(cx, 727.7F + 48.3F * u, 47.7F + 32.3F * u, r.title, Fade(Mix(kWhite, kBlack, 1.0F - dim), a), true, Align::Center, {&kCopy});
            Field(cx, 784.3F + 86.1F * v, 20.4F + 3.6F * v, r.time, Fade(kWhite, a * dim), true, Align::Center);
            if (r.state == DailyOpsRank::State::Locked && !r.note.empty())
                Field(cx, 812.0F, 20.4F, r.note, Fade(kWhite, a * 0.75F), true, Align::Center);
        };
        if (!report.rewardsPage) {
            const float info = alpha * std::clamp((frame - 1.0F) / 29.0F, 0.0F, 1.0F);
            Field(923.7F, 319.3F, 36, report.timeLabel, Fade(kWhite, info), true, Align::Right);
            Box(938.9F, 325, 939.9F, 360, Fade(IM_COL32(166, 166, 167, 255), info));
            Field(953.5F, 323.3F, 30, report.time, Fade(IM_COL32(127, 183, 250, 255), info), true, Align::Left);
            constexpr float kRows[] = {114.3F, 152.3F, 188.8F, 225.9F};
            for (std::size_t i = 0; i < report.info.size() && i < 4; ++i) {
                Field(1404, kRows[i], 26, report.info[i].first, Fade(kWhite, info), true, Align::Right);
                Box(1415.6F, kRows[i] + 6, 1416.6F, kRows[i] + 32, Fade(IM_COL32(166, 166, 167, 255), info));
                Field(1428.6F, kRows[i] + 3, 22, report.info[i].second, Fade(kWhite, info), false, Align::Left);
            }
            const float ranks = alpha * std::clamp((frame - 44.0F) / 21.0F, 0.0F, 1.0F);
            int unearned = 0;
            for (int i = 0; i < 3; ++i) {
                const auto& r = report.ranks[static_cast<std::size_t>(i)];
                rank(i, r, unearned, ranks);
                if (r.state != DailyOpsRank::State::Previously) ++unearned;
            }
        } else {
            // Header bars fade #004B66 to clear over 400 units; rows slide in 5 and fade, one after another.
            const auto header = [&](float top, std::string_view text) {
                list->AddRectFilledMultiColor(fo4::Stage(380, top - 2), fo4::Stage(780, top + 42), Fade(IM_COL32(0, 75, 102, 204), alpha), 0, 0,
                                              Fade(IM_COL32(0, 75, 102, 204), alpha));
                Field(395.95F, top, 36, text, Fade(kWhite, alpha * 0.5F), true, Align::Left);
            };
            const auto rows = [&](float top, const std::vector<std::string>& lines, int first) {
                for (std::size_t i = 0; i < lines.size() && i < 12; ++i) {
                    const float t = std::clamp((frame - 2.0F * static_cast<float>(first + static_cast<int>(i))) / 10.0F, 0.0F, 1.0F);
                    Field(395.95F + 5.0F * (1.0F - t), top + 31.4F * static_cast<float>(i), 20, lines[i], Fade(kWhite, alpha * 0.8F * t), true,
                          Align::Left);
                }
            };
            header(262.3F, report.rewardsTitle);
            rows(318.3F, report.rewards, 0);
            header(712.3F, report.rareTitle);
            rows(768.3F, report.rare, static_cast<int>(std::min<std::size_t>(report.rewards.size(), 12)));
            list->AddRectFilledMultiColor(fo4::Stage(855.4F, 318), fo4::Stage(1300, 382), Fade(IM_COL32(0, 75, 102, 204), alpha), 0, 0,
                                          Fade(IM_COL32(0, 75, 102, 204), alpha));
            Field(866.9F, 314.6F, 60, report.unlocksTitle, Fade(kWhite, alpha * 0.8F), true, Align::Left);
            for (int i = 0; i < 3; ++i) {
                const auto& unlock = report.unlocks[static_cast<std::size_t>(i)];
                const float t = std::clamp((frame - 2.0F - 20.0F * static_cast<float>(i)) / 10.0F, 0.0F, 1.0F);
                const float a = alpha * t, top = 402.4F + 140.0F * static_cast<float>(i), x = 855.4F + 10.0F * (1.0F - t);
                list->AddRectFilledMultiColor(fo4::Stage(x, top), fo4::Stage(x + 550, top + 120), Fade(IM_COL32(0, 75, 102, 128), a), 0, 0,
                                              Fade(IM_COL32(0, 75, 102, 128), a));
                Icon(kDailyOpsRankIcons[i], x + 68, top + 48, 80, IM_COL32(43, 127, 192, 255), a * 0.5F);
                Field(x + 68, top + 88, 28, unlock.rank, Fade(IM_COL32(153, 153, 153, 255), a), true, Align::Center);
                if (!unlock.earned) Icon("item.locked", x + 68, top + 48, 50, kWhite, a, kGlowOnly);
                Box(x + 136.9F, top + 14, x + 137.9F, top + 106, Fade(IM_COL32(166, 166, 167, 255), a));
                float line = top + 60.0F - 13.0F * static_cast<float>(unlock.lines.size());
                for (const auto& text : unlock.lines) {
                    Field(x + 150.5F, line, 21, text, Fade(IM_COL32(204, 204, 204, 255), a), true, Align::Left);
                    line += 26;
                }
            }
            Field(928.8F, 887.5F, 20, report.tooltip, Fade(IM_COL32(204, 204, 204, 255), alpha), false, Align::Center);
        }
        std::vector<fo4::Hint> hints;
        if (!report.rewardsPage) hints.push_back({FUSIONUI_PAD_A, "Enter", report.viewRewards.c_str()});
        hints.push_back({FUSIONUI_PAD_B, "Esc", report.exit.c_str()});
        const int clicked = fo76::HintBar(hints, 982.5F);
        return clicked < 0 ? -1 : report.rewardsPage ? 1 : clicked;
    }
}
