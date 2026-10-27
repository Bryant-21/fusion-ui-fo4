#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "game/Host.h"
#include "fusionui/Version.h"
#include "core/GameTasks.h"
#include "game/GameState.h"
#include "game/InputCapture.h"
#include "game/RenderHooks.h"
#include "game/Tasks.h"
#include "game/PauseSettings.h"
#include "game/McmRuntime.h"
#include "game/KeybindingsRuntime.h"
#include "game/PerfRuntime.h"
#include "perf/PerfHost.h"
#include "fusionui/Tasks.h"
#include "fusionui/fo4/MenuRefs.h"
#include "fusionui/modern/Theme.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <utility>

namespace fusionui::game {
    namespace {
        FUSIONUI_ClientId ApiRegister(const FUSIONUI_ClientDesc* desc) noexcept {
            return desc ? Host::Get().RegisterClient(*desc) : 0;
        }
        std::uint32_t ApiOpen(FUSIONUI_ClientId id) noexcept { return Host::Get().Open(id) ? 1u : 0u; }
        void ApiClose(FUSIONUI_ClientId id) noexcept { Host::Get().Close(id); }
        std::uint32_t ApiIsOpen(FUSIONUI_ClientId id) noexcept { return Host::Get().IsOpen(id) ? 1u : 0u; }
        std::uint32_t ApiAvailable() noexcept { return Host::Get().Available() ? 1u : 0u; }
        std::uint32_t ApiActiveDevice() noexcept { return Host::Get().ActiveDevice(); }
        void ApiSetCursor(float x, float y) noexcept { Host::Get().SetCursor(x, y); }
        void ApiSetPausesGame(FUSIONUI_ClientId id, std::uint32_t pausesGame) noexcept {
            Host::Get().SetPausesGame(id, pausesGame != 0);
        }
        void ApiQueueGameTask(void* user, FUSIONUI_TaskCallback run, FUSIONUI_TaskCallback destroy) noexcept {
            QueueLocalGameTask(core::OwnedGameTask(user, run, destroy));
        }
        std::uint32_t ApiSettingsPanels(FUSIONUI_SettingsPanel* panels, std::uint32_t capacity) noexcept {
            return Host::Get().SettingsPanels(panels, capacity);
        }
        std::uint32_t ApiOpenSettings(FUSIONUI_ClientId id) noexcept { return Host::Get().OpenSettings(id) ? 1u : 0u; }
        const char* ApiSettingsCategory(FUSIONUI_ClientId id) noexcept { return Host::Get().SettingsCategory(id); }
        std::uint32_t ApiRegisterKeybindings(const FUSIONUI_KeybindingProvider* provider) noexcept {
            try { return provider && KeybindingsRuntime::Register(*provider) ? 1u : 0u; }
            catch (...) { return 0; }
        }

        // A modal shown under a loading screen took the game state mid-load: the world came back
        // black and the UI could not close. A load or the main menu therefore closes the focused modal.
        class BlockingMenuWatch : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        public:
            static BlockingMenuWatch* Get() { static BlockingMenuWatch watch; return &watch; }
            RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& e,
                                                  RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
                if (e.menuName == RE::MainMenu::MENU_NAME) GameState::SetAtTitle(e.opening);
                if (!e.opening) return RE::BSEventNotifyControl::kContinue;
                if (e.menuName == RE::LoadingMenu::MENU_NAME) Host::Get().CloseFocusedModal("loading screen");
                else if (e.menuName == RE::MainMenu::MENU_NAME) Host::Get().CloseFocusedModal("main menu");
                return RE::BSEventNotifyControl::kContinue;
            }
        };

        float ClientWidth(float fallback, float& height) {
            RECT client{};
            if (auto* window = RenderHooks::Window(); window && ::GetClientRect(window, &client) && client.right > 0 &&
                                                      client.bottom > 0) {
                height = static_cast<float>(client.bottom - client.top);
                return static_cast<float>(client.right - client.left);
            }
            return fallback;
        }
    }

    const FUSIONUI_HostApi* HostApiTable() {
        static const FUSIONUI_HostApi table = [] {
            FUSIONUI_HostApi t{sizeof(FUSIONUI_HostApi), FUSIONUI_ABI_VERSION, &ApiRegister, &ApiOpen, &ApiClose,
                            &ApiIsOpen, &ApiAvailable, &ApiActiveDevice, &ApiSetCursor, &ApiSetPausesGame,
                            &ApiQueueGameTask, &ApiSettingsPanels, &ApiOpenSettings, &ApiSettingsCategory, &ApiRegisterKeybindings};
            perf::FillHostApi(t);
            return t;
        }();
        return &table;
    }

    void StartHost(bool early) {
        if (early && !(RE::UI::GetSingleton() && RE::MenuControls::GetSingleton() && RenderHooks::RendererReady())) return;
        Host::Get().Start();
    }

    namespace {
        REL::Relocation<void (*)(RE::IMenu*, float, std::uint64_t)> advanceMainMenu;

        // The main menu runs ~20 s before kGameDataReady, and at kInputLoaded there is no swap chain yet.
        void AdvanceMainMenu(RE::IMenu* menu, float delta, std::uint64_t time) {
            StartHost(true);
            advanceMainMenu(menu, delta, time);
        }
    }

    void ArmEarlyStart() {
        REL::Relocation<std::uintptr_t> table{RE::VTABLE::MainMenu[0]};
        advanceMainMenu = table.write_vfunc(0x04, AdvanceMainMenu);
    }

    // M0 Q4 mapping B: MenuCursor is in window-client pixels; the render target can differ in size.
    void CursorToPixels(float rawX, float rawY, float width, float height, float& x, float& y) {
        float clientHeight = height;
        const float clientWidth = ClientWidth(width, clientHeight);
        x = rawX * width / clientWidth;
        y = rawY * height / clientHeight;
    }

    void PixelsToCursor(float x, float y, float width, float height, float& rawX, float& rawY) {
        float clientHeight = height;
        const float clientWidth = ClientWidth(width, clientHeight);
        rawX = width > 0 ? x * clientWidth / width : x;
        rawY = height > 0 ? y * clientHeight / height : y;
    }

    Host& Host::Get() {
        static Host host;
        return host;
    }

    void Host::Start() {
        if (std::exchange(started_, true)) return;
        McmRuntime::Start();
        KeybindingsRuntime::Start();
        PerfRuntime::Start();
        GameState::RegisterMenu();
        PauseSettings::Install();
        if (auto* ui = RE::UI::GetSingleton()) {
            ui->RegisterSink<RE::MenuOpenCloseEvent>(BlockingMenuWatch::Get());
            // The host can start from the title menu's own advance, after its open event.
            GameState::SetAtTitle(fusionui::fo4::menurefs::IsOpen<RE::MainMenu>());
        }
        InputCapture::Install();
        RenderHooks::Install();
        spdlog::info("FusionUI host started ({}.{}.{})", FUSIONUI_VERSION_MAJOR, FUSIONUI_VERSION_MINOR, FUSIONUI_VERSION_PATCH);
    }

    Host::Client* Host::Find(FUSIONUI_ClientId id) {
        return id >= 1 && id <= clients_.size() ? &clients_[id - 1] : nullptr;
    }

    FUSIONUI_ClientId Host::RegisterClient(const FUSIONUI_ClientDesc& desc) {
        if (desc.size < FUSIONUI_CLIENTDESC_MIN_SIZE || !desc.render) return 0;
        std::scoped_lock guard(mutex_);
        Client client;
        std::memcpy(&client.desc, &desc, std::min<std::size_t>(desc.size, sizeof(client.desc)));
        client.desc.size = sizeof(FUSIONUI_ClientDesc);
        client.name = desc.name ? desc.name : "";
        const auto id = focus_.Add(desc.kind == FUSIONUI_KIND_OVERLAY ? core::ClientKind::Overlay : core::ClientKind::Modal,
                                   core::SettingsRegistry::PausesGame(desc));
        clients_.push_back(std::move(client));
        clients_.back().desc.name = clients_.back().name.c_str();
        settings_.Add(id, desc);
        spdlog::info("FusionUI: registered client '{}' (id {})", clients_.back().name, id);
        return id;
    }

    bool Host::Open(FUSIONUI_ClientId id) {
        bool settings{};
        {
            std::scoped_lock guard(mutex_);
            settings = settings_.Contains(id);
        }
        if (settings) return OpenSettings(id);
        FUSIONUI_ClientDesc notify{};
        {
            std::scoped_lock guard(mutex_);
            const auto* client = Find(id);
            if (client && client->desc.kind != FUSIONUI_KIND_OVERLAY)
                if (const auto* reason = GameState::ModalBlockedReason()) {
                    spdlog::info("FusionUI: '{}' not opened: {}", client->name, reason);
                    return false;
                }
            const bool wasFocused = focus_.Focused() == id;
            if (!focus_.Open(id)) return false;
            wantsGameState_ = focus_.WantsGameState();
            anyOpen_ = !focus_.RenderOrder().empty();
            if (!wasFocused && focus_.Focused() == id)
                if (const auto* c = Find(id)) notify = c->desc;
        }
        ApplyGameState();
        if (notify.focusChanged) notify.focusChanged(notify.user, 1);
        return true;
    }

    std::uint32_t Host::SettingsPanels(FUSIONUI_SettingsPanel* panels, std::uint32_t capacity) {
        std::scoped_lock guard(mutex_);
        return settings_.Panels(panels, capacity);
    }
    const char* Host::SettingsCategory(FUSIONUI_ClientId id) {
        std::scoped_lock guard(mutex_);
        return settings_.Category(id);
    }

    bool Host::OpenSettings(FUSIONUI_ClientId id) {
        if (GameState::SettingsBlockedReason()) return false;
        FUSIONUI_ClientDesc closed{}, opened{};
        {
            std::scoped_lock guard(mutex_);
            if (!std::exchange(lastPanelLoaded_, true))
                settings_.Prefer(modern::theme::SavedWindowText("FUSIONUI_Settings", "LastPanel"));
            const auto target = settings_.Resolve(id);
            if (!target) return false;
            const auto current = focus_.Focused();
            if (current == target) return true;
            if (current && !settings_.Contains(current)) return false;
            if (const auto* client = Find(current)) closed = client->desc;
            focus_.Close(current);
            focus_.Open(target);
            settings_.Select(target);
            modern::theme::SaveWindowText("FUSIONUI_Settings", "LastPanel", settings_.Label(target));
            wantsGameState_ = focus_.WantsGameState();
            anyOpen_ = !focus_.RenderOrder().empty();
            opened = Find(target)->desc;
        }
        // Change panels without dropping the shared cursor, player-control lock or pause menu.
        ApplyGameState();
        if (closed.focusChanged) closed.focusChanged(closed.user, 0);
        if (opened.focusChanged) opened.focusChanged(opened.user, 1);
        return true;
    }

    void Host::Close(FUSIONUI_ClientId id) {
        FUSIONUI_ClientDesc notify{};
        {
            std::scoped_lock guard(mutex_);
            const bool wasFocused = id != 0 && focus_.Focused() == id;
            focus_.Close(id);
            wantsGameState_ = focus_.WantsGameState();
            anyOpen_ = !focus_.RenderOrder().empty();
            if (wasFocused) {
                translator_.ReleaseAll(pending_);
                pending_.clear();
                if (const auto* c = Find(id)) notify = c->desc;
            }
        }
        ApplyGameState();
        if (notify.focusChanged) notify.focusChanged(notify.user, 0);
    }

    void Host::SetPausesGame(FUSIONUI_ClientId id, bool pausesGame) {
        {
            std::scoped_lock guard(mutex_);
            if (!focus_.SetPausesGame(id, pausesGame || settings_.Contains(id))) return;
        }
        ApplyGameState();
    }

    bool Host::IsOpen(FUSIONUI_ClientId id) {
        std::scoped_lock guard(mutex_);
        return focus_.IsOpen(id);
    }

    std::uint32_t Host::ActiveDevice() {
        std::scoped_lock guard(mutex_);
        return translator_.ActiveDevice();
    }

    bool Host::WantsGameState() { return wantsGameState_; }

    bool Host::FocusedCapturesCharacters() {
        std::scoped_lock guard(mutex_);
        const auto* client = Find(focus_.Focused());
        return client && (client->desc.flags & FUSIONUI_FLAG_CAPTURES_CHARACTERS) != 0;
    }

    void Host::SetCursor(float x, float y) {
        float width{}, height{};
        {
            std::scoped_lock guard(mutex_);
            width = lastWidth_;
            height = lastHeight_;
        }
        auto* cursor = RE::MenuCursor::GetSingleton();
        if (!cursor || width <= 0 || height <= 0) return;
        float rawX{}, rawY{};
        PixelsToCursor(x, y, width, height, rawX, rawY);
        cursor->cursorPosX = static_cast<std::int32_t>(rawX);
        cursor->cursorPosY = static_cast<std::int32_t>(rawY);
    }

    // Main thread only (Open/Close are marshaled there). Never called under mutex_: the engine calls
    // below take input locks, and the input threads hold those locks while they call into the host.
    void Host::ApplyGameState() {
        bool wanted{}, pause{};
        {
            std::scoped_lock guard(mutex_);
            wanted = focus_.WantsGameState();
            // No world to pause at the title menu; MOD SETTINGS is the only modal that opens there.
            pause = focus_.WantsPause() && !GameState::AtTitle();
        }
        if (wanted && !GameState::Active()) {
            InputCapture::EnsureFront();
            GameState::Enter(pause);
        } else if (!wanted && GameState::Active()) {
            GameState::Leave();
        } else if (wanted) {
            GameState::SetPausesGame(pause);
        }
    }

    void Host::OnRawInput(const core::RawInput& input) {
        std::scoped_lock guard(mutex_);
        const auto before = translator_.ActiveDevice();
        translator_.Feed(input, pending_);
        // Wheel encoding is unverified on some runtimes: log the first few raw wheel events.
        if (static int wheelLogs = 0; input.device == core::RawDevice::Mouse && (input.idCode == 0x800 || input.idCode == 0x900) && wheelLogs < 12) {
            ++wheelLogs;
            spdlog::info("FusionUI: wheel id 0x{:X} value {:.2f} held {:.3f}", input.idCode, input.value, input.heldSeconds);
        }
        if (translator_.ActiveDevice() != before)
            spdlog::info("FusionUI: active device -> {} (kind {} device {} id 0x{:X} value {:.2f} x {:.2f} y {:.2f})",
                         translator_.ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD ? "gamepad" : "keyboard/mouse",
                         static_cast<int>(input.kind), static_cast<int>(input.device), input.idCode, input.value,
                         input.x, input.y);
    }

    void Host::OnWindowFocus(bool focused) {
        bool reenter = false;
        bool pause = false;
        {
            std::scoped_lock guard(mutex_);
            if (!focused) translator_.ReleaseAll(pending_);
            pending_.push_back({FUSIONUI_EV_FOCUS, 0, focused ? 1u : 0u, 0.0F, 0.0F});
            reenter = focused && focus_.WantsGameState();
            pause = focus_.WantsPause();
        }
        if (reenter)
            QueueGameTask([pause] {
                if (!GameState::Active()) return;
                GameState::Leave();
                GameState::Enter(pause);
            });
    }

    void Host::CloseFocusedModal(const char* reason) {
        FUSIONUI_ClientId focused{};
        {
            std::scoped_lock guard(mutex_);
            focused = focus_.Focused();
            if (const auto* c = Find(focused)) spdlog::info("FusionUI: {} opened; closing '{}'", reason, c->name);
        }
        if (focused) Close(focused);
    }

    void Host::RenderClients(bool modalPass, ID3D11Device* device, ID3D11DeviceContext* context, float width, float height,
                             float deltaSeconds, std::uint32_t frameFlags) {
        std::vector<FUSIONUI_Event> events;
        std::vector<FUSIONUI_ClientDesc> targets;
        std::uint32_t activeDevice{};
        {
            std::scoped_lock guard(mutex_);
            lastWidth_ = width;
            lastHeight_ = height;
            const auto focused = focus_.Focused();
            for (const auto id : focus_.RenderOrder()) {
                if ((id == focused) != modalPass) continue;
                if (const auto* c = Find(id)) targets.push_back(c->desc);
            }
            if (targets.empty()) return;
            if (modalPass) events.swap(pending_);
            activeDevice = translator_.ActiveDevice();
        }
        const auto hud = RE::HUDMenuUtils::GetGameplayHUDColor();
        const auto channel = [](float value) { return static_cast<std::uint32_t>(std::clamp(value, 0.0F, 1.0F) * 255.0F + 0.5F); };
        const std::uint32_t hudColor = 0xFF000000u | channel(hud.r) << 16 | channel(hud.g) << 8 | channel(hud.b);
        float cx{}, cy{};
        if (auto* cursor = RE::MenuCursor::GetSingleton())
            CursorToPixels(static_cast<float>(cursor->cursorPosX), static_cast<float>(cursor->cursorPosY), width, height, cx, cy);
        const FUSIONUI_Frame frame{sizeof(FUSIONUI_Frame), frameFlags, width, height, deltaSeconds, cx, cy, activeDevice,
                                modalPass ? 1u : 0u, device, context, events.data(),
                                static_cast<std::uint32_t>(events.size()), hudColor};
        for (const auto& desc : targets) desc.render(desc.user, &frame);
    }

    void Host::DeviceLost() {
        perf::Host::Get().DeviceLost();
        std::vector<FUSIONUI_ClientDesc> all;
        {
            std::scoped_lock guard(mutex_);
            for (const auto& c : clients_) all.push_back(c.desc);
        }
        for (const auto& desc : all)
            if (desc.deviceLost) desc.deviceLost(desc.user);
    }
}
