#pragma once

namespace fusionui::game::InputCapture {
    void Install();      // kGameDataReady
    void EnsureFront();  // every time a modal opens; other mods may have inserted ahead of us
}
