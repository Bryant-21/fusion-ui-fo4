#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "fusionframework/DllLog.h"
#include "fusionui/Require.h"
#include "game/Host.h"

extern "C" F4SE::PluginVersionData F4SEPlugin_Version;

namespace {
    const bool earlyLog = fusionframework::dll_log::Start("FusionUI");
    bool ready{};

    void OnMessage(F4SE::MessagingInterface::Message* message) {
        if (ready) fusionui::game::OnF4SEMessage(message);
    }
}

extern "C" __declspec(dllexport) std::uint32_t FusionUI_Version() { return fusionui::kRequire.version; }

// The pre-NG F4SE on 1.10.163 reads only this classic export; NG/AE F4SE read F4SEPlugin_Version.
F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info) {
    if (!a_f4se || !a_info) return false;
    if (a_f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    a_info->infoVersion = F4SE::PluginInfo::kVersion;
    a_info->name = "FusionUI";
    a_info->version = F4SEPlugin_Version.pluginVersion;
    return true;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se) {
    F4SE::InitInfo info{};
    info.log = false;
    F4SE::Init(a_f4se, info);
    fusionframework::dll_log::Open("FusionUI");
    spdlog::info("FusionUI {}.{}.{} loaded", FUSIONUI_VERSION_MAJOR, FUSIONUI_VERSION_MINOR, FUSIONUI_VERSION_PATCH);
    if (const auto notice = fusionframework::require::Check("Fusion UI", {fusionframework::require::kFramework}); !notice.empty()) {
        spdlog::error("FusionUI disabled: {}", notice);
        return true;
    }
    ready = true;
    if (auto* messaging = F4SE::GetMessagingInterface(); !messaging || !messaging->RegisterListener(OnMessage))
        spdlog::error("FusionUI: F4SE messaging listener failed; no window will open");
    return true;
}
