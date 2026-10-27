#pragma once
#include <Windows.h>

namespace fusionui::game::RenderHooks {
    void Install();
    bool RendererReady();
    HWND Window();
}
