#pragma once
#include <RE/Fallout.h>

#include <atomic>
#include <cstddef>
#include <utility>

// CommonLib's Scaleform::RefCountImpl::AddRef increments the plain refCount instead of its atomic_ref
// (mov eax,[rcx+8]; inc eax; mov [rcx+8],eax), while FO4 counts menus with lock xadd on its UI job and render threads.
// Every Scaleform::Ptr<IMenu> copy in our DLLs (UI::GetMenu, UI::GetMenuOpen) can lose FO4's increment, and the menu
// is deleted while the UI still lists it (crash-2026-10-26-00-24-06: WorkshopMenu freed from UI::ProcessMessages).
// CommonLib can't be patched, so menu lookups go through here: IsOpen takes no reference, Ref counts atomically.
namespace fusionui::fo4::menurefs {
    namespace detail {
        struct Count : Scaleform::RefCountImplCore {
            static constexpr auto member = &Count::refCount;
        };
        inline void Retain(Scaleform::RefCountImplCore& object) {
            std::atomic_ref(const_cast<std::int32_t&>(object.*Count::member)).fetch_add(1);
        }
    }

    template <class T>
    class Ref {
    public:
        Ref() = default;
        Ref(std::nullptr_t) noexcept {}
        explicit Ref(T* menu) noexcept : menu_(menu) { if (menu_) detail::Retain(*menu_); }
        Ref(const Ref& other) noexcept : Ref(other.menu_) {}
        Ref(Ref&& other) noexcept : menu_(std::exchange(other.menu_, nullptr)) {}
        Ref& operator=(Ref other) noexcept { std::swap(menu_, other.menu_); return *this; }
        // CommonLib's Release is atomic.
        ~Ref() { if (menu_) menu_->Release(); }

        T* get() const noexcept { return menu_; }
        T* operator->() const noexcept { return menu_; }
        T& operator*() const noexcept { return *menu_; }
        explicit operator bool() const noexcept { return menu_ != nullptr; }

    private:
        T* menu_ = nullptr;
    };

    inline bool IsOpen(const RE::BSFixedString& name) {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return false;
        const RE::BSAutoReadLock lock{RE::UI::GetMenuMapRWLock()};
        const auto it = ui->menuMap.find(name);
        return it != ui->menuMap.end() && it->second.menu && it->second.menu->OnStack();
    }

    template <class T>
    bool IsOpen() { return IsOpen(T::MENU_NAME); }

    template <class T = RE::IMenu>
    Ref<T> Get(const RE::BSFixedString& name) {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return nullptr;
        const RE::BSAutoReadLock lock{RE::UI::GetMenuMapRWLock()};
        const auto it = ui->menuMap.find(name);
        return Ref<T>(it != ui->menuMap.end() ? static_cast<T*>(it->second.menu.get()) : nullptr);
    }

    template <class T>
    Ref<T> Get() { return Get<T>(T::MENU_NAME); }
}
