#pragma once
// Navigation of FO76's C.A.M.P. build menu, engine-free: the category list (each main category, plus the
// sub-categories of the selected one), the item tile column and the selected tile's variant grid. The
// menu reports the new state; the owner keeps it and republishes the items when the category changes.
#include <algorithm>
#include <span>
#include <vector>

namespace fusionui::core::camp {
    enum class Focus { Categories, Items, Variants };

    struct NavState {
        Focus focus = Focus::Categories;
        int category = 0;
        int sub = -1;  // sub-category of `category`, -1 for the main row
        int item = 0;
        int firstRow = 0;   // first visible category row (rows as RowsOf lists them)
        int firstItem = 0;  // first visible tile
        int variant = 0;           // in the selected tile's variant grid; kept while the grid loses focus
        int firstVariantRow = 0;   // first visible grid row
    };

    struct NavInput {
        int dy = 0, dx = 0;
        bool accept = false, cancel = false;
        int cycle = 0;  // previous / next main category (Z / C, LB)
    };

    struct NavShape {
        std::vector<int> subCounts;  // per main category
        int items = 0;
        int visibleItems = 9;
        float rowsViewport = 832.0F;  // stage height of the category list
        float mainRow = 59.0F, subRow = 49.0F;
        float mainSelectedRow = 0;
        // Per main category, each sub-category row's height when it differs from subRow (long names wrap).
        std::vector<std::vector<float>> subRows;
        int variants = 0;  // the selected tile's variants; fewer than 2 has no grid
        int variantColumns = 3, visibleVariantRows = 8;
        bool horizontalStrips = false;
        // FO76's sortSelectedToTop (Build): the list starts at the selected main category, its sub-categories
        // below it, so an opened category takes over the panel. Modify lists every row.
        bool selectedToTop = true;
    };

    struct NavStep {
        NavState state;
        bool categoryChanged = false, itemChanged = false, focusChanged = false, variantChanged = false;
        bool activate = false;  // accept on an item or a variant
        bool back = false;      // cancel on the category list
    };

    struct Row {
        int category, sub;
    };

    // Category rows: every main category, an opened one (a sub-category selected) followed by its sub-categories,
    // as FO76's BSNestedList lists them. Step's firstRow decides which are shown.
    inline std::vector<Row> RowsOf(const NavShape& shape, int category, bool opened) {
        std::vector<Row> rows;
        for (int c = 0; c < static_cast<int>(shape.subCounts.size()); ++c) {
            rows.push_back({c, -1});
            if (c == category && opened)
                for (int s = 0; s < shape.subCounts[c]; ++s) rows.push_back({c, s});
        }
        return rows;
    }

    inline int RowIndex(std::span<const Row> rows, int category, int sub) {
        for (int i = 0; i < static_cast<int>(rows.size()); ++i)
            if (rows[i].category == category && rows[i].sub == sub) return i;
        return 0;
    }

    // First row so that `selected` is fully inside `viewport`, moving `first` as little as possible.
    inline int FollowRows(int first, int selected, std::span<const float> heights, float viewport) {
        const int count = static_cast<int>(heights.size());
        if (count == 0) return 0;
        selected = std::clamp(selected, 0, count - 1);
        first = std::clamp(first, 0, selected);
        const auto span = [&](int from, int to) {  // height of rows [from, to]
            float h = 0;
            for (int i = from; i <= to; ++i) h += heights[i];
            return h;
        };
        while (first < selected && span(first, selected) > viewport) ++first;
        // Fill the viewport when rows above fit (after the list shrank).
        while (first > 0 && span(first - 1, count - 1) <= viewport) --first;
        return first;
    }

    inline int FollowItems(int first, int selected, int count, int visible) {
        if (count <= visible) return 0;
        if (selected < first) first = selected;
        if (selected >= first + visible) first = selected - visible + 1;
        return std::clamp(first, 0, count - visible);
    }

    inline float SubRowHeight(const NavShape& shape, int category, int sub) {
        if (category < static_cast<int>(shape.subRows.size()) && sub < static_cast<int>(shape.subRows[category].size()))
            return shape.subRows[category][sub];
        return shape.subRow;
    }

    // Variants: a 3-wide grid; up / down move a row and stop at the ends, left from the first column goes
    // back to the tiles (FO76's edge bounce).
    inline void StepVariants(NavState& s, const NavShape& shape, const NavInput& in, NavStep& out) {
        const int cols = std::max(1, shape.variantColumns);
        const int col = s.variant % cols;
        if (in.cancel || (in.dx < 0 && col == 0)) s.focus = Focus::Items;
        else if (in.dx) s.variant = std::clamp(s.variant + in.dx, s.variant - col, std::min(shape.variants - 1, s.variant - col + cols - 1));
        else if (in.dy && s.variant + in.dy * cols >= 0 && s.variant + in.dy * cols < shape.variants) s.variant += in.dy * cols;
        else if (in.dy > 0 && s.variant / cols < (shape.variants - 1) / cols) s.variant = shape.variants - 1;  // short last row
        else if (in.accept) out.activate = true;
    }

    inline int FollowVariantRows(int first, int variant, int variants, int cols, int visible) {
        const int rows = (std::max(variants, 1) + cols - 1) / cols;
        return FollowItems(first, variant / cols, rows, std::max(1, visible));
    }

    inline NavStep Step(NavState s, const NavShape& shape, const NavInput& input) {
        auto in = input;
        if (shape.horizontalStrips && s.focus != Focus::Variants) { in.dy = input.dx; in.dx = -input.dy; }
        NavStep out;
        const int categories = static_cast<int>(shape.subCounts.size());
        const auto before = s;
        s.category = categories ? std::clamp(s.category, 0, categories - 1) : 0;
        if (categories) s.sub = std::clamp(s.sub, -1, shape.subCounts[s.category] - 1);
        if (in.cycle && categories) {
            s.category = ((s.category + in.cycle) % categories + categories) % categories;
            s.sub = -1;
        } else if (s.focus == Focus::Categories) {
            if (in.dy && categories) {
                const auto rows = RowsOf(shape, s.category, s.sub >= 0);
                const int at = std::clamp(RowIndex(rows, s.category, s.sub) + (in.dy > 0 ? 1 : -1), 0, static_cast<int>(rows.size()) - 1);
                s.category = rows[at].category;
                s.sub = rows[at].sub;
            } else if ((in.accept || in.dx > 0) && s.sub < 0 && categories && shape.subCounts[s.category] > 0) {
                s.sub = 0;  // open the category
            } else if ((in.accept || in.dx > 0) && shape.items > 0) {
                s.focus = Focus::Items;
            } else if ((in.cancel || (input.dx < 0 && !shape.horizontalStrips)) && s.sub >= 0) {
                s.sub = -1;  // collapse the opened category (Left too, as the user asked)
            } else if (in.cancel) {
                out.back = true;
            }
        } else if (s.focus == Focus::Items) {
            if (in.dy && shape.items > 0) s.item = ((s.item + in.dy) % shape.items + shape.items) % shape.items;
            else if (in.dx < 0 || in.cancel) s.focus = Focus::Categories;
            else if (in.dx > 0 && shape.variants > 1) s.focus = Focus::Variants;
            else if (in.accept && shape.items > 0) out.activate = true;
        } else {
            StepVariants(s, shape, in, out);
        }
        out.categoryChanged = s.category != before.category || s.sub != before.sub;
        if (out.categoryChanged) s.item = s.firstItem = 0;
        // A new category's items are unknown until the owner republishes them; keep the item in range otherwise.
        else s.item = shape.items ? std::clamp(s.item, 0, shape.items - 1) : 0;
        // Another tile's variants: its own recipe first, until the owner republishes the grid.
        if (out.categoryChanged || s.item != before.item) s.variant = s.firstVariantRow = 0;
        if (s.focus == Focus::Variants && shape.variants < 2) s.focus = Focus::Items;
        s.variant = std::clamp(s.variant, 0, std::max(0, shape.variants - 1));
        if (s.focus == Focus::Items && shape.items == 0 && !out.categoryChanged) s.focus = Focus::Categories;
        out.itemChanged = s.item != before.item;
        out.variantChanged = s.variant != before.variant && !out.itemChanged && !out.categoryChanged;
        out.focusChanged = s.focus != before.focus;
        const auto rows = RowsOf(shape, s.category, s.sub >= 0);
        std::vector<float> heights;
        for (const auto& row : rows) heights.push_back(row.sub < 0 ? (row.category == s.category && shape.mainSelectedRow > 0 ? shape.mainSelectedRow : shape.mainRow) : SubRowHeight(shape, row.category, row.sub));
        const int selectedRow = RowIndex(rows, s.category, s.sub);
        if (shape.selectedToTop) {
            const int top = RowIndex(rows, s.category, -1);
            s.firstRow = std::max(top, FollowRows(top, selectedRow, heights, shape.rowsViewport));
        } else {
            s.firstRow = FollowRows(s.firstRow, selectedRow, heights, shape.rowsViewport);
        }
        s.firstItem = FollowItems(s.firstItem, s.item, shape.items, shape.visibleItems);
        s.firstVariantRow = FollowVariantRows(s.firstVariantRow, s.variant, shape.variants, std::max(1, shape.variantColumns),
                                              shape.visibleVariantRows);
        out.state = s;
        return out;
    }
}
