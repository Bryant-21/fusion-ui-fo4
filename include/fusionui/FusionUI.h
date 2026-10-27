#pragma once
#include "fusionui/Abi.h"

#include <cstdint>

namespace F4SE { class MessagingInterface; }

// The consumer's version, set by fusionui_use() from the target's `fusionui.version` value or set_version().
#ifndef FUSIONUI_MOD_VERSION
#define FUSIONUI_MOD_VERSION ""
#endif

namespace fusionui {
    enum class Device : std::uint8_t { KeyboardMouse = FUSIONUI_DEVICE_KEYBOARD_MOUSE, Gamepad = FUSIONUI_DEVICE_GAMEPAD };

    struct FrameContext {
        float width{};
        float height{};
        float deltaSeconds{};
        float cursorX{};
        float cursorY{};
        Device device{};
        bool focused{};
        ID3D11Device* d3d{};
        ID3D11DeviceContext* context{};
        float leftStickX{}, leftStickY{}, rightStickX{}, rightStickY{};   // raw, +y = up
    };

    struct ClientOptions {
        const char* name = "";
        bool modal = true;
        bool pausesGame = false;
        // Right stick shows and moves a pointer that A clicks. Views that drive their own pointer
        // (the map) turn this off.
        bool padPointer = true;
        // Kit sizes follow common::UiScale (the Windows display scale, or the DLL's override; see
        // fusionui/Common.h). Views laid out in physical
        // pixels, like the map's former web UI, turn this off and scale with the text scale alone.
        bool scaleWithResolution = true;
        // Draw this client's content inside MOD SETTINGS, with a shared mod selector on the left.
        bool settings = false;
        const char* settingsLabel = "";
        const char* settingsIcon = "";
        const char* settingsCategory = "Script Extender";
        // Shown in the MOD SETTINGS header so players can report which build they run.
        const char* settingsVersion = FUSIONUI_MOD_VERSION;
        // While focused, typed characters stop here instead of also reaching game menus. Opt in when
        // a vanilla menu stays open underneath the client (the console), or its text field fills too.
        bool capturesCharacters = false;
    };

    class Client {
    public:
        virtual ~Client() = default;
        // Called every frame the client is open, with this client's ImGui context current.
        virtual void Draw(const FrameContext& frame) = 0;
        // Called before settings chrome and Draw; true keeps the client's own family selector.
        virtual bool PrepareAppearance() { return false; }
        // Optional page links below the shared settings mod selector; render thread, same context.
        virtual void DrawSettingsNavigation(const FrameContext&) {}
        // Called once after the context, fonts and style exist; load textures here.
        virtual void OnContextCreated(const FrameContext&) {}
        virtual void OnFocusChanged(bool) {}
        // D3D objects the client created are invalid; drop them (they are reloaded lazily).
        virtual void OnDeviceLost() {}
    };

    // Must be called before the game reaches kPostLoad completion or any time after; calls made
    // before kPostLoad are queued. `client` must outlive the plugin.
    bool Register(Client& client, const ClientOptions& options);
    // Main-thread safe from any thread (marshaled). Returns false if another modal holds focus
    // or no host is available.
    bool Open(Client& client);
    void Close(Client& client);
    void SetPausesGame(Client& client, bool pausesGame);
    bool IsOpen(const Client& client);
    bool Available();
    Device ActiveDevice();
    // Moves the shared game cursor (render-target pixels).
    void SetCursor(float x, float y);

    // F4SE plugin message (no data) asking the receiving mod to open its main FusionUI window, as its
    // hotkey would. Sent on the game thread, e.g. by the Dev Tools launcher.
    inline constexpr std::uint32_t kOpenWindowMessage = 0x4232574FU;  // 'B2WO'
}
