#include "styles/common/Menus.h"
#include "styles/Controls.h"
#include "styles/Frames.h"
#include "core/MenuStyles.h"
#include "fusionui/fo76/Theme.h"
#include <numbers>

namespace fusionui::styles::menus {
    namespace {
        using namespace hudDraw;
        using namespace controls;
        using modern::theme::Family;
        namespace view = screens::mainmenu;
        namespace layout = core::mainmenu;
        Family CurrentFamily() { return modern::theme::CurrentAppearance().family; }
        bool Classic() { return CurrentFamily() == Family::Fallout3 || CurrentFamily() == Family::NewVegas; }
        float Width() { return ImGui::GetIO().DisplaySize.x / Px(1); }

        // A solid plate's drop shadow, under the plate.
        void PlateShadow(Rect box, ImU32 ink) {
            fo76::ShadowLayers(kMenuShadow, static_cast<float>(ink >> IM_COL32_A_SHIFT) / 255, [&](ImVec2 offset, ImU32 shade) {
                kit::Canvas()->AddRectFilled({At(box.x, box.y).x + offset.x, At(box.x, box.y).y + offset.y},
                                             {At(box.x + box.width, box.y + box.height).x + offset.x, At(box.x + box.width, box.y + box.height).y + offset.y}, shade);
            });
        }

        void Mark(Rect box, ImU32 ink) {
            if (CurrentFamily() == Family::Fallout4) {
                PlateShadow(box, ink);
                Box(box.x, box.y, box.width, box.height, ink);
            }
            else if (Classic()) {
                Box(box.x, box.y, box.width, box.height, Fade(ink, 40.0F / 255));
                kit::Canvas()->AddRect(At(box.x, box.y), At(box.x + box.width, box.y + box.height), ink, 0, 0, Px(1));
            } else if (CurrentFamily() == Family::Skyrim) {
                Line(box.x + 6, box.y + box.height - 2, box.x + box.width - 6, box.y + box.height - 2, ink);
                frames::Diamond(box.x + box.width, box.y + box.height / 2, 3, ink);
            } else {
                Box(box.x, box.y, box.width, box.height, Fade(ink, 0.15F));
                Box(box.x, box.y, 2, box.height, ink);
            }
        }

        void Spinner(float x, float y, float radius, float seconds, ImU32 ink) {
            const int phase = static_cast<int>(std::floor(std::fmod(std::max(0.0F, seconds), 1.0F) * 12));
            for (int i = 0; i < 12; ++i) {
                const float angle = static_cast<float>(i) * std::numbers::pi_v<float> / 6;
                const float alpha = 0.15F + static_cast<float>((i + 12 - phase) % 12) / 12 * 0.85F;
                Line(x + std::cos(angle) * radius * 0.55F, y + std::sin(angle) * radius * 0.55F,
                     x + std::cos(angle) * radius, y + std::sin(angle) * radius, Fade(ink, alpha), 2);
            }
        }

        float Paragraph(float x, float y, float font, float width, float height, std::string_view text, ImU32 ink, bool bold, kit::Align align = kit::Align::Left) {
            for (int i = 0; i < 4; ++i) {
                const float needed = WrappedHeight(font, width, text, bold);
                if (needed <= height) break;
                font *= height / needed;
            }
            return Wrapped(x, y, font, width, text, ink, bold, align);
        }
    }

    std::vector<layout::Box> RowBoxes(const view::List& list) {
        const bool title = list.kind == view::Kind::Title;
        auto boxes = core::menus::MainBoxes(CurrentFamily(), title, static_cast<int>(list.rows.size()), list.selected, Width());
        // FO3 / FNV box each row around its text.
        if (Classic()) for (std::size_t i = 0; i < boxes.size(); ++i)
            boxes[i].x1 = boxes[i].x0 + std::min(520.0F, TextWidth(19, list.rows[i].label, false) + 40 + (list.rows[i].image.empty() ? 0 : 26));
        const float scale = Scale();
        for (auto& box : boxes) { box.x0 *= scale; box.x1 *= scale; box.y0 *= scale; box.y1 *= scale; }
        return boxes;
    }

    view::ListResult DrawList(const view::List& list) {
        Anchor(Edge::Left);
        ShadowScope soft(TextShadow::Soft);
        const auto ink = common::HudColor();
        const bool title = list.kind == view::Kind::Title;
        const auto metrics = core::menus::MainMetrics(CurrentFamily(), title, static_cast<int>(list.rows.size()), Width());
        const auto boxes = fusionui::styles::menus::RowBoxes(list);
        view::ListResult result;
        kit::Canvas()->PushClipRect(At(0, metrics.top), At(Width(), 670), true);
        for (std::size_t i = 0; i < list.rows.size(); ++i) {
            const auto& row = list.rows[i];
            const auto& box = boxes[i];
            const Rect rect{box.x0 / Scale(), box.y0 / Scale(), (box.x1 - box.x0) / Scale(), (box.y1 - box.y0) / Scale()};
            if (rect.y < metrics.top || rect.y + rect.height > 670.01F) continue;
            if (Hover(rect)) { result.hovered = static_cast<int>(i); if (!row.disabled && Click(rect)) result.clicked = static_cast<int>(i); }
            const bool selected = static_cast<int>(i) == list.selected;
            const bool skyrim = CurrentFamily() == Family::Skyrim;
            float font = metrics.font;
            if (skyrim && !selected) font = title ? 20.0F : 16.0F;
            // Starfield's MainPanelListEntry: every row on a 20% plate; the selected one solid with black text.
            const bool starfield = CurrentFamily() == Family::Starfield;
            const bool black = (CurrentFamily() == Family::Fallout4 || starfield) && selected;
            const auto color = Fade(black ? IM_COL32_BLACK : ink, row.disabled ? 0.3F : row.minor ? 0.8F : 1.0F);
            if (starfield && selected) PlateShadow({rect.x, rect.y, rect.width, rect.height - 1}, Fade(ink, row.disabled ? 0.3F : 1.0F));
            if (starfield) Box(rect.x, rect.y, rect.width, rect.height - 1, Fade(ink, selected ? (row.disabled ? 0.3F : 1.0F) : 0.2F));
            else if (selected) Mark(rect, Fade(ink, row.disabled ? 0.3F : 1.0F));
            float left = rect.x + 12, right = rect.x + rect.width - 12;
            if (!row.image.empty()) { Icon(row.image, left + 8, rect.y + rect.height / 2, 16, color); left += 26; }
            if (row.busy) { Spinner(right - 8, rect.y + rect.height / 2, 8, static_cast<float>(ImGui::GetTime()), color); right -= 25; }
            Text(left, rect.y + (rect.height - font) / 2 - 1, font, row.label, color, kit::Align::Left,
                 skyrim ? selected : CurrentFamily() == Family::Starfield, right - left);
        }
        kit::Canvas()->PopClipRect();
        return result;
    }

    view::SettingsResult DrawSettings(const view::SettingsPage& page, const fo4::Nav& nav) {
        Anchor(Edge::Left);
        ShadowScope soft(TextShadow::Soft);
        const auto family = CurrentFamily();
        const auto ink = common::HudColor();
        const bool starfield = family == Family::Starfield, skyrim = family == Family::Skyrim;
        const float catX = Classic() ? Width() - 369 : starfield ? 140.0F / 1.5F : skyrim ? 167.1F : 120;
        const float catTop = Classic() ? 180.0F : starfield ? 383.0F / 1.5F : skyrim ? 159.3F : 200;
        const float catW = Classic() ? 155.0F : starfield ? 440.0F / 1.5F : 220.0F;
        const Rect options = Classic() ? Rect{Width() - 919, 100, 520, 520} : starfield ? Rect{650.0F / 1.5F, 146.0F / 1.5F, 760.0F / 1.5F, 520} :
                             skyrim ? Rect{442.1F, 179.3F, 700, 440} : Rect{471.4F, 347.8F, 550, 280};
        const float pitch = Classic() ? 48.0F : starfield ? 36.0F / 1.5F : skyrim ? 40.0F : 29.05F;
        const float font = Classic() ? 19.0F : starfield ? 22.0F / 1.5F : skyrim ? 22.0F : 19.0F;
        const int catVisible = page.category < 0 ? 0 : std::max(1, static_cast<int>((640 - catTop) / 40));
        const int catFirst = std::clamp(page.category - catVisible + 1, 0, std::max(0, static_cast<int>(page.categories.size()) - catVisible));
        if (page.category < 0) {
            // A save's details: one fixed-height box, the screenshot over its lines.
            const Rect box{options.x - 330, options.y, 310, 300};
            frames::Panel(box.x, box.y, box.width, box.height, ink);
            const float pw = box.width - 16, ph = pw * 9 / 16;
            if (page.preview && page.previewWidth > 0 && page.previewHeight > 0) {
                const float scale = std::min(pw / page.previewWidth, ph / page.previewHeight);
                const float w = page.previewWidth * scale, h = page.previewHeight * scale;
                const float px = box.x + 8 + (pw - w) / 2, py = box.y + 8 + (ph - h) / 2;
                kit::Canvas()->AddImage(page.preview, At(px, py), At(px + w, py + h));
            }
            float y = box.y + 16 + ph;
            for (const auto& line : page.categories) {
                if (y + font > box.y + box.height - 6) break;
                Text(box.x + 12, y, font, line, ink, kit::Align::Left, false, box.width - 24);
                y += font + 5;
            }
        }
        for (int i = catFirst; i < static_cast<int>(page.categories.size()) && i < catFirst + catVisible; ++i) {
            const Rect row{catX, catTop + (i - catFirst) * 40, catW, 38};
            if (i == page.category) Mark(row, ink);
            Text(row.x + row.width - 10, row.y + 5, skyrim && i == page.category ? 27.0F : font, page.categories[static_cast<std::size_t>(i)],
                 family == Family::Fallout4 && i == page.category ? IM_COL32_BLACK : Fade(ink, i == page.category ? 1.0F : 0.6F), kit::Align::Right, true, row.width - 20);
        }
        std::vector<bool> disabled;
        for (const auto& row : page.rows) disabled.push_back(row.disabled);
        const int count = static_cast<int>(page.rows.size()), visible = std::max(1, static_cast<int>(options.height / pitch));
        view::SettingsResult result{layout::StepSelection(page.selected, nav.dy, disabled), page.scroll};
        if (Hover(options) && ImGui::GetIO().MouseWheel) result.selected = layout::StepSelection(result.selected, ImGui::GetIO().MouseWheel > 0 ? -1 : 1, disabled);
        result.scroll = layout::ScrollFor(result.selected, page.scroll, count, visible);
        kit::Canvas()->PushClipRect(At(options.x, options.y), At(options.x + options.width, options.y + options.height), true);
        for (int i = result.scroll; i < count && i < result.scroll + visible; ++i) {
            const auto& row = page.rows[static_cast<std::size_t>(i)];
            const Rect box{options.x, options.y + (i - result.scroll) * pitch, options.width, pitch - 1};
            const auto delta = ImGui::GetIO().MouseDelta;
            if (!row.disabled && Hover(box) && (delta.x || delta.y)) result.selected = i;
            if (!row.disabled && row.kind == view::Setting::Kind::Info && Click(box)) result.pressed = i;
            const bool selected = i == result.selected;
            if (selected) Mark(box, Fade(ink, row.disabled ? 0.3F : 1.0F));
            const auto color = Fade(family == Family::Fallout4 && selected ? IM_COL32_BLACK : ink, row.disabled ? 0.3F : 1.0F);
            Text(box.x + 10, box.y + 4, font, row.label, color, kit::Align::Left, false, box.width * 0.57F - 20);
            const float valueX = box.x + box.width * 0.79F, valueW = box.width * 0.35F;
            if (row.kind == view::Setting::Kind::Slider) {
                frames::Bar(valueX - valueW / 2, box.y + pitch / 2 - 2, valueW, 4, row.fraction, color);
                continue;
            }
            const bool toggle = row.kind == view::Setting::Kind::Toggle;
            const auto value = toggle ? std::string_view(row.value ? page.onLabel : page.offLabel) :
                row.value >= 0 && row.value < static_cast<int>(row.options.size()) ? std::string_view(row.options[static_cast<std::size_t>(row.value)]) : std::string_view{};
            Text(valueX, box.y + 4, font, value, color, kit::Align::Center, true, valueW - 30);
            int change = selected ? nav.dx : 0;
            if (row.kind == view::Setting::Kind::Stepper) {
                Text(valueX - valueW / 2, box.y + 3, font, "<", color);
                Text(valueX + valueW / 2, box.y + 3, font, ">", color, kit::Align::Right);
                if (Click({valueX - valueW / 2 - 5, box.y, 25, box.height})) change = -1;
                if (Click({valueX + valueW / 2 - 20, box.y, 25, box.height})) change = 1;
            }
            int updated = layout::StepValue(row.value, change, toggle ? 2 : static_cast<int>(row.options.size()));
            if (toggle && ((selected && nav.accept) || Click(box))) updated = row.value ? 0 : 1;
            if (!row.disabled && updated != row.value) { result.changed = i; result.value = updated; }
        }
        kit::Canvas()->PopClipRect();
        if (!page.prompt.empty()) {
            // The question sits on a plate across the middle of the list; the game's hint bar answers it.
            const Rect plate{options.x, options.y + options.height / 2 - 24, options.width, 48};
            Box(plate.x, plate.y, plate.width, plate.height, IM_COL32(0, 0, 0, 225));
            kit::Canvas()->AddRect(At(plate.x, plate.y), At(plate.x + plate.width, plate.y + plate.height), ink, 0, 0, Px(1));
            Text(plate.x + plate.width / 2, plate.y + 12, font + 3, page.prompt, ink, kit::Align::Center, true, plate.width - 20);
        }
        result.back = nav.cancel;
        result.hint = Hints(page.hints, 667, ink, true, Width() / 2);
        if (result.hint >= 0 && page.hints[static_cast<std::size_t>(result.hint)].padButton == FUSIONUI_PAD_B) result.back = true;
        return result;
    }

    void DrawLoading(const view::Loading& loading) {
        const auto display = ImGui::GetIO().DisplaySize;
        const auto family = CurrentFamily();
        const bool starfield = family == Family::Starfield;
        const auto fit = loading.image && !loading.minimal && !loading.spinnerOnly ?
            (starfield ? core::menus::PhotoFit(display.x, display.y, loading.imageWidth, loading.imageHeight) : layout::CoverRect(display.x, display.y, loading.imageWidth, loading.imageHeight)) : std::nullopt;
        if (loading.minimal || (starfield && fit))
            kit::Canvas()->AddRectFilled({0, 0}, display, IM_COL32_BLACK);
        if (fit) kit::Canvas()->AddImage(loading.image, {fit->x, fit->y}, {fit->x + loading.imageWidth * fit->scale, fit->y + loading.imageHeight * fit->scale});
        Anchor(Edge::Left);
        if (loading.minimal) return;
        ShadowScope soft(TextShadow::Soft);
        const auto ink = common::HudColor();
        if (!loading.spinnerOnly) {
            if (starfield) {
                kit::Canvas()->AddRectFilledMultiColor(At(0, 533), At(Width(), 720), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 225), IM_COL32(0, 0, 0, 225));
                Paragraph(99.25F / 1.5F, 931.7F / 1.5F, 28.0F / 1.5F, 1312.95F / 1.5F, 108.75F / 1.5F, loading.tip, ink, true);
                Text(193.3F / 1.5F, 871.6F / 1.5F, 16, loading.level, ink, kit::Align::Left, true, 600);
                Text(Width() - 85, 638, 14, loading.loadingLabel, ink, kit::Align::Right, true, 200);
            } else if (family == Family::Skyrim) {
                Paragraph(Width() - 48, 475, 22, 500, 180, loading.tip, ink, false, kit::Align::Right);
                Text(Width() - 48, 52, 24, loading.level, ink, kit::Align::Right, true, 350);
                Text(Width() - 90, 669, 16, loading.loadingLabel, ink, kit::Align::Right, false, 300);
            } else if (family == Family::Fallout4) {
                const float height = std::min(160.0F, WrappedHeight(19, 505, loading.tip, true));
                const float top = 704 - height;
                Paragraph(152, top, 19, 505, 160, loading.tip, ink, true);
                Text(141, top - 29, 17, loading.loadingLabel, ink, kit::Align::Left, false, 505);
                Text(141, top - 73, 19, loading.level, ink, kit::Align::Left, true, 505);
            } else {
                frames::Panel(48, 535, Width() - 96, 160, ink);
                Paragraph(70, 575, 19, Width() - 200, 100, loading.tip, ink, false);
                Text(70, 548, 18, loading.level, ink, kit::Align::Left, false, 400);
                Text(Width() - 90, 548, 18, loading.loadingLabel, ink, kit::Align::Right, false, 400);
            }
        }
        Spinner(Width() - (starfield ? 53.5F : 52), starfield ? 645.5F : 677, Classic() ? 18.0F : 13.0F, loading.seconds, ink);
    }

    void DrawDeath(const screens::death::Model& model, float alpha) {
        Anchor(Edge::Center);
        const auto ink = Fade(fo76::theme::Hud(model.ink), alpha), title = Fade(fo76::theme::Hud(model.titleColor), alpha);
        const bool ready = model.seconds <= 0;
        const bool skyrim = CurrentFamily() == Family::Skyrim;
        const float top = skyrim ? 420.0F : 535.0F, width = skyrim ? 410.0F : 420.0F;
        if (CurrentFamily() != Family::Fallout4) frames::Panel((1280 - width) / 2, top, width, ready ? 70.0F : 150.0F, ink);
        if (ready) {
            const fo4::Hint hint{FUSIONUI_PAD_A, "Space", model.hint.c_str()};
            Hints({&hint, 1}, top + 22, ink, false);
        } else {
            Text(640, top + 14, 28, model.title, title, kit::Align::Center, true, width - 30);
            Text(640, top + 63, 48, core::death::CountdownText(model.seconds), ink, kit::Align::Center);
        }
    }
}
