#pragma once
#include "styles/HudDraw.h"
#include "fusionui/screens/Keypad.h"
#include <span>
#include <optional>
#include <vector>

namespace fusionui::styles::controls {
    using namespace hudDraw;
    struct Rect { float x, y, width, height; };
    inline bool Hover(Rect rect) { return ImGui::IsMouseHoveringRect(At(rect.x, rect.y), At(rect.x + rect.width, rect.y + rect.height)); }
    inline bool Click(Rect rect) { return Hover(rect) && ImGui::IsMouseClicked(ImGuiMouseButton_Left); }

    inline int Hints(std::span<const fo4::Hint> hints, float y, ImU32 ink, bool interactive = true, float anchor = -1, fo4::Align align = fo4::Align::Center, std::optional<bool> gamepad = std::nullopt) {
        const bool pad = gamepad.value_or(common::ActiveDevice() == FUSIONUI_DEVICE_GAMEPAD);
        std::vector<float> widths;
        float total = 0;
        for (const auto& hint : hints) {
            const float key = pad && hint.padButton != common::kNoPad ? common::PadGlyphWidth(Px(9), hint.padButton) / Px(1) : TextWidth(13, hint.key ? hint.key : "", false) + 8;
            const float width = key + TextWidth(14, hint.label ? hint.label : "", false) + 26;
            widths.push_back(width);
            total += width;
        }
        const float fit = total > (kit::CurrentStage().w / Scale()) - 40 ? ((kit::CurrentStage().w / Scale()) - 40) / total : 1;
        float x = (anchor < 0 ? (kit::CurrentStage().w / Scale()) / 2 : anchor) - total * fit * (align == fo4::Align::Right ? 1.0F : align == fo4::Align::Center ? 0.5F : 0.0F);
        int clicked = -1;
        for (std::size_t i = 0; i < hints.size(); ++i) {
            const auto& hint = hints[i];
            const float width = widths[i] * fit;
            const auto color = Fade(ink, hint.enabled ? 1.0F : 0.35F);
            const Rect box{x, y, width - 5 * fit, 29 * fit};
            if (hint.enabled && interactive && Hover(box)) Box(x, y, box.width, box.height, Fade(color, 0.15F));
            const float flash = hint.flashing ? kit::HintFlash(ImGui::GetTime()) : 1;
            float keyWidth;
            if (pad && hint.padButton != common::kNoPad) {
                keyWidth = common::PadGlyphWidth(Px(9 * fit), hint.padButton) / Px(1);
                common::PadGlyph(kit::Canvas(), At(x + keyWidth / 2 + 3 * fit, y + 14 * fit), Px(9 * fit), hint.padButton, Fade(color, flash));
            } else {
                const auto key = std::string_view(hint.key ? hint.key : "");
                keyWidth = TextWidth(13 * fit, key, false) + 8 * fit;
                Text(x + 4 * fit, y + 6 * fit, 13 * fit, key, Fade(color, flash), Align::Left, false);
                Line(x, y + 3 * fit, x, y + 25 * fit, color);
                Line(x + keyWidth, y + 3 * fit, x + keyWidth, y + 25 * fit, color);
            }
            Text(x + keyWidth + 9 * fit, y + 5 * fit, 14 * fit, hint.label ? hint.label : "", color, Align::Left, false);
            if (hint.enabled && interactive && Click(box)) clicked = static_cast<int>(i);
            x += width;
        }
        return clicked;
    }

    inline screens::KeypadOutcome KeypadKeys(screens::KeypadModel& model) {
        namespace kp = core::keypad;
        screens::KeypadOutcome outcome;
        const auto press = [&](kp::Key key) {
            if (auto result = model.Press(key); result.kind != screens::KeypadOutcome::Kind::None) outcome = std::move(result);
        };
        const auto once = [](std::initializer_list<ImGuiKey> keys) {
            for (const auto key : keys)
                if (ImGui::IsKeyPressed(key, false)) return true;
            return false;
        };
        const auto nav = fo4::ReadNav();
        if (nav.dy < 0) press(kp::Key::Up);
        if (nav.dy > 0) press(kp::Key::Down);
        if (nav.dx < 0) press(kp::Key::Left);
        if (nav.dx > 0) press(kp::Key::Right);
        for (int digit = 0; digit <= 9; ++digit)
            if (once({static_cast<ImGuiKey>(ImGuiKey_0 + digit), static_cast<ImGuiKey>(ImGuiKey_Keypad0 + digit)})) model.Digit(digit);
        if (once({ImGuiKey_Space, ImGuiKey_GamepadFaceDown})) press(kp::Key::Select);
        if (once({ImGuiKey_Backspace, ImGuiKey_GamepadFaceLeft})) press(kp::Key::Delete);
        if (once({ImGuiKey_Delete})) press(kp::Key::Clear);
        if (once({ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_GamepadFaceUp})) press(kp::Key::Submit);
        if (nav.cancel) press(kp::Key::Cancel);

        return outcome;
    }
}
