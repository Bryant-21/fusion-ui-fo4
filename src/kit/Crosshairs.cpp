#include "fusionui/Crosshairs.h"

#include "fusionui/Common.h"
#include "fusionui/Icons.h"
#include "fusionui/Paths.h"
#include "fusionui/Texture.h"
#include "fusionui/modern/Theme.h"
#include "core/Crosshairs.h"
#include "kit/DdsParse.h"

#include <Windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <set>
#include <vector>

namespace fusionui::crosshairs {
    namespace {
        struct Image { std::string key; Texture texture; };
        struct Store {
            ID3D11Device* device{};
            bool dirty = true;
            std::vector<Entry> entries;
            std::vector<Image> images;
            std::vector<std::string> warnings;
            ~Store() { for (auto& image : images) image.texture.Release(); }
        };
        Store& Current() { static Store store; return store; }

        std::vector<std::byte> Read(const std::filesystem::path& file, std::size_t limit) {
            std::ifstream stream(file, std::ios::binary | std::ios::ate);
            const auto size = stream.tellg();
            if (size <= 0 || size > static_cast<std::streamoff>(limit)) return {};
            std::vector<std::byte> bytes(static_cast<std::size_t>(size));
            stream.seekg(0);
            stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return stream ? bytes : std::vector<std::byte>{};
        }

        std::string Lower(std::string text) {
            for (auto& c : text) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            return text;
        }

        std::string Utf8(const std::filesystem::path& path) {
            const auto text = path.u8string();
            return {reinterpret_cast<const char*>(text.data()), text.size()};
        }

        std::optional<Texture> LoadCustom(ID3D11Device* device, const std::filesystem::path& file) {
            const auto bytes = Read(file, 4 * 1024 * 1024);
            if (bytes.empty()) return {};
            const bool dds = Lower(Utf8(file.extension())) == ".dds";
            int width{}, height{}, channels{};
            if (dds) {
                const auto info = kit::ParseDds(bytes);
                if (!info) return {};
                width = static_cast<int>(info->width);
                height = static_cast<int>(info->height);
            } else {
                constexpr std::array signature{std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
                                                std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A}};
                if (bytes.size() < signature.size() || !std::equal(signature.begin(), signature.end(), bytes.begin()) ||
                    !stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()),
                                           &width, &height, &channels)) return {};
            }
            if (width <= 0 || height <= 0 || width > 512 || height > 512) return {};
            return dds ? LoadDds(device, bytes) : LoadImageFile(device, bytes);
        }

        void Customs(Store& store) {
            std::set<std::string> keys;
            std::size_t count = 0;
            for (const auto game : core::crosshairs::Games) {
                const auto dir = Directory() / game;
                std::error_code error;
                if (std::filesystem::is_symlink(dir, error)) continue;
                std::vector<std::filesystem::path> paths;
                for (std::filesystem::directory_iterator iterator(dir, error), end; !error && iterator != end; iterator.increment(error)) {
                    const auto& file = *iterator;
                    if (file.is_symlink(error) || !file.is_regular_file(error)) continue;
                    const auto extension = Lower(Utf8(file.path().extension()));
                    if (extension != ".png" && extension != ".dds") continue;
                    paths.push_back(file.path());
                    if (++count == 64) break;
                }
                std::ranges::sort(paths, [](const auto& a, const auto& b) {
                    const bool aPng = Lower(Utf8(a.extension())) == ".png", bPng = Lower(Utf8(b.extension())) == ".png";
                    return aPng != bPng ? aPng : a.filename() < b.filename();
                });
                for (const auto& path : paths) {
                    const auto label = Utf8(path.filename());
                    const auto name = Lower(label);
                    const auto key = core::crosshairs::CustomKey(game, name);
                    if (!keys.insert(key).second) {
                        store.warnings.push_back("Skipped a duplicate custom crosshair: " + label);
                        continue;
                    }
                    auto texture = LoadCustom(store.device, path);
                    if (!texture) {
                        store.warnings.push_back("Skipped invalid crosshair (PNG/DDS, at most 512 px / 4 MiB): " + name);
                        continue;
                    }
                    store.images.push_back({key, *texture});
                    store.entries.push_back({key, label, std::string(game), name == "default.png" || name == "default.dds", true});
                }
                if (count == 64) {
                    store.warnings.emplace_back("Custom crosshairs are limited to 64 files.");
                    break;
                }
            }
        }
    }

    std::filesystem::path Directory() { return RealAssetDir() / "crosshairs"; }

    bool OpenDirectory() {
        std::error_code error;
        for (const auto game : core::crosshairs::Games) std::filesystem::create_directories(Directory() / game, error);
        // The shell may hand the folder to Explorer through COM; the render thread has not joined an apartment.
        const auto com = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        const auto opened = reinterpret_cast<std::intptr_t>(::ShellExecuteW(nullptr, L"open", Directory().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
        if (SUCCEEDED(com)) ::CoUninitialize();
        return opened > 32;
    }
    std::span<const Entry> Entries() { return Current().entries; }
    std::span<const std::string> Diagnostics() { return Current().warnings; }
    void Reload() { Current().dirty = true; }

    void ReleaseDeviceObjects() {
        auto& store = Current();
        for (auto& image : store.images) image.texture.Release();
        store.images.clear();
        store.entries.clear();
        store.device = nullptr;
        store.dirty = true;
    }

    void Load(ID3D11Device* device) {
        auto& store = Current();
        if (!device || (store.device == device && !store.dirty)) return;
        ReleaseDeviceObjects();
        store.device = device;
        store.warnings.clear();
        const auto bytes = Read(Directory() / "builtin.json", 128 * 1024);
        const auto text = std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        auto catalog = core::crosshairs::Parse(nlohmann::json::parse(text, nullptr, false));
        store.warnings = std::move(catalog.warnings);
        icons::Load(device);
        for (auto& entry : catalog.entries)
            if (entry.game != "fo3" && icons::HasArt(entry.key)) store.entries.push_back(std::move(entry));
        Customs(store);
        store.dirty = false;
    }

    bool Draw(ImDrawList* list, ImVec2 center, float size, ImU32 tint, std::string_view choice) {
        if (!list || !std::isfinite(size) || size <= 0) return false;
        const auto& appearance = modern::theme::CurrentAppearance();
        const auto pack = modern::theme::FamilyPack(appearance.family);
        const auto* entry = core::crosshairs::Resolve(Entries(), choice.empty() ? appearance.crosshair : choice, pack.empty() ? "fo4" : pack);
        if (!entry) return false;
        if (!tint) tint = common::HudColor();
        if (entry->custom) {
            for (const auto& image : Current().images) {
                if (image.key != entry->key || !image.texture) continue;
                const auto& texture = image.texture;
                const float factor = size / static_cast<float>(std::max(texture.width, texture.height));
                const ImVec2 half{texture.width * factor / 2, texture.height * factor / 2};
                list->AddImage(texture.Id(), {center.x - half.x, center.y - half.y}, {center.x + half.x, center.y + half.y}, {0, 0}, {1, 1}, tint);
                return true;
            }
            return false;
        }
        const Palette4 palette{tint, tint, tint, tint};
        return icons::Draw(list, entry->key, center, size, palette, 1, false);
    }
}
