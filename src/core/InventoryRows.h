#pragma once
// Engine-free rules of FO76's inventory rows and item card (PipBoyInvListEntry, ItemCard), shared by the
// fo76 widgets and their tests.
#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace fusionui::core::inventory {
    struct RowFlags {
        bool equipped = false;
        bool favorite = false;
        bool tagged = false;  // tagged for search
        bool quest = false;
        bool setItem = false;
        bool setActive = false;  // set bonus active (shown only while equipped, as FO76 does)
        bool locked = false;     // transfer locked
    };

    // Square left of the name: the lock replaces the equip square.
    inline std::string_view LeadIcon(const RowFlags& f) {
        if (f.locked) return "item.locked";
        return f.equipped ? "item.equipped" : std::string_view{};
    }

    // Icons right of the name in FO76's order: favourite, search tag, quest item, set bonus. FO76 stacks
    // quest and set bonus on one slot; they get a slot each here so both stay readable.
    inline std::vector<std::string_view> TrailingIcons(const RowFlags& f) {
        std::vector<std::string_view> keys;
        if (f.favorite) keys.emplace_back("item.favorite");
        if (f.tagged) keys.emplace_back("item.tagged");
        if (f.quest) keys.emplace_back("item.quest");
        if (f.setItem) keys.emplace_back(f.setActive && f.equipped ? "item.setbonus.active" : "item.setbonus");
        return keys;
    }

    // Row label pieces: the name (truncated FO76's way), legendary stars drawn after it, then " (count)".
    struct RowLabel {
        std::string name;
        int stars = 0;
        std::string suffix;
    };

    inline std::size_t Utf8Length(std::string_view s) {
        return static_cast<std::size_t>(std::ranges::count_if(s, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
    }

    // Byte offset of the first `chars` UTF-8 characters.
    inline std::size_t Utf8Prefix(std::string_view s, std::size_t chars) {
        std::size_t i = 0;
        for (std::size_t seen = 0; i < s.size(); ++i) {
            if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80 && seen++ == chars) break;
        }
        return i;
    }

    inline constexpr std::size_t kMaxRowLength = 42;  // GlobalFunc.MAX_TRUNCATED_TEXT_LENGTH

    // TruncateSingleLineLegendary: name + " " + one glyph per star counts against the limit; an overlong
    // legendary name is cut and ends in "…". Non-legendary names are left whole (FO76 shrinks the field).
    inline RowLabel MakeRowLabel(std::string_view name, int count, int stars, std::size_t maxLength = kMaxRowLength) {
        RowLabel label{std::string(name), std::max(0, stars), {}};
        if (label.stars > 0) {
            const std::size_t starChars = static_cast<std::size_t>(label.stars) + 1;
            const std::size_t length = Utf8Length(name) + starChars;
            if (length > maxLength) {
                const std::size_t keep = maxLength > starChars + 1 ? maxLength - starChars - 1 : 0;
                label.name = std::string(name.substr(0, Utf8Prefix(name, keep))) + "\xE2\x80\xA6";
            }
        }
        if (count != 1) label.suffix = std::format(" ({})", count);
        return label;
    }

    // Numbers as FO76 prints them: integers plain, fractions to at most `decimals` places without
    // trailing zeros.
    inline std::string FormatNumber(double value, int decimals = 1) {
        auto text = std::format("{:.{}f}", value, std::max(0, decimals));
        if (text.find('.') != std::string::npos) {
            while (text.back() == '0') text.pop_back();
            if (text.back() == '.') text.pop_back();
        }
        return text == "-0" ? "0" : text;
    }

    // V/W as Tales' inventory columns print it: value per unit weight, rounded; "--" when either is missing or a
    // positive value weighs nothing, 0 when both are zero.
    inline std::string ValuePerWeight(int value, double weight) {
        if (value < 0 || weight < 0 || (weight == 0 && value > 0)) return "--";
        return weight == 0 ? "0" : FormatNumber(std::round(value / weight), 0);
    }

    // Carry weight footer "323 / 425".
    inline std::string FormatWeight(double carried, double capacity) {
        return FormatNumber(carried) + " / " + FormatNumber(capacity);
    }

    // CND bar: condition 0..2 fills a dim bar (0..1) from the right, then a bright bar over it (1..2).
    struct ConditionFill {
        float dim = 0.0F;
        float bright = 0.0F;
    };
    inline ConditionFill ConditionBar(float condition) {
        const float c = std::clamp(condition, 0.0F, 2.0F);
        return {std::min(c, 1.0F), std::max(c - 1.0F, 0.0F)};
    }

    // Quantity modal (FO4's QuantityMenu, FO76's ModalSetQuantity_mc): opens on the whole stack; arrows
    // step by one, bumpers by a twentieth.
    struct Quantity {
        int maximum = 1;
        int value = 1;
        static Quantity Open(int maximum) {
            const int m = std::max(maximum, 1);
            return {m, m};
        }
        [[nodiscard]] int Step() const { return std::max(maximum / 20, 1); }
        bool Adjust(int delta) {
            const int next = static_cast<int>(std::clamp(static_cast<long long>(value) + delta, 1LL, static_cast<long long>(maximum)));
            if (next == value) return false;
            value = next;
            return true;
        }
        [[nodiscard]] float Fraction() const { return maximum > 1 ? static_cast<float>(value - 1) / static_cast<float>(maximum - 1) : 1.0F; }
    };

    // Pip-Boy sub-tab carousel (TabHeader_mc): the selected tab in the middle, one and two neighbours each
    // side. Returns the slot (-2..2) tab `index` takes, or 3 when it is off the carousel.
    inline int CarouselSlot(int index, int selected, int count) {
        if (count <= 0) return 3;
        int d = ((index - selected) % count + count) % count;
        if (d > count / 2) d -= count;
        return std::abs(d) <= 2 && (count > 2 || d >= 0) ? d : 3;
    }

    // Comparison marks beside a stat (ItemCard_Comparison): "+", "++", "+++" or "-", "--", "---".
    inline std::string CompareMarks(int delta) {
        const int n = std::min(std::abs(delta), 3);
        return std::string(static_cast<std::size_t>(n), delta > 0 ? '+' : '-');
    }
}
