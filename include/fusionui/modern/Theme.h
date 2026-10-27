#pragma once

#include <imgui.h>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

// Shared widgets with a modern tool look or a source-game-inspired HUD look: see Family.
namespace fusionui::modern::theme {
    // Apply() sets every colour below for the current family and HUD colour. Render thread only.
    inline ImU32 Background = IM_COL32(0x10, 0x11, 0x14, 0xFF);
    inline ImU32 Sidebar = IM_COL32(0x17, 0x18, 0x1C, 0xFF);
    inline ImU32 Surface = IM_COL32(0x22, 0x24, 0x29, 0xFF);
    inline ImU32 Input = IM_COL32(0x2B, 0x2E, 0x34, 0xFF);
    inline ImU32 Border = IM_COL32(0x3A, 0x3E, 0x46, 0xFF);
    inline ImU32 Text = IM_COL32(0xEC, 0xEE, 0xF2, 0xFF);
    inline ImU32 Muted = IM_COL32(0xA6, 0xAE, 0xBA, 0xFF);
    inline ImU32 Faint = IM_COL32(0x6E, 0x76, 0x82, 0xFF);
    inline ImU32 Accent = IM_COL32(0xC9, 0xA2, 0x27, 0xFF);
    inline ImU32 AccentMuted = IM_COL32(0x9B, 0x7A, 0x1B, 0xFF);
    inline ImU32 AccentHover = IM_COL32(0xDD, 0xB8, 0x45, 0xFF);
    inline ImU32 AccentSoft = IM_COL32(0xC9, 0xA2, 0x27, 0x2E);
    inline ImU32 OnAccent = IM_COL32(0x14, 0x12, 0x0A, 0xFF);
    inline ImU32 Ok = IM_COL32(0x79, 0xCD, 0x92, 0xFF);
    inline ImU32 Warn = IM_COL32(0xED, 0xC5, 0x75, 0xFF);
    inline ImU32 Error = IM_COL32(0xF2, 0x8A, 0x91, 0xFF);
    inline ImU32 ErrorFill = IM_COL32(0xF2, 0x8A, 0x91, 0x26);
    inline ImU32 WarnFill = IM_COL32(0xED, 0xC5, 0x75, 0x22);
    inline ImU32 OkFill = IM_COL32(0x79, 0xCD, 0x92, 0x22);
    inline ImU32 Scrim = IM_COL32(0x00, 0x00, 0x00, 0x73);

    // Every plugin DLL has its own copy of the colours above; FusionUI.dll computes them and SyncColors() copies them in.
    inline constexpr std::array<ImU32*, 20> kColors{&Background, &Sidebar, &Surface, &Input, &Border, &Text, &Muted,
        &Faint, &Accent, &AccentMuted, &AccentHover, &AccentSoft, &OnAccent, &Ok, &Warn, &Error, &ErrorFill, &WarnFill,
        &OkFill, &Scrim};
    std::array<ImU32, kColors.size()> SharedColors();
    inline void SyncColors() {
        const auto colors = SharedColors();
        for (std::size_t i = 0; i < colors.size(); ++i) *kColors[i] = colors[i];
    }

    // Unscaled CSS-like pixel sizes at 1080p.
    inline constexpr float BodySize = 16.0F, SmallSize = 14.0F, HeadingSize = 21.0F, TitleSize = 24.0F, MonoSize = 15.0F;

    struct Fonts {
        ImFont* body{};
        ImFont* bold{};
        ImFont* mono{};
    };

    // Text fonts merge Font Awesome and symbols. Missing game fonts fall back to the modern/condensed
    // fonts, so consumers shipping an older asset set remain usable.
    void LoadFonts(const std::filesystem::path& fontDir);
    const Fonts& CurrentFonts();

    // Rebuilds the style for this UI scale (common::UiScale()). Cheap to call every
    // frame: it only rebuilds when the scale changes or the kit replaced the style.
    void ApplyStyle(float scale);
    inline void Apply(float scale) {
        ApplyStyle(scale);
        SyncColors();
    }
    // 0..1 RGB. Dark picks are brightened so accent text stays readable on the dark surfaces.
    void SetAccent(float r, float g, float b);
    float Scale();
    inline float Px(float value) { return value * Scale(); }
    // Player appearance choices for a modern FusionUI app: kept in Data/F4SE/Plugins/FusionUI.toml, in the
    // app's own [Modern.<client>] section once SetAppearanceClient named it (else the shared [Modern]),
    // and re-read when another app saves it. Apply() folds textScale into Scale().
    enum class Family { Modern, Fallout4, Fallout76, Skyrim, Starfield, Fallout3, NewVegas };
    struct FamilyInfo {
        Family family;
        std::string_view name, label, description, pack;
    };
    inline constexpr std::array Families{
        FamilyInfo{Family::Modern, "modern", "Modern", "Rounded dark tools with the player's HUD accent.", ""},
        FamilyInfo{Family::Fallout4, "fo4", "Fallout 4", "HUD-colour ink, condensed type and inverse selection bars.", "fo4"},
        FamilyInfo{Family::Fallout76, "fo76", "Fallout 76", "HUD-colour ink, selection plates and angular panels.", "fo76"},
        FamilyInfo{Family::Skyrim, "skyrim", "Skyrim", "Fine rules, dark glass and restrained diamond ornaments.", "skyrim"},
        FamilyInfo{Family::Starfield, "starfield", "Starfield", "Instrument type, navy panels and fine technical rules.", "starfield"},
        FamilyInfo{Family::NewVegas, "fnv", "Fallout 3 / New Vegas", "Native Pip-Boy 3000 artwork, phosphor text and bracket frames.", "fnv"},
    };
    constexpr const FamilyInfo& Info(Family family) {
        if (family == Family::Fallout3) family = Family::NewVegas;
        for (const auto& info : Families)
            if (info.family == family) return info;
        return Families.front();
    }
    constexpr std::string_view FamilyLabel(Family family) { return Info(family).label; }
    constexpr std::string_view FamilyDescription(Family family) { return Info(family).description; }
    constexpr std::string_view FamilyPack(Family family) { return Info(family).pack; }
    struct Appearance {
        float backgroundOpacity = 0.90F;  // window background alpha
        float textScale = 1.0F;           // whole interface
        float tableTextSize = 14.0F;      // w::Table rows, unscaled; headers and mono cells keep their ratio
        Family family = Family::Modern;
        std::string cursor = "theme";
        std::string iconSet = "theme";
        std::string crosshair = "theme";
    };
    inline constexpr float MinBackgroundOpacity = 0.3F, MinTextScale = 0.7F, MaxTextScale = 1.6F;
    inline constexpr float MinTableTextSize = 10.0F, MaxTableTextSize = 22.0F;
    // Names this context's section and its defaults (e.g. a see-through window); call before drawing.
    void SetAppearanceClient(std::string_view client, const Appearance& defaults = {});
    const Appearance& DefaultAppearance();
    const Appearance& CurrentAppearance();
    // save = false previews the change in this module only (e.g. while a slider is dragged).
    void SetAppearance(const Appearance& appearance, bool save = true);
    // Switches the look live and saves it for this client, or (allWindows) as the shared choice that
    // every B21 window without its own follows, clearing the other clients' own choices.
    void SetFamily(Family family, bool allWindows = false);
    // Corner radius for this family: `value` scaled in Modern, square otherwise.
    float Round(float value);
    ImU32 SelectionFill();
    ImU32 SelectionText();
    inline float TableTextSize() { return CurrentAppearance().tableTextSize; }
    // Whether a window (by the app's name) was last left full screen; [Windows] in the same FusionUI.toml.
    bool SavedFullscreen(std::string_view window);
    void SaveFullscreen(std::string_view window, bool fullscreen);
    // Other per-window choices in the same [Windows] section: "<window><name>" keys.
    bool SavedWindowFlag(std::string_view window, std::string_view name, bool fallback = false);
    void SaveWindowFlag(std::string_view window, std::string_view name, bool value);
    std::string SavedWindowText(std::string_view window, std::string_view name);
    void SaveWindowText(std::string_view window, std::string_view name, std::string_view value);
    // A background colour at the player's background opacity, for panels an app fills itself.
    inline ImU32 Translucent(ImU32 color) {
        const auto alpha = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * CurrentAppearance().backgroundOpacity;
        return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha + 0.5F) << IM_COL32_A_SHIFT);
    }

    ImVec4 Vec(ImU32 color);
}

// Font Awesome 6 Free Solid codepoints (UTF-8).
namespace fusionui::modern::icon {
    inline constexpr const char* Search = "\xEF\x80\x82";       // f002
    inline constexpr const char* Gear = "\xEF\x80\x93";         // f013
    inline constexpr const char* Keyboard = "\xEF\x84\x9C";
    inline constexpr const char* Info = "\xEF\x81\x9A";
    inline constexpr const char* Close = "\xEF\x80\x8D";        // f00d
    inline constexpr const char* Star = "\xEF\x80\x85";         // f005
    inline constexpr const char* Copy = "\xEF\x83\x85";         // f0c5
    inline constexpr const char* Refresh = "\xEF\x8B\xB1";      // f2f1
    inline constexpr const char* Left = "\xEF\x81\x93";         // f053
    inline constexpr const char* Right = "\xEF\x81\x94";        // f054
    inline constexpr const char* Down = "\xEF\x81\xB8";
    inline constexpr const char* Travel = "\xEF\x84\xA4";       // f124
    inline constexpr const char* Plus = "\xEF\x81\xA7";         // f067
    inline constexpr const char* Trash = "\xEF\x87\xB8";        // f1f8
    inline constexpr const char* Play = "\xEF\x81\x8B";         // f04b
    inline constexpr const char* Check = "\xEF\x80\x8C";        // f00c
    inline constexpr const char* Alert = "\xEF\x81\xB1";        // f071
    inline constexpr const char* Bookmark = "\xEF\x80\xAE";     // f02e
    inline constexpr const char* Quest = "\xEF\x9C\x8E";        // f70e scroll
    inline constexpr const char* Map = "\xEF\x89\xB9";          // f279
    inline constexpr const char* Weather = "\xEF\x9B\x84";      // f6c4 cloud-sun
    inline constexpr const char* Loading = "\xEF\x89\x94";      // f254 hourglass
    inline constexpr const char* Creature = "\xEF\x86\xB0";     // f1b0 paw
    inline constexpr const char* Weapon = "\xEF\x81\x9B";       // f05b crosshairs
    inline constexpr const char* Armor = "\xEF\x8F\xAD";        // f3ed shield-halved
    inline constexpr const char* Clothing = "\xEF\x95\x93";     // f553 shirt
    inline constexpr const char* Reference = "\xEF\x8F\x85";    // f3c5 location-dot
    inline constexpr const char* Spawn = "\xEF\x92\x9E";        // f49e box-open
    inline constexpr const char* Note = "\xEF\x80\xAD";         // f02d book
    inline constexpr const char* Values = "\xEF\x88\x9E";       // f21e heart-pulse
    inline constexpr const char* Target = "\xEF\x85\x80";       // f140 bullseye
    inline constexpr const char* Pointer = "\xEF\x89\x9A";      // f25a hand-pointer
    inline constexpr const char* Terminal = "\xEF\x84\xA0";     // f120
    inline constexpr const char* Code = "\xEF\x84\xA1";         // f121
    inline constexpr const char* Palette = "\xEF\x94\xBF";      // f53f
    inline constexpr const char* Performance = "\xEF\x88\x81";  // f201 chart-line
    inline constexpr const char* Streaming = "\xEF\xA0\xBE";    // f83e wave-square
    inline constexpr const char* World = "\xEF\x95\xBD";        // f57d earth-americas
    inline constexpr const char* Diagnostics = "\xEF\x83\xB1";  // f0f1 stethoscope
    inline constexpr const char* Plugin = "\xEF\x84\xAE";       // f12e puzzle-piece
    inline constexpr const char* Bars = "\xEF\x83\x89";         // f0c9 bars
    inline constexpr const char* ChevronLeft = "\xEF\x81\x93";  // f053 chevron-left
    inline constexpr const char* Layers = "\xEF\x97\xBD";       // f5fd layer-group
    inline constexpr const char* Expand = "\xEF\x81\xA5";       // f065 expand
    inline constexpr const char* Eye = "\xEF\x81\xAE";          // f06e eye
    inline constexpr const char* EyeSlash = "\xEF\x81\xB0";     // f070 eye-slash
    inline constexpr const char* Compress = "\xEF\x81\xA6";     // f066 compress
    inline constexpr const char* Heart = "\xEF\x80\x84";        // f004 heart
    inline constexpr const char* Dice = "\xEF\x94\xA2";         // f522 dice
    inline constexpr const char* Person = "\xEF\x80\x87";       // f007 user
    inline constexpr const char* People = "\xEF\x83\x80";       // f0c0 users
    inline constexpr const char* Sitemap = "\xEF\x83\xA8";      // f0e8 sitemap
    inline constexpr const char* Hud = "\xEE\x96\xA9";          // e5a9 person-through-window
    inline constexpr const char* Shaders = "\xEF\x86\xB3";      // f1b3 cubes
    inline constexpr const char* Shuffle = "\xEF\x81\xB4";      // f074 shuffle
    inline constexpr const char* Launch = "\xEF\x82\x8E";       // f08e arrow-up-right-from-square
    inline constexpr const char* Ammo = "\xEF\x91\xA8";         // f468 boxes-stacked
    inline constexpr const char* Camera = "\xEF\x80\xB0";       // f030 camera
    inline constexpr const char* Bugs = "\xEF\x86\x88";         // f188 bugs
    inline constexpr const char* PhotoFilm = "\xEF\xA1\xBC";    // f87c photo-film
    inline constexpr const char* Water = "\xEF\x9D\xB3";        // f773 water
    inline constexpr const char* Sun = "\xEF\x86\x85";          // f185 sun
    inline constexpr const char* Lightbulb = "\xEF\x83\xAB";    // f0eb lightbulb
    inline constexpr const char* Cloud = "\xEF\x83\x82";        // f0c2 cloud
    // Font Awesome Brands
    inline constexpr const char* KoFi = "\xEE\xA1\x96";         // e856 ko-fi
    inline constexpr const char* Patreon = "\xEF\x8F\x99";      // f3d9 patreon
    inline constexpr const char* YouTube = "\xEF\x85\xA7";      // f167 youtube
    inline constexpr const char* Xbox = "\xEF\x90\x92";         // f412 xbox
}
