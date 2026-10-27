#pragma once
namespace fusionui::fo76::detail {
    void ApplyStyle();
    void RefreshHudColors();  // re-applies the colors when the HUD colour changed; once per frame
    void ApplyColors();   // the colors alone, so a new accent keeps the scaled sizes
}
