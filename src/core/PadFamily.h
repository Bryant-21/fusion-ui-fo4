#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <span>

namespace fusionui::core {
    // Which button names and glyphs a controller user sees. FO4 reads pads through XInput, so a PlayStation pad
    // (often wrapped by Steam Input) looks like an Xbox one to the game; only its HID vendor tells them apart.
    enum class PadFamily : std::uint8_t { Xbox, PlayStation };

    inline constexpr std::uint16_t kSonyVendor = 0x054C;

    // `vendors`: HID vendor ids of the connected gamepads/joysticks. Any Sony pad means PlayStation glyphs.
    inline PadFamily PadFamilyFromVendors(std::span<const std::uint16_t> vendors) {
        return std::ranges::find(vendors, kSonyVendor) != vendors.end() ? PadFamily::PlayStation : PadFamily::Xbox;
    }

    // Set by the kit's device poll; read wherever pad buttons are named. Functions, so every module shares FusionUI.dll's.
    std::atomic<PadFamily>& CurrentPadFamily();
    // The preview's --playstation pins the family; the device poll leaves it alone.
    std::atomic_bool& PadFamilyPinned();
}
