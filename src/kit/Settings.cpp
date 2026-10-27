#include "kit/Settings.h"
#include "fusionui/Paths.h"

namespace fusionui::settings {
    fusionframework::config::Store& Shared() {
        static fusionframework::config::Store store(AssetDir().parent_path() / "FusionUI.toml", "");
        return store;
    }
}
