#pragma once
// FusionUI.dll for fusionframework::require::Check. A UI plugin checks both DLLs first in F4SEPlugin_Load:
//   if (const auto notice = fusionframework::require::Check("My Mod", {fusionframework::require::kFramework,
//           fusionui::kRequire}); !notice.empty()) { /* register nothing; ShowNotice(notice) on game load */ }
#include "fusionframework/Require.h"
#include "fusionui/Version.h"

namespace fusionui {
inline constexpr fusionframework::require::Dll kRequire{"FusionUI.dll", "FusionUI_Version", "Fusion UI",
    FUSION_PACK_VERSION(FUSIONUI_VERSION_MAJOR, FUSIONUI_VERSION_MINOR, FUSIONUI_VERSION_PATCH)};
}
