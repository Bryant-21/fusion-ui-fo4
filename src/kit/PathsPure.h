#pragma once
#include <filesystem>

namespace fusionui::kit {
    std::filesystem::path AssetDirForModule(const std::filesystem::path& modulePath);
}
