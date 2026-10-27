#include "core/PadFamily.h"

namespace fusionui::core {
    std::atomic<PadFamily>& CurrentPadFamily() {
        static std::atomic<PadFamily> family{PadFamily::Xbox};
        return family;
    }

    std::atomic_bool& PadFamilyPinned() {
        static std::atomic_bool pinned{false};
        return pinned;
    }
}
