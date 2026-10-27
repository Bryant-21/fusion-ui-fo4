#pragma once
#include "fusionui/modern/AppearanceCodec.h"

#include <string_view>

namespace fusionui::screens {
    inline modern::theme::Family GameFamily(std::string_view name) {
        const auto family = modern::appearance::ParseFamily(name);
        return family == modern::theme::Family::Modern ? modern::theme::Family::Fallout76 : family;
    }

    using FallbackLogger = void (*)(std::string_view screen, std::string_view style);
    void SetFallbackLogger(FallbackLogger logger);
}
