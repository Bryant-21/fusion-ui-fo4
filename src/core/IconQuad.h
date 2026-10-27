#pragma once
// Where an icon's square cell lands when it is turned and mirrored: corner positions and their UVs.
#include <imgui.h>

#include <array>
#include <cmath>
#include <utility>

namespace fusionui::core::icons {
    struct Quad {
        std::array<ImVec2, 4> pos;  // top-left, top-right, bottom-right, bottom-left of the unturned cell
        std::array<ImVec2, 4> uv;
    };

    // The cell of side 2 * `half` about `center`, turned `angle` radians clockwise on screen (y down) and,
    // when `mirror`, flipped left to right in its own frame before turning.
    inline Quad IconQuad(ImVec2 center, ImVec2 half, float angle, bool mirror, ImVec2 uv0, ImVec2 uv1) {
        const float c = std::cos(angle), s = std::sin(angle);
        const auto at = [&](float dx, float dy) { return ImVec2{center.x + dx * c - dy * s, center.y + dx * s + dy * c}; };
        Quad quad{{at(-half.x, -half.y), at(half.x, -half.y), at(half.x, half.y), at(-half.x, half.y)},
                  {ImVec2{uv0.x, uv0.y}, {uv1.x, uv0.y}, {uv1.x, uv1.y}, {uv0.x, uv1.y}}};
        if (mirror) {
            std::swap(quad.uv[0], quad.uv[1]);
            std::swap(quad.uv[2], quad.uv[3]);
        }
        return quad;
    }

    inline Quad IconQuad(ImVec2 center, float half, float angle, bool mirror, ImVec2 uv0, ImVec2 uv1) {
        return IconQuad(center, ImVec2{half, half}, angle, mirror, uv0, uv1);
    }
}
