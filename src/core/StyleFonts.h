#pragma once
#include "fusionui/modern/Theme.h"

namespace fusionui::core::styles {
    // Each family's text em relative to the FO4 font size the drawing helpers are given.
    inline float FontEm(modern::theme::Family family, bool bold) {
        using modern::theme::Family;
        switch (family) {
        case Family::Fallout3:
        case Family::NewVegas: return 1127.0F / 1000;
        case Family::Skyrim: return 1445.0F / 1000;
        case Family::Starfield: return bold ? 1200.0F / 1000 : 1300.0F / 1000;
        default: return 2400.0F / 2048;
        }
    }
}
