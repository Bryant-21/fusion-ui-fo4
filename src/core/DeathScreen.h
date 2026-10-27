#pragma once
// Fallout 76's death screen (deathrespawn.swf, DeathRespawnMenu), engine-free. Its timeline is two stopped
// frames and no tweens: "Countdown" (dark band, RESPAWN title, the seconds; hint bar at alpha 0) while
// the timer runs, "List" (only the RESPAWN hint) once it reaches 0.
#include <format>
#include <string>

namespace fusionui::core::death {
    enum class Frame { Countdown, List };

    // DeathRespawnMenu.redrawDisplayObject.
    inline Frame FrameFor(int seconds) { return seconds > 0 ? Frame::Countdown : Frame::List; }
    // The seconds field, two digits like the movie's "05".
    inline std::string CountdownText(int seconds) { return std::format("{:02}", seconds < 0 ? 0 : seconds); }
}
