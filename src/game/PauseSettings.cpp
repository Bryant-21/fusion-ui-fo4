#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "game/PauseSettings.h"
#include "fusionui/PauseMenu.h"
#include "fusionui/Settings.h"
#include "fusionui/fo4/MenuRefs.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

namespace fusionui::pause {
namespace {

using Value = Scaleform::GFx::Value;

// Rows carry fusionuiEntry (the id) and fusionuiOwner (this plugin's copy of fusionui), so two plugins that both add
// MOD SETTINGS or the same id never answer each other's press.
const std::string& Owner()
{
    static const std::string owner = std::format("{:x}", reinterpret_cast<std::uintptr_t>(&Owner));
    return owner;
}

std::mutex entriesMutex;
std::vector<Entry> entries;

bool Flagged(const Value& row, const char* flag)
{
    Value value;
    return row.IsObject() && row.GetMember(flag, &value) && value.IsBoolean() && value.GetBoolean();
}

std::string StringMember(const Value& row, const char* name)
{
    Value value;
    return row.IsObject() && row.GetMember(name, &value) && value.IsString() ? value.GetString() : "";
}

bool Ours(const Value& row) { return StringMember(row, "fusionuiOwner") == Owner(); }

std::optional<double> Number(const Value& value)
{
    if (value.IsNumber()) return value.GetNumber();
    if (value.IsInt()) return value.GetInt();
    if (value.IsUInt()) return value.GetUInt();
    return std::nullopt;
}

bool TextIs(const Value& row, std::string_view text)
{
    Value value;
    return row.IsObject() && row.GetMember("text", &value) && value.IsString() && value.GetString() == text;
}

// Any row a B21 plugin added (current or older fusionui builds, Tales' former PHOTO MODE flag).
bool B21Row(const Value& row)
{
    return !StringMember(row, "fusionuiEntry").empty() || Flagged(row, "FusionUISettings") || Flagged(row, "b21PhotoMode");
}

class EntryPress final : public Scaleform::GFx::FunctionHandler {
public:
    explicit EntryPress(Value list) : list_(std::move(list)) {}
    void Call(const Params& args) override
    {
        Value row;
        if (!list_.GetMember("selectedEntry", &row) || !row.IsObject() || !Ours(row)) return;
        if (args.argCount && args.args[0].IsObject()) args.args[0].Invoke("stopImmediatePropagation");
        if (Flagged(row, "disabled")) return;
        const auto id = StringMember(row, "fusionuiEntry");
        std::function<void()> activate;
        {
            const std::lock_guard lock(entriesMutex);
            for (const auto& entry : entries)
                if (entry.id == id) activate = entry.activate;
        }
        if (activate) activate();
    }

private:
    Value list_;
};

bool FindMainMenu(RE::IMenu& menu, Scaleform::GFx::Movie& movie, Value& main)
{
    Value panel;
    if (menu.menuObj.IsObject() && menu.menuObj.GetMember("MainPanel_mc", &panel) && panel.IsObject()) {
        main = menu.menuObj;
        return true;
    }
    for (const auto* path : {"root.Menu_mc", "root1.Menu_mc", "root", "root1"})
        if (movie.GetVariable(&main, path) && main.IsObject() && main.GetMember("MainPanel_mc", &panel) && panel.IsObject())
            return true;
    return false;
}

// The pause list is authored with 9 row clips and a border that fits 9 rows; vanilla AE fills 8, so added
// rows would only be reachable by scrolling. SetNumListItems is protected, so the extra clips are built the
// way it builds them, wired to the list's public handlers.
const char* GrowList(Scaleform::GFx::Movie& movie, Value& list, std::uint32_t rows)
{
    Value clips, first, holder, platform, rollover, press;
    const auto current = list.GetMember("numListItems", &clips) ? Number(clips) : std::nullopt;
    if (!current) return "list has no numListItems";
    if (rows <= *current) return nullptr;
    const std::array firstIndex{Value(0u)};
    if (!list.Invoke("GetClipByIndex", &first, firstIndex) || !first.IsDisplayObject() ||
        !first.GetMember("parent", &holder) || !holder.IsDisplayObject() ||
        !list.GetMember("onEntryRollover", &rollover) || !list.GetMember("onEntryPress", &press))
        return "list row clips are not reachable";
    first.GetMember("iPlatform", &platform);
    for (auto index = static_cast<std::uint32_t>(*current); index < rows; ++index) {
        Value clip;
        movie.CreateObject(&clip, "MainMenuListEntry");
        if (!clip.IsDisplayObject()) return "MainMenuListEntry could not be created";
        if (!platform.IsUndefined()) clip.SetMember("iPlatform", platform);
        clip.SetMember("clipIndex", Value(index));
        const std::array over{Value("mouseOver"), rollover};
        const std::array click{Value("click"), press};
        const std::array child{clip};
        if (!clip.Invoke("addEventListener", over) || !clip.Invoke("addEventListener", click) ||
            !holder.Invoke("addChild", child))
            return "new row clip could not be attached";
    }
    list.SetMember("numListItems", Value(rows));

    // UpdateList stops adding rows once they exceed border.height; size it the way
    // CalculateMaxScrollPosition measures rows (the first clip's defaultHeight or height).
    Value border, height, defaultHeight, spacing, borderHeight;
    first.GetMember("height", &height);
    first.GetMember("defaultHeight", &defaultHeight);
    list.GetMember("verticalSpacing", &spacing);
    const auto row = std::max(Number(height).value_or(0), Number(defaultHeight).value_or(0));
    if (row <= 0 || !list.GetMember("border", &border) || !border.IsDisplayObject() ||
        !border.GetMember("height", &borderHeight))
        return "list border could not be measured";
    const auto needed = rows * row + (rows - 1) * Number(spacing).value_or(0) + 1;
    if (Number(borderHeight).value_or(0) < needed) border.SetMember("height", Value(needed));
    return nullptr;
}

constexpr std::string_view kSettingsId = "fusionui.settings";

// Per pause-menu / title-menu movie; reset when the menu closes because a new movie can reuse the address.
Scaleform::GFx::Movie* listenedMovie{};
bool listening{};
bool announced{};
std::uint32_t failedFrames{};

class PauseClose final : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
    {
        if (!event.opening && (event.menuName == "PauseMenu" || event.menuName == "MainMenu")) {
            listenedMovie = nullptr;
            announced = false;
            failedFrames = 0;
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
PauseClose pauseClose;

struct Wanted {
    std::string id, label;
    bool shown, enabled;
};

// Returns why the rows are not in the list, or nullptr once they are. PauseMenu and MainMenu each have their own
// vtable hooks, so no PauseMode check is needed (MainMenu.PauseMode is private, which native GetMember cannot read).
// shortcut: the title menu shows only MOD SETTINGS (the other rows, like Tales' PHOTO MODE, need a loaded game);
// give Entry a title flag when a plugin needs its own title row (an ABI change: every caller rebuilds).
const char* EnsureEntries(RE::IMenu& menu, bool title)
{
    std::vector<Wanted> wanted;
    {
        const std::lock_guard lock(entriesMutex);
        for (const auto& entry : entries)
            if (!title || entry.id == kSettingsId) wanted.push_back({entry.id, entry.label, !entry.shown || entry.shown(), !entry.enabled || entry.enabled()});
    }
    if (wanted.empty()) return nullptr;
    auto* movie = menu.uiMovie.get();
    if (!movie) return "pause menu has no movie";
    Value main, panel, list, rows;
    if (!FindMainMenu(menu, *movie, main)) return "MainMenu object not found";
    if (!main.GetMember("MainPanel_mc", &panel) || !panel.GetMember("List_mc", &list) || !list.IsObject())
        return "MainPanel_mc.List_mc not found";
    if (!list.GetMember("entryList", &rows) || !rows.IsArray()) return "entryList is not an array";
    if (rows.GetArraySize() == 0) return "entryList is empty";

    if (listenedMovie != movie) {
        listenedMovie = movie;
        Value callback;
        Scaleform::Ptr<EntryPress> handler{new EntryPress(list)};
        movie->CreateFunction(&callback, handler.get());
        const std::array args{Value("BSScrollingList::itemPress"), callback, Value(false), Value(1000)};
        listening = list.Invoke("addEventListener", args);
    }
    if (!listening) return "list refused the press listener";

    bool changed = false;
    for (const auto& want : wanted) {
        const auto count = rows.GetArraySize();
        std::optional<std::uint32_t> found;
        bool elsewhere = false;  // another plugin (or an older fusionui) already shows MOD SETTINGS
        std::uint32_t position = count;
        for (std::uint32_t i = 0; i < count; ++i) {
            Value row;
            if (!rows.GetElement(i, &row)) continue;
            if (StringMember(row, "fusionuiEntry") == want.id && Ours(row)) found = i;
            else if (want.id == kSettingsId && Flagged(row, "FusionUISettings")) elsewhere = true;
            if (B21Row(row) || TextIs(row, "$SETTINGS")) position = i + 1;
        }
        if (elsewhere && !found) continue;
        if (found && !want.shown) {
            const std::array splice{Value(*found), Value(1u)};
            rows.Invoke("splice", splice);
            changed = true;
            continue;
        }
        if (found) {
            Value row, disabled;
            rows.GetElement(*found, &row);
            if (!row.GetMember("disabled", &disabled) || !disabled.IsBoolean() || disabled.GetBoolean() == want.enabled) {
                row.SetMember("disabled", Value(!want.enabled));
                changed = true;
            }
            continue;
        }
        if (!want.shown) continue;
        Value row;
        movie->CreateObject(&row);
        row.SetMember("text", Value(want.label.c_str()));
        row.SetMember("index", Value(-210077));
        row.SetMember("disabled", Value(!want.enabled));
        row.SetMember("fusionuiEntry", Value(want.id.c_str()));
        row.SetMember("fusionuiOwner", Value(Owner().c_str()));
        if (want.id == kSettingsId) row.SetMember("FusionUISettings", Value(true));
        const std::array splice{Value(position), Value(0u), row};
        if (!rows.Invoke("splice", splice)) return "entryList.splice failed";
        changed = true;
    }
    if (!changed) return nullptr;
    const auto* notGrown = GrowList(*movie, list, rows.GetArraySize());
    if (!list.Invoke("InvalidateData")) return "InvalidateData failed";
    if (!announced) {
        announced = true;
        spdlog::info("FUSIONUI: {} pause-menu row(s) added ({} rows){}{}", wanted.size(), rows.GetArraySize(),
            notGrown ? "; list not grown, rows need scrolling: " : "", notGrown ? notGrown : "");
    }
    return nullptr;
}

using AdvanceFn = void (*)(RE::IMenu*, float, std::uint64_t);
using InitListFn = void (*)(RE::IMenu*);
REL::Relocation<AdvanceFn> advancePause, advanceTitle;
REL::Relocation<InitListFn> initPauseList, initTitleList;

void Ensure(RE::IMenu* menu, bool title)
{
    if (!menu) return;
    // The first frames after opening have no list yet, so only a failure that persists is reported (once:
    // a menu replacer that moved the list keeps it moved).
    if (const auto* failure = EnsureEntries(*menu, title); !failure) failedFrames = 0;
    else if (++failedFrames == 120) spdlog::warn("FUSIONUI: {}-menu rows not added: {}", title ? "title" : "pause", failure);
}

void AdvancePause(RE::IMenu* menu, float delta, std::uint64_t time)
{
    advancePause(menu, delta, time);
    Ensure(menu, false);
}

void AdvanceTitle(RE::IMenu* menu, float delta, std::uint64_t time)
{
    advanceTitle(menu, delta, time);
    Ensure(menu, true);
}

// StartMenuBase::InitMainList rebuilds the list through the movie's InitList; adding the rows right after it
// means no native rebuild reaches the screen without them. AS3 also rebuilds on its own when backing out of
// a panel, which the per-frame check covers.
void InitPauseList(RE::IMenu* menu)
{
    initPauseList(menu);
    if (menu) EnsureEntries(*menu, false);
}

void InitTitleList(RE::IMenu* menu)
{
    initTitleList(menu);
    if (menu) EnsureEntries(*menu, true);
}

void InstallHooks()
{
    static bool installed{};
    if (std::exchange(installed, true)) return;
    if (auto* ui = RE::UI::GetSingleton()) ui->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(&pauseClose);
    REL::Relocation<std::uintptr_t> table{RE::VTABLE::PauseMenu[0]};
    advancePause = table.write_vfunc(0x04, AdvancePause);
    initPauseList = table.write_vfunc(0x15, InitPauseList);
    REL::Relocation<std::uintptr_t> title{RE::VTABLE::MainMenu[0]};
    advanceTitle = title.write_vfunc(0x04, AdvanceTitle);
    initTitleList = title.write_vfunc(0x15, InitTitleList);
}
}

void AddEntry(Entry entry)
{
    {
        const std::lock_guard lock(entriesMutex);
        entries.push_back(std::move(entry));
    }
    InstallHooks();
}
}

namespace fusionui::game::PauseSettings {
void Install()
{
    pause::AddEntry({.id = std::string(pause::kSettingsId), .label = "MOD SETTINGS", .shown = [] { return !SettingsPanels().empty(); },
        .activate = [] {
            // Settings bring their own cursor and pause; a pause menu left open under them kept its own
            // cursor and input as well. The title menu stays open under them (its input is stopped) to come back to.
            if (OpenSettings() && fusionui::fo4::menurefs::IsOpen(RE::BSFixedString("PauseMenu")))
                if (auto* queue = RE::UIMessageQueue::GetSingleton())
                    queue->AddMessage(RE::BSFixedString("PauseMenu"), RE::UI_MESSAGE_TYPE::kHide);
        }});
    spdlog::info("FUSIONUI: shared settings pause-menu row registered");
}
}
