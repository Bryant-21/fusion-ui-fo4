#include "fusionui/Paths.h"
#include "kit/PathsPure.h"

#include <Windows.h>
#include <Psapi.h>

#include <optional>

namespace fusionui {
    namespace {
        std::optional<std::filesystem::path>& Override() {
            static std::optional<std::filesystem::path> dir;
            return dir;
        }
    }

    void SetAssetDirOverride(std::filesystem::path dir) { Override() = std::move(dir); }

    std::filesystem::path AssetDirOf(const void* address) {
        if (Override()) return *Override();
        HMODULE module{};
        ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             static_cast<LPCWSTR>(address), &module);
        wchar_t buffer[MAX_PATH]{};
        const auto length = ::GetModuleFileNameW(module, buffer, MAX_PATH);
        return kit::AssetDirForModule(std::filesystem::path(std::wstring(buffer, length)));
    }

    std::filesystem::path AssetDir() { return AssetDirOf(reinterpret_cast<const void*>(&AssetDir)); }

    std::filesystem::path RealAssetDir() {
        if (Override()) return *Override();
        // The DLL image's mapping names the file it was really loaded from, as \Device\<volume>\... .
        wchar_t buffer[MAX_PATH * 2]{};
        const std::wstring mapped(buffer, ::GetMappedFileNameW(::GetCurrentProcess(), reinterpret_cast<void*>(&RealAssetDir), buffer,
                                                               static_cast<DWORD>(std::size(buffer))));
        constexpr std::wstring_view kNetwork = L"\\Device\\Mup\\";
        if (mapped.starts_with(kNetwork)) return kit::AssetDirForModule(L"\\\\" + mapped.substr(kNetwork.size()));
        for (wchar_t drive[] = L"A:"; drive[0] <= L'Z'; ++drive[0]) {
            wchar_t device[MAX_PATH]{};
            if (!::QueryDosDeviceW(drive, device, MAX_PATH)) continue;
            const std::wstring prefix = std::wstring(device) + L'\\';
            if (mapped.starts_with(prefix)) return kit::AssetDirForModule(drive + mapped.substr(prefix.size() - 1));
        }
        return AssetDir();
    }
}
