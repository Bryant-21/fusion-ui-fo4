#include "fusionui/Common.h"
#include "fusionui/Paths.h"
#include "kit/Settings.h"
#include "client/InputMap.h"
#include "core/PadCursor.h"
#include "core/PadFamily.h"
#include "fusionui/UiScale.h"

#include <imgui_internal.h>
#include <Windows.h>
#include <ShellScalingApi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fusionui::common {
    namespace {
        struct State {
            Fonts fonts;
            float uiScale = 1.0F;
            std::uint32_t device = FUSIONUI_DEVICE_KEYBOARD_MOUSE;
            ImU32 hud = IM_COL32(18, 255, 21, 255);
            std::function<void(float)> rebuild;
        };
        State& Current() { return ContextSlot<State>(); }

        float ReadFloat(const char* key, float fallback, float lo, float hi) {
            return std::clamp(static_cast<float>(settings::Shared().Load().GetFloat("Gamepad", key, fallback)), lo, hi);
        }

        struct PadLabel {
            const char* text;
            ImU32 color;
        };

        namespace ps {
            constexpr ImU32 Cross = IM_COL32(0x7C, 0xB2, 0xE8, 0xFF), Circle = IM_COL32(0xFF, 0x66, 0x66, 0xFF),
                            Square = IM_COL32(0xE5, 0x8C, 0xD9, 0xFF), Triangle = IM_COL32(0x40, 0xE2, 0xA0, 0xFF);
        }

        bool PlayStation() { return core::CurrentPadFamily().load(std::memory_order_relaxed) == core::PadFamily::PlayStation; }

        PadLabel LabelOf(std::uint32_t button, ImU32 ink) {
            // PlayStation face buttons are drawn as shapes (PadGlyph); the rest keep Xbox's layout under Sony's names.
            if (PlayStation()) switch (button) {
            case FUSIONUI_PAD_A: return {"", ps::Cross};
            case FUSIONUI_PAD_B: return {"", ps::Circle};
            case FUSIONUI_PAD_X: return {"", ps::Square};
            case FUSIONUI_PAD_Y: return {"", ps::Triangle};
            case FUSIONUI_PAD_LB: return {"L1", ink};
            case FUSIONUI_PAD_RB: return {"R1", ink};
            case FUSIONUI_PAD_LT: return {"L2", ink};
            case FUSIONUI_PAD_RT: return {"R2", ink};
            case FUSIONUI_PAD_BACK: return {"Share", ink};
            case FUSIONUI_PAD_START: return {"Options", ink};
            case FUSIONUI_PAD_LS: return {"L3", ink};
            case FUSIONUI_PAD_RS: return {"R3", ink};
            default: break;
            }
            switch (button) {
            case FUSIONUI_PAD_A: return {"A", pad::A};
            case FUSIONUI_PAD_B: return {"B", pad::B};
            case FUSIONUI_PAD_X: return {"X", pad::X};
            case FUSIONUI_PAD_Y: return {"Y", pad::Y};
            case FUSIONUI_PAD_LB: return {"LB", ink};
            case FUSIONUI_PAD_RB: return {"RB", ink};
            case FUSIONUI_PAD_LT: return {"LT", ink};
            case FUSIONUI_PAD_RT: return {"RT", ink};
            case FUSIONUI_PAD_BACK: return {"View", ink};
            case FUSIONUI_PAD_START: return {"Menu", ink};
            case FUSIONUI_PAD_LS: return {"LS", ink};
            case FUSIONUI_PAD_RS: return {"RS", ink};
            case FUSIONUI_PAD_DPAD_UP: return {"\xE2\x86\x91", ink};
            case FUSIONUI_PAD_DPAD_DOWN: return {"\xE2\x86\x93", ink};
            case FUSIONUI_PAD_DPAD_LEFT: return {"\xE2\x86\x90", ink};
            case FUSIONUI_PAD_DPAD_RIGHT: return {"\xE2\x86\x92", ink};
            default: return {"", ink};
            }
        }
    }

    std::filesystem::path WindowsFont(const wchar_t* name) {
        wchar_t windows[MAX_PATH]{};
        if (!::GetWindowsDirectoryW(windows, MAX_PATH)) ::GetEnvironmentVariableW(L"WINDIR", windows, MAX_PATH);
        return std::filesystem::path(windows) / "Fonts" / name;
    }

    namespace {
        // Every client context loads the same few files; read each once and lend the bytes to every atlas.
        // Never freed: atlases keep reading the data for as long as they live.
        const std::vector<char>* FontBytes(const std::filesystem::path& file) {
            static std::mutex mutex;
            static std::map<std::filesystem::path, std::unique_ptr<std::vector<char>>> cache;
            std::scoped_lock guard(mutex);
            auto& bytes = cache[file];
            if (!bytes) {
                std::ifstream stream(file, std::ios::binary);
                bytes = std::make_unique<std::vector<char>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
            }
            return bytes->empty() ? nullptr : bytes.get();
        }

        ImFont* AddFont(const std::filesystem::path& file, ImFontConfig config = {}) {
            const auto* bytes = FontBytes(file);
            if (!bytes) return nullptr;
            config.FontDataOwnedByAtlas = false;
            return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<char*>(bytes->data()), static_cast<int>(bytes->size()), 0.0F,
                                                             &config);
        }
    }

    ImFont* LoadFont(const std::filesystem::path& file, std::span<const FontMerge> merge) {
        auto* font = AddFont(file);
        if (!font) return nullptr;
        for (const auto& extra : merge) {
            ImFontConfig config;
            config.MergeMode = true;
            config.GlyphOffset = extra.glyphOffset;
            AddFont(extra.file, config);
        }
        return font;
    }

    const Fonts& InitBaseContext(const std::filesystem::path& fontDir) {
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
        // Roboto Condensed has no arrows, stars or box glyphs; Segoe UI Symbol (every Windows) fills them in.
        const std::array symbols{FontMerge{WindowsFont(L"seguisym.ttf")}};
        auto& fonts = Current().fonts;
        fonts.regular = LoadFont(fontDir / "RobotoCondensed-Regular.ttf", symbols);
        fonts.bold = LoadFont(fontDir / "RobotoCondensed-Bold.ttf", symbols);
        if (!fonts.regular) fonts.regular = io.Fonts->AddFontDefault();
        if (!fonts.bold) fonts.bold = fonts.regular;
        fonts.wide = LoadFont(fontDir / "Roboto-Regular.ttf", symbols);
        if (!fonts.wide) fonts.wide = fonts.regular;
        fonts.icons = LoadFont(fontDir / "Font_Awesome_7_Free-Solid-900.otf");
        return fonts;
    }

    const Fonts& CurrentFonts() { return Current().fonts; }

    void SetStyleRebuild(std::function<void(float)> rebuild) { Current().rebuild = std::move(rebuild); }

    void RebuildStyle() {
        auto& current = Current();
        if (!current.rebuild) return;
        // ScaleAllSizes compounds, so every rebuild starts from ImGui's defaults.
        ImGui::GetStyle() = ImGuiStyle();
        current.rebuild(current.uiScale);
    }

    void SetUiScale(float scale) {
        auto& current = Current();
        if (scale == current.uiScale) return;
        current.uiScale = scale;
        RebuildStyle();
    }

    float UiScale() { return Current().uiScale; }

    namespace {
        std::atomic<float> uiScaleOverride{uiscale::kAuto};
        std::atomic<std::uint32_t> monitorDpi{0};

        // The first visible top-level window of this process: the game's (or the preview's) window.
        HWND ProcessWindow() {
            struct Search {
                DWORD pid;
                HWND found;
            } search{::GetCurrentProcessId(), nullptr};
            ::EnumWindows(
                [](HWND window, LPARAM param) -> BOOL {
                    auto& s = *reinterpret_cast<Search*>(param);
                    DWORD pid{};
                    ::GetWindowThreadProcessId(window, &pid);
                    if (pid != s.pid || !::IsWindowVisible(window) || ::GetWindow(window, GW_OWNER)) return TRUE;
                    s.found = window;
                    return FALSE;
                },
                reinterpret_cast<LPARAM>(&search));
            return search.found;
        }
    }

    void SetUiScaleOverride(float scale) { uiScaleOverride = uiscale::Sanitize(scale); }

    float ResolveUiScale(float displayHeight) {
        return uiscale::Resolve(uiScaleOverride.load(), displayHeight, monitorDpi.load());
    }

    void RefreshMonitorDpi() {
        const HWND window = ProcessWindow();
        if (!window) return;
        // Fallout4.exe is DPI-unaware, so its threads are always told 96; ask as a per-monitor-aware thread.
        const auto previous = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        UINT dpiX{}, dpiY{};
        const HRESULT hr =
            ::GetDpiForMonitor(::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        if (previous) ::SetThreadDpiAwarenessContext(previous);
        monitorDpi = SUCCEEDED(hr) ? dpiX : 0;
    }
    std::uint32_t ActiveDevice() { return Current().device; }
    namespace {
        // Gamepad and joystick HID vendors, re-read at most every 2 s while a pad drives the UI (hot-plugging).
        void RefreshPadFamily() {
            static std::atomic<ULONGLONG> next{0};
            if (core::PadFamilyPinned()) return;
            const auto now = ::GetTickCount64();
            if (now < next.load(std::memory_order_relaxed)) return;
            next = now + 2000;
            UINT count = 0;
            if (::GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0) return;
            std::vector<RAWINPUTDEVICELIST> devices(count);
            count = ::GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST));
            if (count == static_cast<UINT>(-1)) return;
            std::vector<std::uint16_t> vendors;
            for (UINT i = 0; i < count; ++i) {
                if (devices[i].dwType != RIM_TYPEHID) continue;
                RID_DEVICE_INFO info{.cbSize = sizeof(RID_DEVICE_INFO)};
                UINT size = sizeof(info);
                if (::GetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &size) == static_cast<UINT>(-1)) continue;
                // Generic Desktop page: 4 joystick, 5 gamepad.
                if (info.hid.usUsagePage == 1 && (info.hid.usUsage == 4 || info.hid.usUsage == 5))
                    vendors.push_back(static_cast<std::uint16_t>(info.hid.dwVendorId));
            }
            core::CurrentPadFamily() = core::PadFamilyFromVendors(vendors);
        }
    }

    void SetActiveDevice(std::uint32_t device) {
        Current().device = device;
        if (device == FUSIONUI_DEVICE_GAMEPAD) RefreshPadFamily();
    }
    ImU32 HudColor() { return Current().hud; }
    void SetHudColor(ImU32 color) { Current().hud = color; }

    // ImGui keeps a nav focus for mouse users too (e.g. a newly focused window's first item); it only
    // shows while the nav cursor is visible.
    bool ItemNavFocused() { return ImGui::IsItemFocused() && GImGui->NavCursorVisible; }

    bool NavAdjust(float* value, float min, float max, float step) {
        if (!ImGui::IsItemFocused() || ImGui::IsItemActive()) return false;
        const ImGuiKey keys[]{ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadLStickLeft,
                              ImGuiKey_GamepadLStickRight};
        for (const auto key : keys) ImGui::SetItemKeyOwner(key);
        float delta = 0.0F;
        for (int i = 0; i < 4; ++i)
            if (ImGui::IsKeyPressed(keys[i], ImGuiInputFlags_Repeat, ImGui::GetItemID())) delta += i % 2 ? step : -step;
        if (delta == 0.0F) return false;
        *value = std::clamp(*value + delta * (max - min), min, max);
        return true;
    }

    float PadGlyphWidth(float r, std::uint32_t button) {
        if (button <= FUSIONUI_PAD_Y) return r * 2.0F;
        return std::max(r * 2.8F, ImGui::CalcTextSize(LabelOf(button, 0).text).x + r * 0.8F);
    }

    void PadGlyph(ImDrawList* list, ImVec2 c, float r, std::uint32_t button, ImU32 ink) {
        const auto [label, color] = LabelOf(button, ink);
        if (button <= FUSIONUI_PAD_Y) {
            list->AddCircleFilled(c, r, IM_COL32(0, 0, 0, 180));
            list->AddCircle(c, r, color, 0, 2.0F);
        } else {
            const float half = PadGlyphWidth(r, button) * 0.5F;
            list->AddRectFilled({c.x - half, c.y - r * 0.8F}, {c.x + half, c.y + r * 0.8F}, IM_COL32(0, 0, 0, 180), r * 0.3F);
            list->AddRect({c.x - half, c.y - r * 0.8F}, {c.x + half, c.y + r * 0.8F}, color, r * 0.3F, 0, 1.5F);
        }
        if (button <= FUSIONUI_PAD_Y && PlayStation()) {
            const float s = r * 0.45F, w = std::max(1.5F, r * 0.14F);
            switch (button) {
            case FUSIONUI_PAD_A:
                list->AddLine({c.x - s, c.y - s}, {c.x + s, c.y + s}, color, w);
                list->AddLine({c.x - s, c.y + s}, {c.x + s, c.y - s}, color, w);
                break;
            case FUSIONUI_PAD_B: list->AddCircle(c, s, color, 0, w); break;
            case FUSIONUI_PAD_X: list->AddRect({c.x - s * 0.85F, c.y - s * 0.85F}, {c.x + s * 0.85F, c.y + s * 0.85F}, color, 0, 0, w); break;
            default: list->AddTriangle({c.x, c.y - s}, {c.x + s, c.y + s * 0.75F}, {c.x - s, c.y + s * 0.75F}, color, w); break;
            }
            return;
        }
        const auto size = ImGui::CalcTextSize(label);
        list->AddText({c.x - size.x * 0.5F, c.y - size.y * 0.5F}, color, label);
    }

    bool PadPressed(std::uint32_t button) {
        return button != kNoPad && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
               ImGui::IsKeyPressed(client::KeyFromPad(button & ~kPadGlyphOnly), false);
    }

    namespace {
        constexpr float kMinPointerMultiplier = 0.5F, kMaxPointerMultiplier = 4.0F;
        std::atomic<float> pointerMultiplier{0.0F};
    }

    float PointerMultiplier() {
        if (pointerMultiplier.load() == 0.0F) RefreshPointerMultiplier();
        return pointerMultiplier.load();
    }

    void RefreshPointerMultiplier() {
        pointerMultiplier = ReadFloat("fPointerMultiplier", core::kDefaultPointerMultiplier, kMinPointerMultiplier, kMaxPointerMultiplier);
    }

    void SetPointerMultiplier(float multiplier) {
        multiplier = std::clamp(multiplier, kMinPointerMultiplier, kMaxPointerMultiplier);
        pointerMultiplier = multiplier;
        settings::Shared().Update([&](auto& document) {
            document.Set("Gamepad", "fPointerMultiplier", std::round(multiplier * 100.0) / 100.0);
        });
    }

    const PadTuningValues& PadTuning() {
        static const PadTuningValues values = [] {
            PadTuningValues v;
            v.deadzone = ReadFloat("fDeadzone", v.deadzone, 0.0F, 0.9F);
            v.cursorSpeed = ReadFloat("fCursorSpeed", v.cursorSpeed, 100.0F, 5000.0F);
            v.panSpeed = ReadFloat("fPanSpeed", v.panSpeed, 100.0F, 5000.0F);
            v.exponent = ReadFloat("fExponent", v.exponent, 0.5F, 3.0F);
            return v;
        }();
        return values;
    }
}
