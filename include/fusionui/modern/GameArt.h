#pragma once

#include "fusionui/modern/Theme.h"
#include "fusionui/GameIcons.h"

#include <array>
#include <string_view>

namespace fusionui::modern::art {
    struct Cursor {
        std::string_view pack, key, outline;
        float size;
        float aspect = 1.0F;
    };

    inline constexpr std::array Cursors{
        Cursor{"fo76", "cursor", "cursor.outline", 26.55F},
        Cursor{"fo4", "fo4.cursor", "fo4.cursor.outline", 21.75F},
        Cursor{"skyrim", "skyrim.cursor", "skyrim.cursor.outline", 41.95F},
        Cursor{"starfield", "starfield.cursor", "starfield.cursor.outline", 21.75F},
        // shortcut: texture cursors have no stage measurements; use 32 px until calibrated in game.
        Cursor{"fo3", "fo3.cursor", "", 32.0F},
        Cursor{"fnv", "fnv.cursor", "", 32.0F}
    };

    inline std::string_view Pack(std::string_view choice, theme::Family family) {
        if (choice == "fo3") return "fnv";
        return choice == "theme" ? theme::FamilyPack(family) : choice == "default" ? std::string_view{} : choice;
    }

    inline const Cursor* FindCursor(std::string_view pack) {
        if (pack == "fo3") pack = "fnv";
        for (const auto& cursor : Cursors)
            if (cursor.pack == pack) return &cursor;
        return nullptr;
    }

    struct Icon {
        std::string_view glyph, neutral;
        std::array<std::string_view, 6> keys;  // Cursors' pack order; empty means Font Awesome.
    };

    inline constexpr std::array Icons{
        Icon{icon::Search, "ui.search", {"item.tagged", "fo4.hud.search", "", "", "", ""}},
        Icon{icon::Star, "ui.favorite", {"item.favorite", "fo4.hud.star", "skyrim.quest.diamond", "", "", ""}},
        Icon{icon::Check, "ui.check", {"item.equipped", "fo4.hud.check", "skyrim.quest.diamond_filled", "", "", ""}},
        Icon{icon::Right, "ui.right", {"", "fo4.hud.chevron_right", "", "", "fo3.arrow.right", "fnv.arrow.right"}},
        Icon{icon::Left, "ui.left", {"", "fo4.hud.bracket_left", "", "", "fo3.arrow.left", "fnv.arrow.left"}},
        Icon{icon::Down, "ui.down", {"", "fo4.hud.chevron_down", "skyrim.ui.arrow_down", "", "fo3.arrow.down", "fnv.arrow.down"}},
        Icon{icon::Weapon, "item.weapons", {"item.type.rifle", "fo4.pipboy.weapon_pistol", "", "starfield.weapon.beowulf", "fo3.item.smallguns", "fnv.item.small_gun"}},
        Icon{icon::Armor, "item.armor", {"item.type.apparel", "fo4.pipboy.defense", "", "starfield.map.marker.cityarmorvendor", "fo3.item.bodywear", "fnv.item.clothing"}},
        Icon{icon::Clothing, "item.clothing", {"item.type.apparel", "fo4.pipboy.helmet_cap", "", "", "fo3.item.bodywear", "fnv.item.clothing"}},
        Icon{icon::Note, "item.books", {"item.type.book", "", "", "", "fo3.item.books", "fnv.item.books"}},
        Icon{icon::Heart, "hud.health", {"hud.revive", "fo4.hud.heart", "", "starfield.status.cardio", "fo3.item.stimpack", "fnv.item.stimpack"}},
        Icon{icon::Quest, "map.quest", {"hud.quest", "fo4.hud.quest", "skyrim.compass.quest", "starfield.compass.questdoor", "fo3.arrow.pointer", "fnv.arrow.pointer"}},
        Icon{icon::Map, "map.location", {"compass.location", "", "skyrim.map.house", "starfield.map.structure.colony", "fo3.map.settlement", "fnv.map.settlement"}},
        Icon{icon::Ammo, "item.ammo", {"item.type.ammo", "fo4.pipboy.ammo", "", "", "fo3.item.bigguns", "fnv.item.ammoswap"}},
        Icon{icon::Alert, "hud.warning", {"hud.rads", "fo4.pipboy.warning", "skyrim.quest.failed", "starfield.status.radiation", "fo3.body.rad_icon", "fnv.hud.explosive"}}
    };

    inline std::string_view IconKey(std::string_view pack, std::string_view glyph) {
        if (pack == "fo3") pack = "fnv";
        for (std::size_t i = 0; i < Cursors.size(); ++i)
            if (Cursors[i].pack == pack) {
                for (const auto& entry : Icons)
                    if (entry.glyph == glyph) return entry.keys[i];
                break;
            }
        return fusionui::icons::GameKey(pack, glyph);
    }
}
