#include "fusionui/modern/Theme.h"

#include "fusionui/Common.h"
#include "fusionui/fo76/Style.h"

#include <algorithm>
#include <array>

namespace fusionui::modern::theme {
    namespace {
        struct ContextTheme {
            Fonts fonts;
            Fonts condensed;
            Fonts skyrim;
            Fonts starfield;
            Fonts terminal;
            float displayScale = 0.0F;
            float scale = 0.0F;
            float backgroundOpacity = -1.0F;
            float appliedWindowRounding = -1.0F;
            int accentRevision = -1;
            Family family = Family::Modern;
        };

        int accentRevision = 0;
        Family paletteFamily = Family::Modern;
        ImU32 paletteHud = 0;
        bool paletteSet = false;

        ContextTheme& Current() { return common::ContextSlot<ContextTheme>(); }

        // Font Awesome icons plus the arrows, stars and box glyphs the text fonts lack.
        // Font Awesome Solid and Brands share one code point space, so both merge into the text fonts.
        ImFont* Load(const std::filesystem::path& file, const std::filesystem::path& iconDir) {
            const std::array merge{common::FontMerge{iconDir / "Font_Awesome_7_Free-Solid-900.otf", {0.0F, 0.5F}},
                                   common::FontMerge{iconDir / "Font_Awesome_7_Brands-Regular-400.otf", {0.0F, 0.5F}},
                                   common::FontMerge{common::WindowsFont(L"seguisym.ttf")}};
            return common::LoadFont(file, iconDir.empty() ? std::span(merge).subspan(2) : std::span(merge));
        }

        ImVec4 Blend(ImU32 a, ImU32 b, float weight) {
            const auto x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
            return {x.x + (y.x - x.x) * weight, x.y + (y.y - x.y) * weight, x.z + (y.z - x.z) * weight, 1.0F};
        }

        ImU32 Mix(ImU32 a, ImU32 b, float weight) { return ImGui::ColorConvertFloat4ToU32(Blend(a, b, weight)); }

        ImU32 WithAlpha(ImU32 color, int alpha) {
            return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
        }

        // Dark picks are brightened so accent text stays readable on the dark surfaces.
        ImU32 Readable(float r, float g, float b) {
            float h{}, s{}, v{};
            ImGui::ColorConvertRGBtoHSV(std::clamp(r, 0.0F, 1.0F), std::clamp(g, 0.0F, 1.0F), std::clamp(b, 0.0F, 1.0F), h, s, v);
            v = std::max(v, 0.78F);
            ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
            // Saturated blues and reds stay dim at full value; lift them toward white until accent text reads on #101114.
            const auto luma = [&] { return 0.2126F * r + 0.7152F * g + 0.0722F * b; };
            for (int step = 0; step < 8 && luma() < 0.38F; ++step) {
                r += (1 - r) * 0.15F;
                g += (1 - g) * 0.15F;
                b += (1 - b) * 0.15F;
            }
            return ImGui::ColorConvertFloat4ToU32({r, g, b, 1.0F});
        }

        void AssignAccent(ImU32 accent, ImU32 onAccent) {
            constexpr ImU32 black = IM_COL32(0, 0, 0, 0xFF), white = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
            Accent = accent;
            AccentMuted = Mix(black, accent, 0.77F);
            AccentHover = Mix(accent, white, 0.2F);
            AccentSoft = WithAlpha(accent, 0x2E);
            OnAccent = onAccent;
        }

        void Statuses(ImU32 ok, ImU32 warn, ImU32 error) {
            Ok = ok;
            Warn = warn;
            Error = error;
            OkFill = WithAlpha(ok, 0x22);
            WarnFill = WithAlpha(warn, 0x22);
            ErrorFill = WithAlpha(error, 0x26);
        }

        void Palette(Family family, ImU32 hud) {
            constexpr ImU32 black = IM_COL32(0, 0, 0, 0xFF);
            const auto hudVec = ImGui::ColorConvertU32ToFloat4(hud);
            const auto readableHud = Readable(hudVec.x, hudVec.y, hudVec.z);
            Scrim = IM_COL32(0x00, 0x00, 0x00, 0x73);
            switch (family) {
            case Family::Fallout4:
                // FO4 menus: the HUD colour on black, everything else a shade of it (fo4::Fg).
                Background = Mix(black, hud, 0.04F);
                Sidebar = Mix(black, hud, 0.07F);
                Surface = Mix(black, hud, 0.11F);
                Input = Mix(black, hud, 0.16F);
                Border = Mix(black, hud, 0.45F);
                Text = hud;
                Muted = Mix(black, hud, 0.78F);
                Faint = Mix(black, hud, 0.52F);
                AssignAccent(hud, IM_COL32(0, 0, 0, 0xFF));
                AccentSoft = WithAlpha(hud, 0x40);
                Statuses(hud, IM_COL32(0xED, 0xC5, 0x75, 0xFF), IM_COL32(0xF2, 0x8A, 0x91, 0xFF));
                break;
            case Family::Fallout76: {
                const ImU32 panel = fo76::theme::Panel;
                Background = panel;
                Sidebar = WithAlpha(Mix(panel, black, 0.25F), static_cast<int>(panel >> IM_COL32_A_SHIFT));
                Surface = fo76::theme::CardBox;
                Input = fo76::theme::CardHeader;
                Border = WithAlpha(hud, 102);
                Text = hud;
                Muted = Mix(black, hud, 0.85F);
                Faint = fo76::theme::Disabled;
                AssignAccent(hud, fo76::theme::DarkOnGold);
                AccentHover = hud;
                Statuses(IM_COL32(0x79, 0xCD, 0x92, 0xFF), fo76::theme::Warning, fo76::theme::Fail);
                break;
            }
            case Family::Skyrim:
                Background = IM_COL32(0x0D, 0x0F, 0x11, 0xFF);
                Sidebar = IM_COL32(0x13, 0x16, 0x19, 0xFF);
                Surface = IM_COL32(0x1C, 0x20, 0x24, 0xFF);
                Input = IM_COL32(0x26, 0x2A, 0x2F, 0xFF);
                Border = IM_COL32(0x83, 0x88, 0x8B, 0xFF);
                Text = hud;
                Muted = Mix(black, hud, 0.85F);
                Faint = IM_COL32(0x86, 0x8A, 0x8D, 0xFF);
                AssignAccent(hud, IM_COL32(0x15, 0x18, 0x1B, 0xFF));
                AccentSoft = WithAlpha(Accent, 0x20);
                Statuses(IM_COL32(0xBA, 0xD1, 0xB2, 0xFF), IM_COL32(0xD9, 0xC7, 0x9A, 0xFF), IM_COL32(0xEA, 0x8D, 0x88, 0xFF));
                break;
            case Family::Starfield:
                Background = IM_COL32(0x0B, 0x16, 0x22, 0xFF);
                Sidebar = IM_COL32(0x10, 0x1E, 0x2D, 0xFF);
                Surface = IM_COL32(0x18, 0x2A, 0x3B, 0xFF);
                Input = IM_COL32(0x20, 0x35, 0x48, 0xFF);
                Border = IM_COL32(0x6A, 0x7D, 0x8D, 0xFF);
                Text = hud;
                Muted = Mix(black, hud, 0.85F);
                Faint = IM_COL32(0x85, 0x9A, 0xAB, 0xFF);
                AssignAccent(hud, IM_COL32(0x0B, 0x16, 0x22, 0xFF));
                Statuses(IM_COL32(0x8E, 0xD0, 0xC6, 0xFF), IM_COL32(0xE5, 0xC5, 0x7A, 0xFF), IM_COL32(0xF0, 0x8A, 0x81, 0xFF));
                break;
            case Family::Fallout3: {
                const ImU32 phosphor = hud;
                Background = IM_COL32(0x04, 0x0C, 0x07, 0xFF);
                Sidebar = IM_COL32(0x07, 0x16, 0x0C, 0xFF);
                Surface = IM_COL32(0x0A, 0x20, 0x10, 0xFF);
                Input = IM_COL32(0x0C, 0x2B, 0x16, 0xFF);
                Border = Mix(black, phosphor, 0.67F);
                Text = phosphor;
                Muted = Mix(black, phosphor, 0.78F);
                Faint = Mix(black, phosphor, 0.56F);
                AssignAccent(phosphor, Background);
                Statuses(phosphor, IM_COL32(0xE8, 0xD2, 0x74, 0xFF), IM_COL32(0xF5, 0x88, 0x75, 0xFF));
                break;
            }
            case Family::NewVegas: {
                const ImU32 amber = hud;
                Background = IM_COL32(0x12, 0x0B, 0x04, 0xFF);
                Sidebar = IM_COL32(0x20, 0x15, 0x09, 0xFF);
                Surface = IM_COL32(0x2C, 0x1D, 0x0C, 0xFF);
                Input = IM_COL32(0x39, 0x27, 0x10, 0xFF);
                Border = Mix(black, amber, 0.70F);
                Text = amber;
                Muted = Mix(black, amber, 0.83F);
                Faint = Mix(black, amber, 0.62F);
                AssignAccent(amber, Background);
                Statuses(IM_COL32(0xBA, 0xD7, 0x85, 0xFF), amber, IM_COL32(0xF2, 0x8A, 0x72, 0xFF));
                break;
            }
            default:
                Background = IM_COL32(0x10, 0x11, 0x14, 0xFF);
                Sidebar = IM_COL32(0x17, 0x18, 0x1C, 0xFF);
                Surface = IM_COL32(0x22, 0x24, 0x29, 0xFF);
                Input = IM_COL32(0x2B, 0x2E, 0x34, 0xFF);
                Border = IM_COL32(0x3A, 0x3E, 0x46, 0xFF);
                Text = IM_COL32(0xEC, 0xEE, 0xF2, 0xFF);
                Muted = IM_COL32(0xA6, 0xAE, 0xBA, 0xFF);
                Faint = IM_COL32(0x6E, 0x76, 0x82, 0xFF);
                // After the lift every accent is light enough that near-black text on it out-contrasts white.
                AssignAccent(readableHud, IM_COL32(0x14, 0x12, 0x0A, 0xFF));
                Statuses(IM_COL32(0x79, 0xCD, 0x92, 0xFF), IM_COL32(0xED, 0xC5, 0x75, 0xFF), IM_COL32(0xF2, 0x8A, 0x91, 0xFF));
                break;
            }
        }

        void Colors(float backgroundOpacity) {
            auto* c = ImGui::GetStyle().Colors;
            const auto hover = Blend(Input, Accent, 0.28F);
            c[ImGuiCol_WindowBg] = Vec(Background);
            c[ImGuiCol_WindowBg].w *= backgroundOpacity;
            c[ImGuiCol_ChildBg] = Vec(IM_COL32(0, 0, 0, 0));
            c[ImGuiCol_PopupBg] = Vec(Surface);
            c[ImGuiCol_Text] = Vec(Text);
            c[ImGuiCol_TextDisabled] = Vec(Muted);
            c[ImGuiCol_Border] = Vec(Border);
            c[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
            c[ImGuiCol_Separator] = Vec(Border);
            c[ImGuiCol_SeparatorHovered] = Vec(AccentMuted);
            c[ImGuiCol_SeparatorActive] = Vec(Accent);
            c[ImGuiCol_FrameBg] = Vec(Input);
            c[ImGuiCol_FrameBgHovered] = Blend(Input, Accent, 0.12F);
            c[ImGuiCol_FrameBgActive] = Blend(Input, Accent, 0.18F);
            c[ImGuiCol_CheckMark] = Vec(Accent);
            c[ImGuiCol_SliderGrab] = Vec(AccentMuted);
            c[ImGuiCol_SliderGrabActive] = Vec(Accent);
            c[ImGuiCol_Button] = Vec(Input);
            c[ImGuiCol_ButtonHovered] = hover;
            c[ImGuiCol_ButtonActive] = Vec(AccentMuted);
            c[ImGuiCol_Header] = Blend(Surface, Accent, 0.22F);
            c[ImGuiCol_HeaderHovered] = Blend(Surface, Accent, 0.12F);
            c[ImGuiCol_HeaderActive] = Blend(Surface, Accent, 0.30F);
            c[ImGuiCol_TitleBg] = Vec(Background);
            c[ImGuiCol_TitleBgActive] = Vec(Surface);
            c[ImGuiCol_MenuBarBg] = Vec(Background);
            c[ImGuiCol_Tab] = Vec(Surface);
            c[ImGuiCol_TabSelected] = Vec(AccentMuted);
            c[ImGuiCol_TabHovered] = hover;
            c[ImGuiCol_TabSelectedOverline] = Vec(Accent);
            c[ImGuiCol_TableHeaderBg] = Vec(Input);
            c[ImGuiCol_TableBorderStrong] = Vec(Border);
            c[ImGuiCol_TableBorderLight] = Blend(Surface, Border, 0.6F);
            c[ImGuiCol_TableRowBg] = {0, 0, 0, 0};
            c[ImGuiCol_TableRowBgAlt] = {1, 1, 1, 0.025F};
            c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
            c[ImGuiCol_ScrollbarGrab] = Vec(Border);
            c[ImGuiCol_ScrollbarGrabHovered] = Vec(Muted);
            c[ImGuiCol_ScrollbarGrabActive] = Vec(Accent);
            c[ImGuiCol_PlotLines] = Vec(Accent);
            c[ImGuiCol_PlotLinesHovered] = Vec(Warn);
            c[ImGuiCol_PlotHistogram] = Vec(AccentMuted);
            c[ImGuiCol_PlotHistogramHovered] = Vec(Accent);
            c[ImGuiCol_TextSelectedBg] = Blend(Input, Accent, 0.45F);
            c[ImGuiCol_NavCursor] = Vec(Accent);
            c[ImGuiCol_ResizeGrip] = {0, 0, 0, 0};
            c[ImGuiCol_ModalWindowDimBg] = Vec(Scrim);
        }

        void Metrics(float scale, Family family) {
            auto& s = ImGui::GetStyle();
            s.WindowPadding = {16, 16};
            s.FramePadding = {10, 6};
            s.ItemSpacing = {10, 8};
            s.ItemInnerSpacing = {6, 5};
            s.CellPadding = {8, 6};
            s.WindowRounding = 10;
            s.ChildRounding = 6;
            s.FrameRounding = 5;
            s.PopupRounding = 6;
            s.GrabRounding = 4;
            s.TabRounding = 4;
            s.ScrollbarRounding = 6;
            s.ScrollbarSize = 12;
            s.GrabMinSize = 10;
            s.WindowBorderSize = 1;
            s.ChildBorderSize = 1;
            s.PopupBorderSize = 1;
            s.FrameBorderSize = 1;
            s.TabBarOverlineSize = 0;
            s.SeparatorTextBorderSize = 1;
            s.SeparatorTextPadding = {0, 4};
            s.SelectableTextAlign = {0, 0.5F};
            s.ButtonTextAlign = {0.5F, 0.5F};
            s.WindowTitleAlign = {0, 0.5F};
            s.IndentSpacing = 18;
            if (family != Family::Modern) {
                // A hair of rounding identifies our style when another kit rebuilds it (see Apply).
                s.WindowRounding = 0.01F;
                s.ChildRounding = s.FrameRounding = s.PopupRounding = s.GrabRounding = s.TabRounding = s.ScrollbarRounding = 0;
            }
            switch (family) {
            case Family::Fallout4:
                s.FramePadding = {8, 4};
                s.ItemSpacing = {12, 7};
                s.ScrollbarSize = 8;
                break;
            case Family::Fallout76:
                s.FramePadding = {12, 7};
                s.ItemSpacing = {12, 9};
                s.CellPadding = {10, 7};
                s.WindowBorderSize = 2;
                break;
            case Family::Skyrim:
                s.WindowPadding = {20, 18};
                s.FramePadding = {14, 5};
                s.ItemSpacing = {14, 10};
                s.FrameBorderSize = 0;
                s.ScrollbarSize = 5;
                s.GrabMinSize = 5;
                break;
            case Family::Starfield:
                s.WindowPadding = {20, 20};
                s.FramePadding = {12, 6};
                s.ItemSpacing = {12, 10};
                s.CellPadding = {12, 8};
                s.ScrollbarSize = 6;
                s.TabBarOverlineSize = 2;
                break;
            case Family::Fallout3:
                s.WindowPadding = {14, 14};
                s.FramePadding = {9, 5};
                s.ItemSpacing = {9, 7};
                s.CellPadding = {8, 5};
                s.WindowBorderSize = s.ChildBorderSize = s.PopupBorderSize = s.FrameBorderSize = 2;
                s.ScrollbarSize = 10;
                break;
            case Family::NewVegas:
                s.WindowPadding = {18, 14};
                s.FramePadding = {10, 4};
                s.ItemSpacing = {14, 8};
                s.CellPadding = {10, 5};
                s.WindowBorderSize = s.PopupBorderSize = 2;
                s.ScrollbarSize = 9;
                break;
            default: break;
            }
            s.ScaleAllSizes(scale);
            s.FontSizeBase = BodySize * scale;
        }

        void UpdatePalette(Family family) {
            const auto hud = common::HudColor();
            if (paletteSet && paletteFamily == family && paletteHud == hud) return;
            Palette(family, hud);
            paletteSet = true;
            paletteFamily = family;
            paletteHud = hud;
            ++accentRevision;
        }
    }

    ImVec4 Vec(ImU32 color) { return ImGui::ColorConvertU32ToFloat4(color); }

    void LoadFonts(const std::filesystem::path& dir) {
        auto& current = Current();
        auto& fonts = current.fonts;
        fonts.body = Load(dir / "Roboto-Regular.ttf", dir);
        fonts.bold = Load(dir / "Roboto-Bold.ttf", dir);
        fonts.mono = Load(dir / "Inconsolata-Medium.ttf", dir);
        const auto& kit = common::CurrentFonts();
        if (!fonts.body) fonts.body = kit.regular ? kit.regular : ImGui::GetIO().Fonts->AddFontDefault();
        if (!fonts.bold) fonts.bold = kit.bold ? kit.bold : fonts.body;
        if (!fonts.mono) fonts.mono = fonts.body;
        auto& condensed = current.condensed;
        condensed.body = Load(dir / "RobotoCondensed-Regular.ttf", dir);
        condensed.bold = Load(dir / "RobotoCondensed-Bold.ttf", dir);
        if (!condensed.body) condensed.body = fonts.body;
        if (!condensed.bold) condensed.bold = fonts.bold;
        auto* terminal = Load(dir / "ShareTechMono-Regular.ttf", dir);
        if (!terminal) terminal = fonts.mono;
        condensed.mono = terminal;
        current.terminal = {terminal, terminal, terminal};
        auto& skyrim = current.skyrim;
        skyrim.body = Load(dir / "Jost-Light.ttf", dir);
        skyrim.bold = Load(dir / "Jost-Medium.ttf", dir);
        skyrim.mono = fonts.mono;
        if (!skyrim.bold) skyrim.bold = skyrim.body ? skyrim.body : condensed.bold;
        if (!skyrim.body) skyrim.body = condensed.body;
        auto& starfield = current.starfield;
        starfield.body = Load(dir / "ChakraPetch-Light.ttf", dir);
        starfield.bold = Load(dir / "Barlow-SemiBold.ttf", dir);
        starfield.mono = terminal;
        if (!starfield.body) starfield.body = fonts.body;
        if (!starfield.bold) starfield.bold = fonts.bold;
        // Modern is this context's style from now on: scale changes rebuild it instead of the fo76 default.
        common::SetStyleRebuild([](float scale) { Apply(scale); });
    }

    const Fonts& CurrentFonts() {
        const auto& current = Current();
        switch (CurrentAppearance().family) {
        case Family::Fallout3:
        case Family::NewVegas: return current.terminal.body ? current.terminal : current.fonts;
        case Family::Fallout4:
        case Family::Fallout76: return current.condensed.body ? current.condensed : current.fonts;
        case Family::Skyrim: return current.skyrim.body ? current.skyrim : current.fonts;
        case Family::Starfield: return current.starfield.body ? current.starfield : current.fonts;
        default: return current.fonts;
        }
    }

    float Scale() {
        const auto scale = Current().scale;
        return scale > 0.0F ? scale : 1.0F;
    }

    float Round(float value) { return CurrentAppearance().family == Family::Modern ? Px(value) : 0.0F; }

    ImU32 SelectionFill() {
        const auto family = CurrentAppearance().family;
        return family == Family::Modern || family == Family::Skyrim ? AccentSoft : Accent;
    }

    ImU32 SelectionText() {
        const auto family = CurrentAppearance().family;
        return family == Family::Modern || family == Family::Skyrim ? Text : OnAccent;
    }

    std::array<ImU32, kColors.size()> SharedColors() {
        std::array<ImU32, kColors.size()> colors{};
        for (std::size_t i = 0; i < colors.size(); ++i) colors[i] = *kColors[i];
        return colors;
    }

    void ApplyStyle(float displayScale) {
        const auto& appearance = CurrentAppearance();
        UpdatePalette(appearance.family);
        auto& current = Current();
        auto& style = ImGui::GetStyle();
        const float scale = displayScale * appearance.textScale;
        // The kit rebuilds its own style whenever the display scale changes; our rounding marks ours.
        if (current.displayScale == displayScale && current.scale == scale && current.backgroundOpacity == appearance.backgroundOpacity &&
            style.WindowRounding == current.appliedWindowRounding && current.accentRevision == accentRevision &&
            current.family == appearance.family)
            return;
        style = ImGuiStyle();
        Metrics(scale, appearance.family);
        Colors(appearance.backgroundOpacity);
        current.displayScale = displayScale;
        current.scale = scale;
        current.backgroundOpacity = appearance.backgroundOpacity;
        current.appliedWindowRounding = style.WindowRounding;
        current.accentRevision = accentRevision;
        current.family = appearance.family;
    }

    void SetAccent(float r, float g, float b) {
        const auto accent = Readable(r, g, b);
        if (accent == Accent) return;
        AssignAccent(accent, OnAccent);
        ++accentRevision;
    }
}
