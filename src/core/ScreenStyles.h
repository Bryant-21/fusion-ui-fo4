#pragma once
#include "fusionui/modern/Theme.h"

#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace fusionui::core::screens {
    using Fallbacks = std::set<std::pair<std::string, modern::theme::Family>>;

    inline bool FirstFallback(Fallbacks& reported, std::string_view screen,
                              modern::theme::Family requested, modern::theme::Family renderer) {
        return requested != renderer && reported.emplace(std::string(screen), requested).second;
    }
}
