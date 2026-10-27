#pragma once

namespace fusionui::game::GameState {
    void RegisterMenu();          // once, at kGameDataReady
    void Enter(bool pauseGame);   // show stub menu, lock controls, text-entry mode, optional freeze
    void Leave();
    void SetPausesGame(bool pausesGame);
    bool Active();
    // Why a modal must not open right now (loading screen, main menu, no save loaded); nullptr when it may.
    const char* ModalBlockedReason();
    // Like ModalBlockedReason, but the title menu is allowed.
    const char* SettingsBlockedReason();
    // The title menu is open (kept by Host from menu open / close events; read on input threads).
    void SetAtTitle(bool open);
    bool AtTitle();
}
