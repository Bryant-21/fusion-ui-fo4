#pragma once
// Fallout 76's death screen (deathrespawn.swf, DeathRespawnMenu), drawn natively from a plain view model.
// Its timeline (core/DeathScreen.h): "Countdown" shows the dark bottom band, the RESPAWN title and the
// seconds; "List" shows only the RESPAWN button hint. Draw between fo4::hud::Begin(id, kFo76Stage) and End.
#include "fusionui/fo76/Stage.h"
#include "core/DeathScreen.h"

#include <string>

#include "fusionui/screens/DeathScreen.h"

namespace fusionui::fo76::death {
    using fusionui::screens::death::Model;
    void Draw(const Model& model, float alpha = 1.0F);
}
