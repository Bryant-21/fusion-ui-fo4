#pragma once
#include "fusionui/screens/CampBuild.h"

namespace fusionui::styles::camp {
    namespace view = screens::camp;
    core::camp::NavShape ShapeOf(const view::Model& model);
    view::Layout LayoutFor(float width, float height);
    float CardBottom(const view::Card& card, bool expanded);
    std::pair<float, float> CardSpan(const view::Layout& layout, bool expanded);
    bool CardExpandable(const view::Card& card);
    float CardScrollLimit(const view::Card& card);
    view::Result Menu(const view::Model& model, const view::Input& input);
}
