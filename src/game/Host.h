#pragma once
#include "fusionui/Abi.h"
#include "core/Focus.h"
#include "core/InputTranslate.h"
#include "core/SettingsRegistry.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace fusionui::game {
    const FUSIONUI_HostApi* HostApiTable();
    // Runs once; `early` (the first main-menu frames) waits until the UI and renderer exist, else kGameDataReady starts it.
    void StartHost(bool early);
    void ArmEarlyStart();  // hooks MainMenu's advance to call StartHost(true)
    // FusionUI.dll's own F4SE listener (Plugin.cpp) forwards every F4SE message here.
    void OnF4SEMessage(const void* f4seMessage);

    class Host {
    public:
        static Host& Get();

        void Start();
        FUSIONUI_ClientId RegisterClient(const FUSIONUI_ClientDesc& desc);
        bool Open(FUSIONUI_ClientId id);
        bool OpenSettings(FUSIONUI_ClientId id);
        std::uint32_t SettingsPanels(FUSIONUI_SettingsPanel* panels, std::uint32_t capacity);
        const char* SettingsCategory(FUSIONUI_ClientId id);
        void Close(FUSIONUI_ClientId id);
        void SetPausesGame(FUSIONUI_ClientId id, bool pausesGame);
        bool IsOpen(FUSIONUI_ClientId id);
        [[nodiscard]] bool Available() const { return available_; }
        [[nodiscard]] std::uint32_t ActiveDevice();
        void SetCursor(float x, float y);
        [[nodiscard]] bool WantsGameState();
        [[nodiscard]] bool FocusedCapturesCharacters();
        // Lock-free: Present skips all D3D and ImGui work while no client is open.
        [[nodiscard]] bool AnyOpen() const { return anyOpen_.load(std::memory_order_relaxed); }

        void OnRawInput(const core::RawInput& input);
        void OnWindowFocus(bool focused);
        void CloseFocusedModal(const char* reason);
        // modalPass: true = draw the focused modal client, false = draw open overlays.
        void RenderClients(bool modalPass, ID3D11Device* device, ID3D11DeviceContext* context, float width, float height,
                           float deltaSeconds, std::uint32_t frameFlags);
        void DeviceLost();
        void SetRenderAvailable(bool available) { available_ = available; }

    private:
        struct Client {
            FUSIONUI_ClientDesc desc{};
            std::string name;
        };
        void ApplyGameState();
        Client* Find(FUSIONUI_ClientId id);

        std::mutex mutex_;
        core::FocusArbiter focus_;
        core::SettingsRegistry settings_;
        bool lastPanelLoaded_{};
        core::InputTranslator translator_;
        std::vector<FUSIONUI_Event> pending_;
        std::vector<Client> clients_;
        bool available_{};
        bool started_{};
        std::atomic<bool> wantsGameState_{};
        std::atomic<bool> anyOpen_{};
        float lastWidth_{};
        float lastHeight_{};
    };

    // Maps game cursor units to render-target pixels (M0 Q4) and back.
    void CursorToPixels(float rawX, float rawY, float width, float height, float& x, float& y);
    void PixelsToCursor(float x, float y, float width, float height, float& rawX, float& rawY);
}
