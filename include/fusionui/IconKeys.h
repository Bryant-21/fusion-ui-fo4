#pragma once
// Stable icon keys shared by every FusionUI view and by the converters that export icon packs. A pack
// (fusionui/Icons.h) supplies art for any of these; a key no pack supplies falls back to the Font Awesome
// 7 Free Solid glyph listed here (codepoints read from assets/fonts/Font_Awesome_7_Free-Solid-900.otf).
// Add keys, never rename them: packs on players' machines use these names.
#include <string_view>

namespace fusionui::icons {
    struct KeyGlyph {
        std::string_view key;
        const char* glyph;  // UTF-8
    };

    inline constexpr KeyGlyph kFallbacks[] = {
        // C.A.M.P. build categories and actions
        {"camp.category.structures", "\xEF\x80\x95"},  // house
        {"camp.category.floors", "\xEF\x80\x8A"},      // table-cells
        {"camp.category.walls", "\xEF\x9B\x99"},       // dungeon
        {"camp.category.doors", "\xEF\x94\xAB"},       // door-open
        {"camp.category.roofs", "\xEF\x92\x94"},       // warehouse
        {"camp.category.stairs", "\xEE\x8A\x89"},      // stairs
        {"camp.category.furniture", "\xEF\x92\xB8"},   // couch
        {"camp.category.decor", "\xEF\x80\xBE"},       // image
        {"camp.category.lights", "\xEF\x83\xAB"},      // lightbulb
        {"camp.category.power", "\xE2\x9A\xA1"},       // bolt
        {"camp.category.resources", "\xEF\x81\xAC"},   // leaf
        {"camp.category.defense", "\xEF\x8F\xAD"},     // shield-halved
        {"camp.category.crafting", "\xEF\x9F\x99"},    // screwdriver-wrench
        {"camp.category.containers", "\xEF\x91\xA6"},  // box
        {"camp.category.misc", "\xEF\x85\x81"},        // ellipsis
        {"camp.category.camp", "\xEF\x9A\xBB"},        // campground
        {"camp.category.walldecor", "\xEE\x88\x89"},   // panorama
        {"camp.category.wallpapers", "\xEF\x96\xAA"},  // paint-roller
        {"camp.category.history", "\xEF\x87\x9A"},     // clock-rotate-left
        {"camp.category.quest", "\xEF\x88\x99"},       // diamond
        {"camp.category.dwellers", "\xEF\x94\x80"},    // user-group
        {"camp.category.fo1st", "\xEF\x94\xA1"},       // crown
        {"camp.category.edit", "\xEF\x9B\xA3"},        // hammer
        {"camp.category.lock", "\xEF\x82\x84"},        // key
        {"camp.category.blueprint", "\xEF\x83\x85"},   // copy
        {"camp.category.replace", "\xEF\x80\xA1"},     // arrows-rotate
        {"camp.badge.missing", "\xEF\x82\x85"},        // gears
        {"camp.badge.unavailable", "\xEF\x81\x9E"},    // ban
        {"camp.badge.locked", "\xEF\x80\xA3"},         // lock
        {"camp.badge.premium", "\xEF\x97\x92"},        // atom
        {"camp.badge.fo1st", "\xEF\x94\xA1"},          // crown
        {"camp.badge.variants", "\xEF\x97\xBD"},       // layer-group
        {"camp.card.transferlock", "\xEF\x80\xA3"},    // lock
        {"camp.action.build", "\x2B"},                 // plus
        {"camp.action.scrap", "\xEF\x87\xB8"},         // trash
        {"camp.action.store", "\xEF\x91\xA6"},         // box
        {"camp.action.move", "\xEF\x81\x87"},          // arrows-up-down-left-right
        {"camp.action.lock", "\xEF\x80\xA3"},          // lock
        {"camp.budget", "\xEF\x98\xA4"},               // gauge
        {"mainmenu.play", "\xEF\x81\x8B"},          // play
        {"mainmenu.character", "\xEF\x80\x87"},     // user
        {"reputation.crater", "\xEF\x95\x8C"},                     // skull
        {"reputation.foundation", "\xEF\x9B\xA3"},                 // hammer
        {"reputation.crater.hostile", "\xEF\x95\x96"},             // face-angry
        {"reputation.crater.cautious", "\xEF\x97\x82"},            // face-surprise
        {"reputation.crater.neutral", "\xEF\x84\x9A"},             // face-meh
        {"reputation.crater.cooperative", "\xEF\x84\x98"},         // face-smile
        {"reputation.crater.friendly", "\xEF\x96\x80"},            // face-grin
        {"reputation.crater.neighborly", "\xEF\x96\x99"},          // face-laugh
        {"reputation.crater.ally", "\xEF\x96\x84"},                // face-grin-hearts
        {"reputation.foundation.hostile", "\xEF\x95\x96"},         // face-angry
        {"reputation.foundation.cautious", "\xEF\x97\x82"},        // face-surprise
        {"reputation.foundation.neutral", "\xEF\x84\x9A"},         // face-meh
        {"reputation.foundation.cooperative", "\xEF\x84\x98"},     // face-smile
        {"reputation.foundation.friendly", "\xEF\x96\x80"},        // face-grin
        {"reputation.foundation.neighborly", "\xEF\x96\x99"},      // face-laugh
        {"reputation.foundation.ally", "\xEF\x96\x84"},            // face-grin-hearts
        {"dailyops.logo", "\xEF\x95\x8C"},                         // skull
        {"casino.roulette.wheel", "\xEF\x99\x95"},                 // dharmachakra
        {"casino.roulette.dealer", "\xEF\x94\x88"},                // user-tie
        {"casino.dice.dealer", "\xEF\x94\xA2"},                    // dice
        {"casino.slots3", "\xEF\x94\x9E"},                         // coins
        {"casino.slots5", "\xEF\x94\x9E"},                         // coins
        {"casino.slots.lose", "\xEF\x95\x8C"},                     // skull
        {"photo.tab.lens", "\xEF\x80\xB0"},            // camera
        {"photo.tab.player", "\xEF\x80\x87"},          // user
        {"photo.tab.cinematic", "\xEF\x86\x85"},       // sun
        {"photo.tab.filters", "\xEF\x94\xBF"},         // palette
        {"photo.tab.overlay", "\xEF\x80\xBE"},         // image
        {"loading.survivalguide", "\xEF\x80\xAD"},     // book (FO76 italic "Survival Guide" wordmark)
        {"hud.hit.reflected", "\xEF\x8F\xA5"},         // reply
        {"compass.quest.distance", "\xEF\x8F\x85"},    // location-dot
        {"compass.quest.npc", "\xEF\x80\x87"},         // user
        {"paperdoll.body", "\xEF\x80\x87"},            // user
        {"paperdoll.clothes", "\xEF\x80\x87"},         // user
        {"paperdoll.vaultsuit", "\xEF\x80\x87"},       // user
        {"paperdoll.torso", "\xEF\x80\x87"},           // user
        {"paperdoll.arm.left", "\xEF\x80\x87"},        // user
        {"paperdoll.arm.right", "\xEF\x80\x87"},       // user
        {"paperdoll.leg.left", "\xEF\x80\x87"},        // user
        {"paperdoll.leg.right", "\xEF\x80\x87"},       // user
        {"paperdoll.helmet", "\xEF\x80\x87"},          // user
        {"paperdoll.goggles", "\xEF\x80\x87"},         // user
        {"paperdoll.gasmask", "\xEF\x80\x87"},         // user
        {"loading.vault", "\xEF\x80\x93"},             // gear (FO76's vault cog)
        {"cursor", "\xEF\x89\x85"},                    // mouse-pointer; FO76 cursor fill; drawn over cursor.outline in one frame
        {"cursor.outline", "\xEF\x89\x85"},            // mouse-pointer
        {"loading.vault.number", "4"},                 // drawn still over the spinning cog
        {"ui.chevron.up", "\xEF\x84\x82"},             // angles-up
        {"ui.chevron.down", "\xEF\x84\x83"},           // angles-down
        {"camp.card.tagged", "\xEF\x80\x82"},          // magnifying-glass
        {"camp.resource.water", "\xEF\x81\x83"},       // droplet
        {"camp.resource.power", "\xEF\x83\xA7"},       // bolt
        {"camp.resource.powergenerated", "\xEF\x83\xA7"},  // bolt
        {"camp.resource.safety", "\xEF\x8F\xAD"},      // shield-halved
        {"camp.resource.food", "\xEF\x83\xB5"},        // utensils
        {"camp.resource.copper", "\xEF\x86\xB3"},      // cubes
        {"camp.resource.fertilizer", "\xEF\x81\xAC"},  // leaf
        {"camp.resource.scavenge", "\xEF\x80\x93"},    // gear
        {"camp.resource.steel", "\xEF\x86\xB3"},       // cubes
        {"camp.resource.aluminum", "\xEF\x91\xA6"},    // box
        {"camp.resource.acid", "\xEF\x83\x83"},        // flask
        {"hud.levelup", "\xEF\x93\xBB"},               // user-astronaut (until the Vault Boy perk-card art exists)
        {"reward.vaultboy", "\xEF\x93\xBB"},           // user-astronaut (until the quest Vault Boy art exists)
        {"hud.hit.protected", "\xEF\x8F\xAD"},         // shield-halved
        {"hud.voice.mic", "\xEF\x84\xB0"},             // microphone
        {"hud.voice.speaking", "\xEF\x80\xA8"},        // volume-high
        {"hud.voice.muted", "\xEF\x84\xB1"},           // microphone-slash
        {"hud.voice.channel", "\xEF\x83\x80"},         // users
        // Favorites wheel item types
        {"item.type.pistol", "\xEE\x86\x9B"},          // gun
        {"item.type.rifle", "\xEE\x95\x8E"},           // person-rifle
        {"item.type.melee", "\xEF\x9B\xA3"},           // hammer
        {"item.type.unarmed", "\xE2\x9C\x8A"},         // hand-fist
        {"item.type.heavy", "\xEF\x84\xB5"},           // rocket
        {"item.type.thrown", "\xEF\x87\xA2"},          // bomb
        {"item.type.apparel", "\xEF\x95\x93"},         // shirt
        {"item.type.helmet", "\xEF\xA0\x87"},          // helmet-safety
        {"item.type.food", "\xEF\x83\xB5"},            // utensils
        {"item.type.drink", "\xEE\x93\x85"},           // bottle-water
        {"item.type.chem", "\xEF\x92\x84"},            // pills
        {"item.type.stimpak", "\xEF\x92\x8E"},         // syringe
        {"item.type.book", "\xEF\x80\xAD"},            // book
        {"item.type.ammo", "\xEF\x91\xA6"},            // box
        {"item.type.misc", "\xEF\x85\x81"},            // ellipsis
        {"item.type.holotape", "\xEF\x93\x9B"},        // tape
        {"item.unique", "\xEF\x8E\xA5"},               // gem
        // Currencies
        {"currency.caps.hud", "\xEF\x94\x9E"},         // coins
        {"currency.caps","\xEF\x94\x9E"},             // coins
        {"currency.scrip", "\xEF\x95\x83"},            // receipt
        {"currency.bullion", "\xEF\xA0\x9D"},          // sack-dollar
        {"currency.stamps", "\xEF\x96\xBF"},           // stamp
        {"currency.treasurynote", "\xEF\x83\x96"},     // money-bill
        {"currency.perkcoin", "\xEF\x94\x9E"},         // coins
        {"currency.perkpack", "\xEF\x91\xA6"},         // box
        {"currency.badge", "\xEF\x96\xA2"},            // medal
        {"currency.atoms", "\xE2\x9A\x9B"},            // atom
        {"shop.equipped", ""},             // circle-check
        {"currency.supplies", "\xEF\x91\xA6"},         // box
        {"currency.tadpole", "\xEF\x94\xAE"},          // frog
        // HUD and status
        {"hud.revive", "\xEF\x91\xB9"},                // kit-medical
        {"hud.hunger", "\xEF\x83\xB5"},                // utensils
        {"hud.thirst", "\xEF\x81\x83"},                // droplet
        {"hud.disease", "\xEE\x81\xB4"},               // virus
        {"hud.companion", "\xEF\x83\x80"},             // users
        {"hud.team.link", "\xEF\x83\x81"},             // link
        {"hud.quest", "\xEF\x81\xAA"},                 // circle-exclamation
        {"hud.loot", "\xEF\x8A\x90"},                  // bag-shopping
        {"hud.xp", "\xE2\xAD\x90"},                    // star
        {"hud.quest.main", "\xE2\x96\xA0"},            // square
        {"hud.quest.shared", "\xE2\xAD\x90"},          // star
        {"hud.team.leader", "\xE2\xAD\x90"},           // star
        {"hud.team.dead", "\xEF\x95\x8C"},             // skull
        {"hud.team.arrow", "\xEF\x83\x97"},            // caret-down
        {"hud.rank", "\xE2\x97\x8F"},                  // circle
        {"hud.time.sun", "\xEF\x86\x85"},              // sun
        {"hud.time.moon", "\xEF\x86\x86"},             // moon
        // Challenges
        {"challenge.daily", "\xEF\x84\xB3"},           // calendar
        {"challenge.weekly", "\xEF\x81\xB3"},          // calendar-days
        {"challenge.world", "\xEF\x82\xAC"},           // globe
        {"challenge.character", "\xEF\x80\x87"},       // user
        {"challenge.complete", "\xE2\x9C\x93"},        // check
        // Equipment bonuses, legendary, item rows
        {"bonus.damage", "\xEE\x93\x9C"},              // burst
        {"bonus.armor", "\xEF\x8F\xAD"},               // shield-halved
        {"bonus.weight", "\xEF\x97\x8D"},              // weight-hanging
        {"bonus.condition", "\xEF\x9F\x99"},           // screwdriver-wrench
        {"legendary.star", "\xE2\xAD\x90"},            // star
        {"legendary.star.empty", "\xE2\xAD\x90"},      // star
        {"item.locked", "\xEF\x80\xA3"},               // lock
        {"item.favorite", "\xE2\x99\xA5"},             // heart
        {"item.tagged", "\xEF\x80\x82"},               // magnifying-glass
        {"item.setbonus", "\xEF\x80\xAE"},             // bookmark
        {"item.setbonus.active", "\xEF\x80\xAE"},      // bookmark
        {"item.quest", "\xE2\x99\xA6"},                // diamond
        {"item.equipped", "\xE2\x96\xA0"},             // square
        {"item.weight", "\xEF\x97\x8D"},               // weight-hanging
        {"item.junk", "\xEF\x80\x93"},                 // gear
        {"stat.ammo", "\xEE\x86\x9B"},                 // gun
        {"stat.damage", "\xEF\x81\x9B"},               // crosshairs
        // Damage types
        {"damage.ballistic", "\xEF\x81\x9B"},          // crosshairs
        {"damage.energy", "\xE2\x9A\xA1"},             // bolt
        {"damage.fire", "\xEF\x81\xAD"},               // fire
        {"damage.poison", "\xE2\x98\xA0"},             // skull-crossbones
        {"damage.cold", "\xE2\x9D\x84"},               // snowflake
        {"damage.radiation", "\xEF\x9E\xB9"},          // radiation
        {"damage.explosive", "\xEF\x87\xA2"},          // bomb
        {"damage.bleed", "\xEF\x81\x83"},              // droplet
        // Resistances (FO76 draws them apart from damage only for ballistic: shield vs crosshair)
        {"resist.ballistic", "\xEF\x8F\xAD"},          // shield-halved
        {"resist.energy", "\xE2\x9A\xA1"},             // bolt
        {"resist.fire", "\xEF\x81\xAD"},               // fire
        {"resist.cold", "\xE2\x9D\x84"},               // snowflake
        {"resist.poison", "\xE2\x98\xA0"},             // skull-crossbones
        {"resist.radiation", "\xEF\x9E\xB9"},          // radiation
        // HUD meters and combat
        {"hud.rads", "\xEF\x9E\xB9"},                  // radiation
        {"hud.ap", "\xE2\x9A\xA1"},                    // bolt
        {"hud.crit.star", "\xE2\xAD\x90"},             // star
        {"hud.explosive.grenade", "\xEF\x87\xA2"},     // bomb
        {"hud.explosive.mine", "\xEF\x86\x92"},        // circle-dot
        {"hud.explosive.car", "\xEF\x86\xB9"},         // car
        {"hud.enemy.legendary", "\xEF\x94\xA1"},       // crown
        {"hud.enemy.skull", "\xEF\x95\x8C"},           // skull
        {"hud.enemy.skull.elite", "\xEF\x95\x8C"},     // skull
        {"hud.enemy.target", "\xEF\x81\x9B"},          // crosshairs
        {"hud.enemy.target.elite", "\xEF\x81\x9B"},    // crosshairs
        {"hud.enemy.target.boss", "\xEF\x81\x9B"},     // crosshairs
        {"hud.bounty", "\xE2\x98\xA0"},                // skull-crossbones
        {"hud.condition", "\xEF\x82\xAD"},             // wrench
        {"hud.overheat", "\xEF\x81\xAD"},              // fire
        {"hud.fusioncore", "\xEF\x97\x9F"},            // car-battery
        {"hud.feral", "\xEF\x95\x96"},                 // face-angry
        // Active-effect cards
        {"effect.mutation", "\xEF\x91\xB1"},           // dna
        {"effect.disease", "\xEE\x81\xB4"},            // virus
        {"effect.radiation", "\xEF\x9E\xB9"},          // radiation
        {"effect.hunger", "\xEF\x83\xB5"},             // utensils
        {"effect.thirst", "\xEF\x81\x83"},             // droplet
        {"effect.chems", "\xEF\x92\x84"},              // pills
        {"effect.alcohol", "\xEF\x95\xBB"},            // martini-glass
        {"effect.stealthboy", "\xEF\x81\xB0"},         // eye-slash
        {"effect.encumbered", "\xEF\x97\x8D"},         // weight-hanging
        {"effect.trackingdart", "\xEF\x81\x9B"},       // crosshairs
        {"effect.healingfromitem", "\xEF\x91\xB9"},    // kit-medical
        {"effect.candybucket", "\xEF\x9E\x86"},        // candy-cane
        {"effect.feral", "\xEF\x95\x8C"},              // skull
        {"effect.drinksoda", "\xEE\x93\x85"},          // bottle-water
        {"effect.drinktea", "\xE2\x98\x95"},           // mug-hot
        {"effect.drinkgeneric", "\xEE\x93\xB4"},       // glass-water
        {"effect.lifesaver", "\xEF\x87\x8D"},          // life-ring
        {"effect.emt", "\xEF\x83\xB9"},                // truck-medical
        {"effect.pareboot", "\xEF\x8B\xB1"},           // rotate
        {"effect.scoutscourage", "\xEF\x96\xA2"},      // medal
        {"effect.perk", "\xEF\x8A\xBB"},               // address-card
        {"effect.generic", "\xEF\x81\x9A"},            // circle-info
        {"effect.onslaught", "\xEF\x84\x82"},          // angles-up
        {"effect.bleed", "\xEF\x81\x83"},              // droplet
        {"effect.adrenaline", "\xEF\x95\x8C"},         // skull
        {"effect.sleep", "\xEF\x88\xB6"},              // bed
        {"effect.food", "\xEE\x93\x86"},               // bowl-food
        {"effect.stimpak", "\xEF\x92\x8E"},            // syringe
        {"effect.rage", "\xE2\x9C\x8A"},               // hand-fist
        {"effect.chargeup", "\xEF\x9B\xA3"},           // hammer
        {"effect.buff.positive", "\xEF\x81\x95"},      // circle-plus
        {"effect.buff.negative", "\xEF\x81\x96"},      // circle-minus
        // Compass markers
        {"compass.quest", "\xE2\x99\xA6"},             // diamond
        {"compass.quest.main", "\xE2\x96\xA0"},        // square
        {"compass.enemy", "\xE2\x97\x8F"},             // circle
        {"compass.location", "\xEF\x8F\x85"},          // location-dot
        {"compass.location.undiscovered", "\xEF\x89\xB6"},  // map-pin
        {"compass.camp", "\xE2\x9B\xBA"},              // campground
        {"compass.powerarmor", "\xEF\x94\x85"},        // user-shield
        {"compass.player", "\xEF\x84\xA4"},            // location-arrow
        {"compass.quest.door", "\xEF\x94\xAA"},        // door-closed
        {"compass.quest.shared", "\xE2\xAD\x90"},      // star
        {"compass.quest.area", "\xE2\x97\x8F"},        // circle
        {"compass.enemy.targeted", "\xE2\x99\xA6"},    // diamond
        {"compass.teammate", "\xEF\x80\x87"},          // user
        {"compass.teammate.down", "\xEF\x9C\xA8"},     // user-injured
        {"compass.teammate.dead", "\xEF\x95\x8C"},     // skull
        {"compass.teammate.leader", "\xE2\xAD\x90"},   // star
        {"compass.pet", "\xEF\x86\xB0"},               // paw
        {"compass.player.hostile", "\xE2\x96\xA0"},    // square
        {"compass.player.friendly", "\xE2\x96\xA0"},   // square
        {"compass.player.neutral", "\xE2\x96\xA0"},    // square
        {"compass.nukezone", "\xEF\x9E\xB9"},          // radiation
        {"compass.workshop", "\xEF\x9F\x99"},          // screwdriver-wrench
        {"compass.vendor.legendary", "\xEF\x95\x8E"},  // store
        {"compass.briefcase", "\xEF\x82\xB1"},         // briefcase
        {"compass.terminal", "\xEE\x93\xA5"},          // computer
        // Feature HUD overlays (fusionhud/HudOverlays.h); the faces, Vault Boys, portrait and casino art are full colour
        {"reputation.levelup.crater", "\xEF\x93\xBB"},             // user-astronaut (until the Vault Boy art exists)
        {"reputation.levelup.foundation", "\xEF\x93\xBB"},         // user-astronaut (until the Vault Boy art exists)
        {"vo.dodge", "\xEF\x80\x87"},                              // user
        {"dailyops.wordmark", "\xEF\x8F\xAD"},                     // shield-halved
        {"dailyops.rank.initiate", "\xEF\x8F\xAD"},                // shield-halved
        {"dailyops.rank.paladin", "\xEF\x84\xB2"},                 // shield
        {"dailyops.rank.elder", "\xEF\x94\xA1"},                   // crown
        {"casino.derby.win", "\xEF\x82\x91"},                      // trophy
        {"casino.derby.lose", "\xEF\x96\xA2"},                     // medal
        // Missing aiming art must stay empty rather than displaying a placeholder reticle.
        {"fo76.crosshair.standard", ""},
        {"fo76.crosshair.dot", ""},
        {"fo76.crosshair.activate", ""},
        {"fo76.crosshair.command", ""},
        {"fo76.crosshair.charge", ""},
        {"fo76.crosshair.scope_01", ""},
        {"fo76.crosshair.scope_02", ""},
        {"fo76.crosshair.scope_03", ""},
        {"fo76.crosshair.scope_04", ""},
        {"fo76.crosshair.scope_05", ""},
        {"fo76.crosshair.scope_06", ""},
        {"fo76.crosshair.scope_07", ""},
        {"fo76.crosshair.scope_08", ""},
        {"fo76.crosshair.scope_09", ""},
        {"fo76.crosshair.scope_10", ""},
        {"fo76.crosshair.scope_11", ""},
        {"fo76.crosshair.scope_13", ""},
        {"fo76.crosshair.scope_14", ""},
        {"fo76.crosshair.scope_15", ""},
        {"fo76.crosshair.scope_16", ""},
        {"fo76.crosshair.scope_17", ""},
        {"fo76.crosshair.camera_01", ""},
        {"fo76.crosshair.camera_19", ""},
        {"fo76.crosshair.camera_20", ""},
        {"fo76.crosshair.camera_21", ""},
        {"fo76.crosshair.camera_22", ""},
    };

    inline constexpr const char* kPlaceholderGlyph = "\xEF\x81\x99";  // circle-question

    constexpr const char* FallbackGlyph(std::string_view key) {
        for (const auto& entry : kFallbacks)
            if (entry.key == key) return entry.glyph;
        return nullptr;
    }
}
