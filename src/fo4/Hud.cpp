#include "fusionui/fo4/Hud.h"
#include "fusionui/Icons.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace fusionui::fo4::hud {
    namespace {
        constexpr float kOutline = 0.75F;  // stage units

        ImU32 WithAlpha(ImU32 color, float alpha) {
            const auto a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(alpha, 0.0F, 1.0F);
            return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5F) << IM_COL32_A_SHIFT);
        }

        std::string Upper(std::string_view text) {
            std::string out(text);
            for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }
    }

    void OutlinedText(float x, float y, float size, std::string_view text, ImU32 color, Align align, bool bold) {
        const auto alpha = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F;
        const auto outline = WithAlpha(Black, alpha * 0.85F);
        for (const auto [dx, dy] : {std::pair{-kOutline, 0.0F}, {kOutline, 0.0F}, {0.0F, -kOutline}, {0.0F, kOutline}})
            Text(x + dx, y + dy, size, text, outline, bold, 0.0F, align);
        Text(x, y, size, text, color, bold, 0.0F, align);
    }

    void Meter(float x, float y, float width, float height, float fill, ImU32 color, int segments) {
        auto* list = Canvas();
        if (!list) return;
        const auto alpha = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F;
        list->AddRectFilled(Stage(x, y), Stage(x + width, y + height), WithAlpha(Black, alpha * 0.35F));
        list->AddRect(Stage(x, y), Stage(x + width, y + height), WithAlpha(color, 0.4F), 0.0F, 0, std::max(1.0F, Px(1.0F)));
        fill = std::clamp(fill, 0.0F, 1.0F);
        if (fill <= 0.0F) return;
        const float inset = 1.5F;
        const float innerW = width - inset * 2, innerH = height - inset * 2;
        if (segments <= 1) {
            list->AddRectFilled(Stage(x + inset, y + inset), Stage(x + inset + innerW * fill, y + inset + innerH), color);
            return;
        }
        const float gap = 1.5F;
        const float block = (innerW - gap * static_cast<float>(segments - 1)) / static_cast<float>(segments);
        const float filled = fill * static_cast<float>(segments);
        for (int i = 0; i < segments && static_cast<float>(i) < filled; ++i) {
            const float part = std::min(1.0F, filled - static_cast<float>(i));
            const float x0 = x + inset + static_cast<float>(i) * (block + gap);
            list->AddRectFilled(Stage(x0, y + inset), Stage(x0 + block * part, y + inset + innerH), color);
        }
    }

    float IconLabel(float x, float y, float size, std::string_view iconKey, std::string_view label, ImU32 color) {
        auto* list = Canvas();
        if (!list) return 0.0F;
        const float iconSize = size * 1.1F;
        icons::Draw(list, iconKey, Stage(x + iconSize / 2, y + size * 0.6F), Px(iconSize), color, 1.0F, true);
        const auto text = Upper(label);
        const float textX = x + iconSize + size * 0.4F;
        OutlinedText(textX, y, size, text, color);
        return textX - x + TextWidth(size, text);
    }

    void Toasts(const ToastQueue& queue, float x, float y, float size, Align align) {
        const float rowHeight = size * 1.6F;
        for (const auto& shown : queue.Visible()) {
            const auto color = Fg(shown.alpha);
            const float rowY = y + static_cast<float>(shown.slot) * rowHeight;
            const auto text = Upper(shown.toast->text);
            if (shown.toast->icon.empty()) {
                OutlinedText(x, rowY, size, text, color, align);
                continue;
            }
            const float iconSize = size * 1.1F;
            const float width = iconSize + size * 0.4F + TextWidth(size, text);
            const float left = align == Align::Right ? x - width : align == Align::Center ? x - width / 2 : x;
            IconLabel(left, rowY, size, shown.toast->icon, shown.toast->text, color);
        }
    }
}
