#include "styles/common/Menus.h"
#include "styles/Controls.h"
#include "styles/Frames.h"
#include "styles/PhotoOverlays.h"

namespace fusionui::styles::menus {
    namespace {
        using namespace hudDraw;
        using namespace controls;
        using modern::theme::Family;
        Family CurrentFamily() { return modern::theme::CurrentAppearance().family; }
        bool Classic() { return CurrentFamily() == Family::Fallout3 || CurrentFamily() == Family::NewVegas; }
        float Width() { return kit::CurrentStage().w / Scale(); }

        void Selected(Rect box, ImU32 ink) {
            if (CurrentFamily() == Family::Fallout4) Box(box.x, box.y, box.width, box.height, ink);
            else if (Classic()) {
                Box(box.x, box.y, box.width, box.height, Fade(ink, 40.0F / 255));
                kit::Canvas()->AddRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height), ink, 0, 0, Px(1));
            } else if (CurrentFamily() == Family::Skyrim) {
                Line(box.x + 6, box.y, box.x + box.width - 6, box.y, Fade(ink, 0.6F));
                Line(box.x + 6, box.y + box.height, box.x + box.width - 6, box.y + box.height, Fade(ink, 0.6F));
                frames::Diamond(box.x, box.y + box.height / 2, 3, ink);
                frames::Diamond(box.x + box.width, box.y + box.height / 2, 3, ink);
            } else {
                Box(box.x, box.y, box.width, box.height, Fade(ink, 0.15F));
                Box(box.x, box.y, 2, box.height, ink);
            }
        }
    }

    screens::KeypadOutcome Keypad(screens::KeypadModel& model) {
        namespace keypad = core::keypad;
        auto outcome = KeypadKeys(model);
        const auto press = [&](keypad::Key key) {
            auto result = model.Press(key);
            if (result.kind != screens::KeypadOutcome::Kind::None) outcome = std::move(result);
        };
        const auto ink = common::HudColor();
        const bool skyrim = CurrentFamily() == Family::Skyrim;
        const float width = skyrim ? 410.0F : Classic() ? 400.0F : 360.0F;
        const Rect body{(Width() - width) / 2, skyrim ? 130.0F : 100.0F, width, skyrim ? 410.0F : 500.0F};
        Box(body.x, body.y, body.width, body.height, IM_COL32(0, 0, 0, 210));
        frames::Panel(body.x, body.y, body.width, body.height, ink);
        Text(Width() / 2, body.y + 18, 22, model.title, ink, kit::Align::Center, true, width - 35);
        Box(body.x + 32, body.y + 58, width - 64, 65, IM_COL32(0, 0, 0, 200));
        Text(Width() / 2, body.y + 66, 40, model.Readout(), ink, kit::Align::Center, true, width - 80);
        if (model.pending) Text(Width() / 2, body.y + 125, 12, "SUBMITTING...", ink, kit::Align::Center);
        const float cellW = skyrim ? 84.0F : 76.0F, cellH = skyrim ? 48.0F : 58.0F;
        const float pitchX = cellW + 8, pitchY = cellH + 8, gridX = (Width() - 3 * pitchX + 8) / 2;
        const float gridY = body.y + 152;
        for (int key = 0; key < keypad::kKeys; ++key) {
            const Rect cell{gridX + (key % 3) * pitchX, gridY + (key / 3) * pitchY, cellW, cellH};
            if (!model.pending && Hover(cell)) {
                const auto delta = ImGui::GetIO().MouseDelta;
                if (delta.x || delta.y) model.selected = key;
                if (Click(cell)) { model.selected = key; press(keypad::Key::Select); }
            }
            const bool selected = key == model.selected;
            Box(cell.x, cell.y, cell.width, cell.height, IM_COL32(0, 0, 0, 110));
            if (selected) Selected(cell, Fade(ink, model.pending ? 0.4F : 1.0F));
            const auto color = CurrentFamily() == Family::Fallout4 && selected ? IM_COL32_BLACK : Fade(ink, model.pending ? 0.35F : 1.0F);
            const auto label = keypad::kLabels[static_cast<std::size_t>(key)];
            const float size = label.size() > 1 ? 17.0F : 27.0F;
            Text(cell.x + cellW / 2, cell.y + (cellH - size) / 2 - 2, size, label, color, kit::Align::Center);
        }
        const std::array<fo4::Hint, 3> hints{{{FUSIONUI_PAD_Y, "Enter", "Submit", !model.pending},
            {FUSIONUI_PAD_X, "Backspace", "Delete", !model.pending}, {FUSIONUI_PAD_B, "Tab", "Exit"}}};
        const int clicked = Hints(hints, body.y + body.height - 35, ink);
        if (clicked >= 0) press(clicked == 0 ? keypad::Key::Submit : clicked == 1 ? keypad::Key::Delete : keypad::Key::Cancel);
        return outcome;
    }

    void PhotoMode(const screens::PhotoModeView& view) {
        fusionui::styles::PhotoOverlays(view);
        const auto& model = view.model;
        if (!model.visible) return;
        const auto ink = common::HudColor();
        const bool starfield = CurrentFamily() == Family::Starfield;
        const float width = starfield ? 555.0F / 1.5F : Classic() ? 410.0F : 350.0F;
        const float x = starfield ? 1332.0F / 1.5F : Width() - width - 30;
        const float top = starfield ? 632.0F / 1.5F : Classic() ? 325.0F : 350.0F;
        const float rowTop = starfield ? 695.0F / 1.5F : top + 55;
        const float pitch = starfield ? 38.0F / 1.5F : 35;
        const float font = starfield ? 18.0F / 1.5F : 18;
        frames::Panel(x, top, width, 648 - top, ink);
        const float tabWidth = (width - 20) / core::photo::kPanelCount;
        for (int tab = 0; tab < core::photo::kPanelCount; ++tab) {
            const float tx = x + 10 + tabWidth * tab;
            if (tab == model.panel) Selected({tx, top + 6, tabWidth, 33}, ink);
            Icon(screens::kPhotoTabIcons[static_cast<std::size_t>(tab)], tx + tabWidth / 2, top + 22, 22,
                 CurrentFamily() == Family::Fallout4 && tab == model.panel ? IM_COL32_BLACK : Fade(ink, tab == model.panel ? 1.0F : 0.6F));
        }
        const auto& panel = model.Current();
        const int count = static_cast<int>(panel.controls.size()), visible = std::max(1, static_cast<int>((638 - rowTop) / pitch));
        const int first = std::clamp(panel.selected - visible + 1, 0, std::max(0, count - visible));
        kit::Canvas()->PushClipRect(At(x + 5, rowTop), At(x + width - 5, 641), true);
        for (int i = first; i < count && i < first + visible; ++i) {
            const auto& control = panel.controls[static_cast<std::size_t>(i)];
            const Rect row{starfield ? 1333.0F / 1.5F : x + 6, rowTop + (i - first) * pitch,
                           starfield ? 529.0F / 1.5F : width - 12, starfield ? 35.0F / 1.5F : pitch - 1};
            const bool selected = i == panel.selected;
            const auto color = Fade(CurrentFamily() == Family::Fallout4 && selected ? IM_COL32_BLACK : ink, control.active ? 1.0F : 0.25F);
            if (selected) Selected(row, Fade(ink, control.active ? 1.0F : 0.25F));
            const float valueWidth = starfield ? 160.0F : 140.0F, right = x + width - 18;
            Text(row.x + 8 + (control.sub ? 20 : 0), row.y + (starfield ? 10.0F / 1.5F : 5), font, control.label, color, kit::Align::Left, true, width - valueWidth - 45 - (control.sub ? 20 : 0));
            if (control.slider) {
                Line(right - valueWidth, row.y + pitch / 2, right, row.y + pitch / 2, color, starfield ? 4.0F : 2.0F);
                const float knob = right - valueWidth + valueWidth * std::clamp(control.Fraction(), 0.0F, 1.0F);
                Box(knob - 2, row.y + pitch / 2 - 6, 4, 12, color);
            } else {
                Text(right - valueWidth / 2, row.y + 5, font, control.Shown(), color, kit::Align::Center, true, valueWidth - 30);
                if (control.Arrows()) {
                    Text(right - valueWidth, row.y + 4, font, "<", color);
                    Text(right, row.y + 4, font, ">", color, kit::Align::Right);
                }
            }
        }
        kit::Canvas()->PopClipRect();
        const std::array<fo4::Hint, 4> hints{{{FUSIONUI_PAD_A, "Space", view.hints[0].c_str()}, {FUSIONUI_PAD_X, "T", view.hints[1].c_str()},
            {FUSIONUI_PAD_Y, "R", view.hints[2].c_str()}, {FUSIONUI_PAD_B, "Tab", view.hints[3].c_str()}}};
        if (starfield) Hints(hints, 995.0F / 1.5F, ink, false, 1870.0F / 1.5F, kit::Align::Right);
        else Hints(hints, 672, ink, false);
    }
}
