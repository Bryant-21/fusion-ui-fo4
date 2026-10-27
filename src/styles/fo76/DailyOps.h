#pragma once
// Fallout 76's Daily Ops report (dailyopshud.swf DailyOpsModalManager) in FO76's 1920 x 1080 stage units.
#include "fusionui/screens/DailyOps.h"

namespace fusionui::fo76::dailyops {
    using screens::hud::DailyOpsReport;
    using screens::hud::Palette;
    int Draw(const DailyOpsReport& report, const Palette& palette, float alpha);
}
