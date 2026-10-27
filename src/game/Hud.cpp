#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "fusionui/fo4/MenuRefs.h"

#include "fusionui/fo4/HudGame.h"
#include "core/Hud.h"

namespace fusionui::fo4::hud {
    namespace {
        // The in-game pause menu is PauseMenu (MainMenu is the title screen). Console is absent on purpose: FO4
        // keeps its HUD under the console, and hiding ours there released the stock clips' leases.
        // FUSIONUI_CursorMenu is up while any FusionUI modal is.
        constexpr std::string_view kHidesHud[] = {
            "PipboyMenu", "ContainerMenu", "BarterMenu", "ExamineMenu", "WorkshopMenu", "DialogueMenu",
            "VATSMenu", "LooksMenu", "BookMenu", "TerminalMenu", "CookingMenu", "LevelUpMenu", "PauseMenu",
            "SPECIALMenu", "FavoritesMenu", "FUSIONUI_CursorMenu",
        };

        bool Open(const RE::UI& ui, std::string_view name) { return fusionui::fo4::menurefs::IsOpen(RE::BSFixedString(name)); }
    }

    bool Visible(std::span<const std::string_view> alsoHiddenBy) {
        core::hud::MenuState state;
        auto* ui = RE::UI::GetSingleton();
        state.uiReady = ui != nullptr;
        if (ui) {
            state.hudMenuOpen = Open(*ui, "HUDMenu");
            state.menusHidden = !ui->menuSystemVisible;
            state.loading = Open(*ui, "LoadingMenu");
            state.mainMenu = Open(*ui, "MainMenu");
            for (const auto name : kHidesHud) state.blockingMenus += Open(*ui, name) ? 1 : 0;
            for (const auto name : alsoHiddenBy) state.blockingMenus += Open(*ui, name) ? 1 : 0;
        }
        const auto* player = RE::PlayerCharacter::GetSingleton();
        state.gameLoaded = player && player->parentCell;
        return core::hud::ShouldDraw(state);
    }

    std::optional<ImVec2> Project(float x, float y, float z, ImVec2 viewport, float margin) {
        auto* camera = RE::Main::WorldRootCamera();
        if (!camera) return std::nullopt;
        float nx = 0, ny = 0, nz = 0;
        if (!camera->WorldPtToScreenPt3(RE::NiPoint3{x, y, z}, nx, ny, nz, 1e-5F)) return std::nullopt;
        const auto pixel = core::hud::NormalizedToScreen(nx, ny, nz, viewport.x, viewport.y, margin);
        if (!pixel) return std::nullopt;
        return ImVec2{(*pixel)[0], (*pixel)[1]};
    }

    std::optional<ImVec2> Project(float x, float y, float z, float margin) {
        return Project(x, y, z, ImGui::GetIO().DisplaySize, margin);
    }
}
