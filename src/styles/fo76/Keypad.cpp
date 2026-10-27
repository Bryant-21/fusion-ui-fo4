#include "styles/fo76/Keypad.h"
#include "styles/Controls.h"

#include <array>

// keypadmenu.swf, 16:9 frame: KeyRects_mc at (739.55, 374.45); Selector<row><column>_mc are shape 1 (a 143.85
// square ring 5.3 thick, #FFFFCB; its #F5CB5B hairline is lost under the glow) under a #F5CB5B glow (blur 6, strength 0.25).
// Columns sit 148.8 / 298.85 right, rows 138.55 / 277.1 / 423.85 down, following the 3D keypad's buttons.
namespace fusionui::fo76 {
    using fo4::Align;
    using fo4::Canvas;
    using fo4::Px;
    using fo4::Stage;
    namespace kp = core::keypad;

    namespace {
        constexpr float kGridX = 739.55F, kGridY = 374.45F, kCell = 143.85F, kRing = 5.3F;
        constexpr std::array<float, kp::kColumns> kColumnX{0.0F, 148.8F, 298.85F};
        constexpr std::array<float, kp::kRows> kRowY{0.0F, 138.55F, 277.1F, 423.85F};
        constexpr float kFaceInset = 12.0F;
        // The keypad body around the grid, its title and code readout, and the hint bar below.
        constexpr float kBodyX0 = 690.0F, kBodyX1 = 1232.0F, kBodyY0 = 150.0F, kBodyY1 = 972.0F;
        constexpr float kCentreX = (kBodyX0 + kBodyX1) / 2;
        constexpr float kReadoutY0 = 236.0F, kReadoutY1 = 336.0F;
        // The glow's falloff per stage unit off the ring, outside and inside, read off the Ruffle render
        // (selector-zoom.png); Flash's 3-pass blur 6 at strength 0.25 fades out within 6 units.
        constexpr std::array<float, 6> kGlowOutside{0.55F, 0.21F, 0.14F, 0.08F, 0.04F, 0.02F};
        constexpr std::array<float, 6> kGlowInside{0.42F, 0.18F, 0.12F, 0.07F, 0.035F, 0.015F};
        ImVec2 Cell(int key) {
            return {kGridX + kColumnX[static_cast<std::size_t>(key % kp::kColumns)], kGridY + kRowY[static_cast<std::size_t>(key / kp::kColumns)]};
        }

        void Rect(float x0, float y0, float x1, float y1, ImU32 color) { Canvas()->AddRectFilled(Stage(x0, y0), Stage(x1, y1), color); }

        void Ring(float x, float y, ImU32 color) {
            Rect(x, y, x + kCell, y + kRing, color);
            Rect(x, y + kCell - kRing, x + kCell, y + kCell, color);
            Rect(x, y + kRing, x + kRing, y + kCell - kRing, color);
            Rect(x + kCell - kRing, y + kRing, x + kCell, y + kCell - kRing, color);
        }

        void Selector(ImVec2 cell) {
            const ImU32 gold = theme::Hud(theme::Gold) & ~IM_COL32_A_MASK;
            const auto outline = [&](float inset, float alpha) {
                const auto a = static_cast<ImU32>(alpha * 255.0F + 0.5F) << IM_COL32_A_SHIFT;
                Canvas()->AddRect(Stage(cell.x + inset, cell.y + inset), Stage(cell.x + kCell - inset, cell.y + kCell - inset), gold | a, 0.0F, 0, Px(1.0F));
            };
            for (std::size_t d = 0; d < kGlowOutside.size(); ++d) {
                outline(-0.5F - static_cast<float>(d), kGlowOutside[d]);
                outline(kRing - 0.5F + static_cast<float>(d), kGlowInside[d]);
            }
            Ring(cell.x, cell.y, theme::Hud(theme::Ink));
        }
        void ReadInput(KeypadModel& model, KeypadOutcome& outcome) {
            outcome = fusionui::styles::controls::KeypadKeys(model);
            const auto press = [&](kp::Key key) {
                if (auto result = model.Press(key); result.kind != KeypadOutcome::Kind::None) outcome = std::move(result);
            };
            const auto delta = ImGui::GetIO().MouseDelta;
            for (int key = 0; key < kp::kKeys; ++key) {
                const auto cell = Cell(key);
                if (!ImGui::IsMouseHoveringRect(Stage(cell.x, cell.y), Stage(cell.x + kCell, cell.y + kCell))) continue;
                if ((delta.x != 0.0F || delta.y != 0.0F) && !model.pending) model.selected = key;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !model.pending) {
                    model.selected = key;
                    press(kp::Key::Select);
                }
            }
        }
    }

    KeypadOutcome Keypad(KeypadModel& model) {
        KeypadOutcome outcome;
        ReadInput(model, outcome);

        Rect(kBodyX0, kBodyY0, kBodyX1, kBodyY1, theme::MenuPanel);
        Canvas()->AddRect(Stage(kBodyX0, kBodyY0), Stage(kBodyX1, kBodyY1), (theme::Hud(theme::Ink) & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 102),
                          0.0F, 0, Px(1.0F));
        ShadowText(kCentreX, 172.0F, 30.0F, model.title, theme::Hud(theme::Gold), true, 0.0F, Align::Center);
        Rect(kGridX, kReadoutY0, kGridX + kColumnX.back() + kCell, kReadoutY1, IM_COL32(0, 0, 0, 255));
        ShadowText(kCentreX, kReadoutY0 + 14.0F, 60.0F, model.Readout(), theme::Hud(theme::Ink), true, 0.0F, Align::Center);
        if (model.pending) ShadowText(kCentreX, kReadoutY1 + 6.0F, 22.0F, "SUBMITTING...", theme::Hud(theme::Ink), false, 0.0F, Align::Center);

        for (int key = 0; key < kp::kKeys; ++key) {
            const auto cell = Cell(key);
            const float x0 = cell.x + kFaceInset, y0 = cell.y + kFaceInset, x1 = cell.x + kCell - kFaceInset, y1 = cell.y + kCell - kFaceInset;
            Rect(x0, y0, x1, y1, theme::CardHeader);
            Canvas()->AddRect(Stage(x0, y0), Stage(x1, y1), (theme::Hud(theme::Ink) & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 64), 0.0F, 0, Px(1.0F));
            const auto label = kp::kLabels[static_cast<std::size_t>(key)];
            const float size = label.size() > 1 ? 28.0F : 44.0F;
            ShadowText(cell.x + kCell / 2, cell.y + (kCell - size * 1.2F) / 2, size, label, theme::Hud(theme::Ink), true, 0.0F, Align::Center);
        }
        Selector(Cell(model.selected));

        const bool open = !model.pending;
        const std::array<fo4::Hint, 3> hints{{{FUSIONUI_PAD_Y, "ENTER", "Submit", open},
                                              {FUSIONUI_PAD_X, "BACKSPACE", "Delete", open},
                                              {FUSIONUI_PAD_B, "TAB", "Exit"}}};
        const auto clicked = fo76::HintBar(hints, 985.0F, kCentreX);
        const std::array<kp::Key, 3> actions{kp::Key::Submit, kp::Key::Delete, kp::Key::Cancel};
        if (clicked >= 0)
            if (auto result = model.Press(actions[static_cast<std::size_t>(clicked)]); result.kind != KeypadOutcome::Kind::None) outcome = std::move(result);
        return outcome;
    }
}
