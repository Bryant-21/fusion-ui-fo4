#pragma once
#include <imgui.h>
#include "fusionui/screens/HudPalette.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fusionui::screens::hud {
    // Daily Ops report (dailyopshud.swf DailyOpsModalManager): a full-screen modal, not part of the HUD.
    // Initiate, Paladin, Elder: rank shields `dailyops.rank.*` (FO76 tints them #2B7FC0).
    inline constexpr std::string_view kDailyOpsRankIcons[] = {"dailyops.rank.initiate", "dailyops.rank.paladin", "dailyops.rank.elder"};
    struct DailyOpsRank {
        std::string title;  // "PALADIN"
        std::string time;   // "< 10 MINUTES"
        enum class State { Locked, Earned, Previously } state = State::Locked;
        std::string note;   // over an earned shield ("RANK ACHIEVED", "PREVIOUSLY EARNED"); under a locked title ("(TEAM RECOMMENDED)")
    };
    struct DailyOpsUnlock {
        std::string rank;
        std::vector<std::string> lines;
        bool earned = false;  // no lock
    };
    struct DailyOpsReport {
        bool rewardsPage = false;
        // Ranking page
        std::string timeLabel = "COMPLETION TIME", time;
        std::vector<std::pair<std::string, std::string>> info;  // GAME MODE / LOCATION / FACTION / MUTATION rows
        std::array<DailyOpsRank, 3> ranks;
        // Rewards page
        std::string rewardsTitle = "REWARDS", rareTitle = "RARE REWARDS", unlocksTitle = "DAILY UNLOCKS";
        std::vector<std::string> rewards, rare;  // "(N) Name", at most 12 each
        std::array<DailyOpsUnlock, 3> unlocks;
        std::string tooltip;
        std::string viewRewards = "VIEW REWARDS", exit = "EXIT";
        float age = 0.0F;  // since this page opened
    };
    // A rank entry's pose (DORankEntry, sprite 56). A just-earned rank shows locked until it activates: its
    // lock plays "unlock" once the ranks are in (report frame 65), then 1.5 s + 0.8 s per earlier unearned
    // rank later the lock fades and the entry plays "activate" (icon, title and time grow from their locked
    // scales with a bounce and a #1E92CB glow), then loops "on" (the glow breathes over 120 frames).
    // Scales are relative to the earned layout; `lock` is the lock body's scale over its locked 0.25.
    struct DailyOpsRankPose {
        float icon = 1.0F, title = 1.0F, time = 1.0F;  // locked: 0.716, 0.596, 0.85
        float glowBlur = 0.0F, glowStrength = 0.0F;
        ImU32 glowColor = IM_COL32(30, 146, 203, 255);
        float note = 1.0F;  // "RANK ACHIEVED" alpha
        float lock = 1.0F, lockAlpha = 0.0F, lockShift = 0.0F;
    };
    inline constexpr float kDailyOpsRanksIn = 65.0F;
    inline DailyOpsRankPose DailyOpsRankAt(DailyOpsRank::State state, int unearnedBefore, float reportFrame) {
        // Frames 1..31 of "activate" (1 = the locked "off" pose).
        static constexpr float kIcon[] = {0.716F, 0.716F, 0.709F, 0.702F, 0.697F, 0.693F, 0.690F, 0.690F, 0.692F, 0.697F, 0.705F,
                                          0.719F, 0.740F, 0.770F, 0.813F, 0.863F, 0.912F, 0.950F, 0.977F, 0.996F, 1.010F, 1.018F,
                                          1.024F, 1.026F, 1.027F, 1.026F, 1.023F, 1.019F, 1.013F, 1.007F, 1.0F};
        static constexpr float kTitle[] = {0.596F, 0.596F, 0.594F, 0.588F, 0.580F, 0.571F, 0.563F, 0.558F, 0.556F, 0.558F, 0.568F,
                                           0.585F, 0.611F, 0.649F, 0.698F, 0.761F, 0.835F, 0.898F, 0.947F, 0.985F, 1.011F, 1.028F,
                                           1.038F, 1.040F, 1.038F, 1.032F, 1.025F, 1.016F, 1.008F, 1.002F, 1.0F};
        static constexpr float kTime[] = {0.85F,  0.85F,  0.849F, 0.847F, 0.844F, 0.841F, 0.838F, 0.836F, 0.835F, 0.836F, 0.839F,
                                          0.846F, 0.856F, 0.870F, 0.888F, 0.911F, 0.939F, 0.962F, 0.980F, 0.994F, 1.004F, 1.010F,
                                          1.014F, 1.015F, 1.014F, 1.012F, 1.009F, 1.006F, 1.003F, 1.001F, 1.0F};
        static constexpr float kBlur[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.09F, 0.76F, 1.74F, 3.10F, 4.74F, 6.30F, 7.52F, 8.40F,
                                          9.02F, 9.44F, 9.72F, 9.90F, 9.98F, 10.0F, 10.29F, 10.92F, 11.85F, 13.02F, 14.42F, 16.0F};
        static constexpr float kStrength[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.066F, 0.168F, 0.309F, 0.469F, 0.617F, 0.75F, 0.828F,
                                              0.898F, 0.938F, 0.969F, 0.977F, 0.988F, 1.0F, 0.957F, 0.887F, 0.777F, 0.637F, 0.477F, 0.297F};
        const auto at = [](const float (&track)[31], float f) {
            const float t = std::clamp(f, 1.0F, 31.0F) - 1.0F;
            const int i = std::min(static_cast<int>(t), 29);
            return track[i] + (track[i + 1] - track[i]) * (t - static_cast<float>(i));
        };
        // "on" loops frames 31..151: blur 16 -> 5 -> 16, strength 0.297 -> 0.79, colour toward #1E92CC and back.
        const auto breathe = [](DailyOpsRankPose& pose, float f) {
            const float tri = 1.0F - std::abs(std::fmod(f - 31.0F, 120.0F) - 60.0F) / 60.0F;
            pose.glowBlur = 16.0F - 11.0F * tri;
            pose.glowStrength = 0.297F + 0.493F * tri;
            pose.glowColor = IM_COL32(30, static_cast<int>(102 + 44 * tri), static_cast<int>(216 - 12 * tri), 255);
        };
        DailyOpsRankPose pose;
        if (state == DailyOpsRank::State::Previously) {
            breathe(pose, 31.0F);
            return pose;
        }
        pose = {at(kIcon, 1), at(kTitle, 1), at(kTime, 1), 0, 0, IM_COL32(30, 146, 203, 255), 0, 1, 1, 0};
        if (state == DailyOpsRank::State::Locked) return pose;
        // LockIcon_mc: "unlock" grows the body 0.25 -> 0.387 (frame 29) -> 0.375 and lifts it up-left 16, holds,
        // then "fadeOut" over 10 frames at activation.
        const float unlock = reportFrame - kDailyOpsRanksIn + 2.0F;
        const float activate = reportFrame - (kDailyOpsRanksIn + 45.0F + 24.0F * static_cast<float>(unearnedBefore)) + 2.0F;
        if (unlock > 12) {
            const float grow = std::clamp((unlock - 12.0F) / 17.0F, 0.0F, 1.0F);
            const float settle = std::clamp((unlock - 29.0F) / 6.0F, 0.0F, 1.0F);
            const float eased = grow * grow * (3.0F - 2.0F * grow);
            pose.lock = (0.25F + 0.137F * eased - 0.012F * settle) / 0.25F;
            pose.lockShift = 17.6F * eased - 1.6F * settle;
        }
        pose.lockAlpha = 1.0F - std::clamp((activate - 1.0F) / 10.0F, 0.0F, 1.0F);
        if (activate < 2) return pose;
        pose.icon = at(kIcon, activate);
        pose.title = at(kTitle, activate);
        pose.time = at(kTime, activate);
        if (activate < 31) {
            pose.glowBlur = at(kBlur, activate);
            pose.glowStrength = at(kStrength, activate);
        } else {
            breathe(pose, activate);
            pose.note = 1.0F;
        }
        return pose;
    }
    // The modal panel and its hint bar; returns the hint the mouse clicked (0 view rewards, 1 exit) or -1.
    int DailyOpsReportScreen(const DailyOpsReport& report, const Palette& palette, float alpha);
}
