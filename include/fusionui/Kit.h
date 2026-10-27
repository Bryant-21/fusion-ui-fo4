#pragma once
// Compatibility name from before the style split: kit:: = the shared core (fusionui/Common.h) plus the
// fo76 look (fusionui/fo76/Style.h). New menus include one style header instead.
#include "fusionui/Common.h"
#include "fusionui/fo76/Style.h"

namespace fusionui::kit {
    using namespace ::fusionui::common;
    using namespace ::fusionui::fo76;
}
