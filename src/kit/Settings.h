#pragma once
#include <fusionframework/Config.h>

namespace fusionui::settings {
    // Data/F4SE/Plugins/FusionUI.toml, shared by every consumer DLL. One Store per process; write through
    // Update() so concurrent edits from other DLLs are read-modify-write. A missing file reads as defaults.
    fusionframework::config::Store& Shared();
}
