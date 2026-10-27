#pragma once
#include <filesystem>

namespace fusionui {
    // Data/F4SE/Plugins/<stem of the module containing `address`>/ (or the preview's override).
    std::filesystem::path AssetDirOf(const void* address);
    // Data/F4SE/Plugins/FusionUI/: the shared fonts, icon packs and crosshairs.
    std::filesystem::path AssetDir();
    // Data/F4SE/Plugins/<ThisDllStem>/ for the plugin that calls it: inline, so the address is the caller's own.
    inline std::filesystem::path PluginAssetDir() { return AssetDirOf(reinterpret_cast<const void*>(&PluginAssetDir)); }
    // AssetDir() where its files really are: MO2 serves Data through a virtual file system that Explorer cannot see
    // and that misses files added while the game runs.
    std::filesystem::path RealAssetDir();
    // Preview exe only: point the kit at a mod's asset folder.
    void SetAssetDirOverride(std::filesystem::path dir);
}
