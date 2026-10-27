#include "fusionui/Icons.h"
#include "fusionui/Common.h"
#include "fusionui/IconAtlas.h"
#include "fusionui/Paths.h"
#include "fusionui/modern/GameArt.h"
#include "core/IconResolve.h"

#include <memory>
#include <mutex>
#include <vector>

namespace fusionui::icons {
    namespace {
        struct Pack {
            std::filesystem::path file;
            Priority priority = Priority::Normal;
            std::unique_ptr<IconAtlas> atlas;
            bool attempted = false;  // a failed load is not retried until the device changes
            bool original = false;
        };

        // Packs are registered from game threads and drawn on the render thread.
        std::mutex mutex;
        std::vector<Pack> packs;
        bool builtinRegistered = false;

        ImU32 Scale(ImU32 color, float factor) {
            const auto channel = [&](int shift) {
                return static_cast<ImU32>(static_cast<float>((color >> shift) & 0xFF) * factor) << shift;
            };
            return channel(IM_COL32_R_SHIFT) | channel(IM_COL32_G_SHIFT) | channel(IM_COL32_B_SHIFT) | (color & IM_COL32_A_MASK);
        }

        void Glyph(ImDrawList* list, const char* glyph, ImVec2 center, float size, ImU32 tint, float alpha, bool shadow) {
            auto* font = common::CurrentFonts().icons;
            if (!font) font = ImGui::GetFont();
            const auto extent = font->CalcTextSizeA(size, FLT_MAX, 0.0F, glyph);
            const ImVec2 pos{center.x - extent.x / 2, center.y - extent.y / 2};
            const auto a = static_cast<float>((tint >> IM_COL32_A_SHIFT) & 0xFF) / 255.0F * alpha;
            const auto color = (tint & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a * 255.0F + 0.5F) << IM_COL32_A_SHIFT);
            if (shadow) list->AddText(font, size, {pos.x + 1, pos.y + 1}, IM_COL32(0, 0, 0, static_cast<int>(a * 200)), glyph);
            list->AddText(font, size, pos, color, glyph);
        }
    }

    void RegisterPack(const std::filesystem::path& atlasJson, Priority priority) {
        const std::lock_guard lock(mutex);
        for (const auto& pack : packs)
            if (pack.file == atlasJson) return;
        packs.push_back({atlasJson, priority, nullptr, false, atlasJson.filename() == "fusion.json"});
    }

    void Load(ID3D11Device* device) {
        if (!device) return;
        if (!builtinRegistered) {
            builtinRegistered = true;
            const auto dir = AssetDir() / "icons";
            RegisterPack(dir / "fusion.json", Priority::Fallback);
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator(dir, error))  // map + per-game packs the mod ships
                if (entry.path().extension() == ".json") RegisterPack(entry.path(), Priority::Fallback);
        }
        const std::lock_guard lock(mutex);
        for (auto& pack : packs) {
            if (pack.attempted) continue;
            pack.attempted = true;
            auto atlas = std::make_unique<IconAtlas>();
            std::error_code error;
            if (std::filesystem::is_regular_file(pack.file, error) && atlas->Load(device, pack.file)) pack.atlas = std::move(atlas);
        }
    }

    bool HasArt(std::string_view key) {
        const std::lock_guard lock(mutex);
        for (const auto& pack : packs)
            if (pack.atlas && pack.atlas->Has(key)) return true;
        return false;
    }

    void DrawGame(ImDrawList* list, std::string_view key, ImVec2 center, float size, ImU32 tint, float alpha, bool shadow) {
        if (!list) return;
        const auto& appearance = modern::theme::CurrentAppearance();
        const auto game = modern::art::Pack(appearance.iconSet, appearance.family);
        const std::lock_guard lock(mutex);
        const auto result = core::icons::ResolveGame(key, game, packs.size(), [&](std::size_t i, std::string_view art) {
            return packs[i].atlas && packs[i].atlas->Has(art);
        }, [&](std::size_t i) { return packs[i].priority == Priority::Fallback; },
           [&](std::size_t i) { return packs[i].original; });
        if (result.pack >= 0) {
            auto& atlas = *packs[static_cast<std::size_t>(result.pack)].atlas;
            // Skyrim-style packs ship a black rim layer under each glyph (`<art>.back`).
            const auto back = std::string(result.art) + ".back";
            if (atlas.Has(back)) {
                const ImU32 rim = tint & IM_COL32_A_MASK;
                atlas.Draw(list, back, center, size, Palette4{rim, rim, rim, rim}, alpha, shadow);
                shadow = false;
            }
            const Palette4 palette{Scale(tint, 0.25F), Scale(tint, 0.55F), Scale(tint, 0.8F), tint};
            atlas.Draw(list, result.art, center, size, palette, alpha, shadow);
            return;
        }
        Glyph(list, result.glyph, center, size, tint, alpha, shadow);
    }

    void Draw(ImDrawList* list, std::string_view key, ImVec2 center, float size, ImU32 tint, float alpha, bool shadow, float angle,
              bool mirror) {
        if (!list) return;
        const std::lock_guard lock(mutex);
        const auto resolved = core::icons::Resolve(key, packs.size(), [&](std::size_t i, std::string_view k) {
            return packs[i].atlas && packs[i].atlas->Has(k);
        }, [&](std::size_t i) { return packs[i].priority == Priority::Fallback; });
        if (resolved.pack >= 0) {
            const Palette4 palette{Scale(tint, 0.25F), Scale(tint, 0.55F), Scale(tint, 0.8F), tint};
            packs[static_cast<std::size_t>(resolved.pack)].atlas->Draw(list, key, center, size, palette, alpha, shadow, angle, mirror);
            return;
        }
        Glyph(list, resolved.glyph, center, size, tint, alpha, shadow);
    }

    bool Draw(ImDrawList* list, std::string_view key, ImVec2 center, float size, const Palette4& palette, float alpha, bool shadow,
              float angle, bool mirror) {
        if (!list) return false;
        const std::lock_guard lock(mutex);
        const auto resolved = core::icons::Resolve(key, packs.size(), [&](std::size_t i, std::string_view k) {
            return packs[i].atlas && packs[i].atlas->Has(k);
        }, [&](std::size_t i) { return packs[i].priority == Priority::Fallback; });
        return resolved.pack >= 0 &&
               packs[static_cast<std::size_t>(resolved.pack)].atlas->Draw(list, key, center, size, palette, alpha, shadow, angle, mirror);
    }

    bool DrawRect(ImDrawList* list, std::string_view key, ImVec2 min, ImVec2 max, ImU32 tint, float alpha, bool shadow) {
        if (!list) return false;
        const std::lock_guard lock(mutex);
        const auto resolved = core::icons::Resolve(key, packs.size(), [&](std::size_t i, std::string_view k) {
            return packs[i].atlas && packs[i].atlas->Has(k);
        }, [&](std::size_t i) { return packs[i].priority == Priority::Fallback; });
        const Palette4 palette{Scale(tint, 0.25F), Scale(tint, 0.55F), Scale(tint, 0.8F), tint};
        return resolved.pack >= 0 && packs[static_cast<std::size_t>(resolved.pack)].atlas->DrawRect(list, key, min, max, palette, alpha, shadow);
    }

    void ReleaseDeviceObjects() {
        const std::lock_guard lock(mutex);
        for (auto& pack : packs) {
            pack.atlas.reset();
            pack.attempted = false;
        }
    }
}
