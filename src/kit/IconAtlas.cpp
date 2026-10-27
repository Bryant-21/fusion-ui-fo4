#include "fusionui/IconAtlas.h"
#include "core/IconQuad.h"
#include "kit/IconAtlasData.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace fusionui::common {
    void BindMipSampler(const ImDrawList*, const ImDrawCmd*);  // kit/ImageModes.cpp
    void NativeSpriteCallback(ImDrawList*, ImVec4 clip, float aperture, bool monochrome, bool alphaMask, bool luminanceMask);
}

namespace fusionui {
    struct IconAtlas::Impl {
        kit::IconAtlasData data;
        Texture texture;
    };

    IconAtlas::IconAtlas() : impl_(std::make_unique<Impl>()) {}
    IconAtlas::~IconAtlas() { Release(); }

    bool IconAtlas::Load(ID3D11Device* device, const std::filesystem::path& jsonFile) {
        Release();
        std::ifstream in(jsonFile);
        if (!in) return false;
        std::stringstream text;
        text << in.rdbuf();
        auto data = kit::ParseIconAtlas(text.str());
        if (!data) return false;
        auto texture = LoadDds(device, jsonFile.parent_path() / data->texture);
        if (!texture) return false;
        impl_->data = std::move(*data);
        impl_->texture = *texture;
        return true;
    }

    void IconAtlas::Release() {
        impl_->texture.Release();
        impl_->data = {};
    }

    bool IconAtlas::Has(std::string_view symbol) const {
        return impl_->texture && impl_->data.icons.contains(std::string(symbol));
    }

    bool IconAtlas::Draw(ImDrawList* list, std::string_view symbol, ImVec2 center, float size, const Palette4& palette,
                         float alpha, bool shadow, float angle, bool mirror) const {
        const auto found = impl_->data.icons.find(std::string(symbol));
        if (found == impl_->data.icons.end()) return false;
        const float aspect = found->second.aspect;
        const ImVec2 half{size * 0.5F * std::min(aspect, 1.0F), size * 0.5F / std::max(aspect, 1.0F)};
        return DrawRect(list, symbol, {center.x - half.x, center.y - half.y}, {center.x + half.x, center.y + half.y},
                        palette, alpha, shadow, angle, mirror);
    }

    bool IconAtlas::DrawRect(ImDrawList* list, std::string_view symbol, ImVec2 min, ImVec2 max, const Palette4& palette,
                             float alpha, bool shadow, float angle, bool mirror) const {
        const auto found = impl_->data.icons.find(std::string(symbol));
        if (!impl_->texture || found == impl_->data.icons.end() || max.x <= min.x || max.y <= min.y) return false;
        const auto& icon = found->second;
        shadow = shadow && icon.shadow;
        const auto id = impl_->texture.Id();
        const ImVec2 center{(min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F};
        const ImVec2 half{(max.x - min.x) * 0.5F, (max.y - min.y) * 0.5F};
        const auto withAlpha = [alpha](ImU32 c) {
            const auto a = static_cast<ImU32>(((c >> IM_COL32_A_SHIFT) & 0xFF) * alpha);
            return (c & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
        };
        const auto quad = [&](const kit::IconRect& r, ImVec2 offset, ImU32 color, kit::IconRect bounds = {0, 0, 1, 1}) {
            float dx = (bounds.u0 + bounds.u1 - 1) * half.x;
            const float dy = (bounds.v0 + bounds.v1 - 1) * half.y;
            if (mirror) dx = -dx;
            const ImVec2 position{center.x + offset.x + dx * std::cos(angle) - dy * std::sin(angle),
                                  center.y + offset.y + dx * std::sin(angle) + dy * std::cos(angle)};
            const ImVec2 extent{half.x * (bounds.u1 - bounds.u0), half.y * (bounds.v1 - bounds.v0)};
            const auto q = core::icons::IconQuad(position, extent, angle, mirror, {r.u0, r.v0}, {r.u1, r.v1});
            list->AddImageQuad(id, q.pos[0], q.pos[1], q.pos[2], q.pos[3], q.uv[0], q.uv[1], q.uv[2], q.uv[3], withAlpha(color));
        };
        const float shift = std::max(1.0F, std::max(max.x - min.x, max.y - min.y) * 0.04F);
        list->AddCallback(&common::BindMipSampler, nullptr);
        if (icon.monochrome || icon.alphaMask || icon.luminanceMask || icon.aperture > 0 || icon.clip.u0 != 0 || icon.clip.v0 != 0 || icon.clip.u1 != 1 || icon.clip.v1 != 1)
            common::NativeSpriteCallback(list, {icon.clip.u0, icon.clip.v0, icon.clip.u1, icon.clip.v1}, icon.aperture, icon.monochrome, icon.alphaMask, icon.luminanceMask);
        if (!icon.layers.empty()) {
            for (const auto& layer : icon.layers) {
                if (shadow) quad(layer.shape, {shift, shift}, IM_COL32(0, 0, 0, 204), layer.bounds);
                quad(layer.shape, {0, 0}, palette.bright, layer.bounds);
            }
        } else if (icon.mask) {
            if (shadow) quad(icon.shape, {shift, shift}, IM_COL32(0, 0, 0, 204));
            quad(icon.shape, {0, 0}, palette.bright);
        } else if (!icon.tintable) {
            if (shadow) quad(icon.shape, {shift, shift}, IM_COL32(0, 0, 0, 204));
            quad(icon.shape, {0, 0}, IM_COL32_WHITE);
        } else {
            if (shadow) quad(icon.shape, {shift, shift}, IM_COL32(0, 0, 0, 204));
            const ImU32 tones[4]{palette.shadow, palette.dim, palette.mid, palette.bright};
            for (int i = 0; i < 4; ++i) quad(icon.bands[i], {0, 0}, tones[i]);
        }
        list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
        return true;
    }
}
