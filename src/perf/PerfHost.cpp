#include "perf/PerfHost.h"
#include "perf/Overlay.h"
#include "fusionui/FusionUI.h"
#include "fusionui/Paths.h"
#include "fusionui/Version.h"
#include "kit/Settings.h"

#include <Windows.h>
#include <ShlObj.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <format>
#include <fstream>
#include <thread>

namespace fusionui::perf {
    namespace {
        constexpr auto kSection = "Performance";
        constexpr std::uint32_t kMaxCaptureFrames = 250000;

        std::string Narrow(std::wstring_view text) {
            if (text.empty()) return {};
            std::string out(static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr)), '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), static_cast<int>(out.size()), nullptr, nullptr);
            return out;
        }
        std::string Utf8(const std::filesystem::path& path) { return Narrow(path.wstring()); }

        // An empty path is the shared FusionUI.toml; the preview points at its own file.
        fusionframework::config::Store& SettingsStore(const std::filesystem::path& file) {
            static std::optional<fusionframework::config::Store> custom;
            if (file.empty()) return settings::Shared();
            if (!custom) custom.emplace(file, "");
            return *custom;
        }

        float ReadFloat(const fusionframework::config::Document& document, const char* key, float fallback, float lo, float hi) {
            const auto value = static_cast<float>(document.GetFloat(kSection, key, fallback));
            return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
        }

        // The executable's file version ("1.10.984.0"), empty when it has none.
        std::string ExecutableVersion(std::wstring& name) {
            wchar_t path[MAX_PATH]{};
            const auto length = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
            name = std::filesystem::path(std::wstring(path, length)).filename().wstring();
            DWORD handle{};
            const auto size = ::GetFileVersionInfoSizeW(path, &handle);
            if (!size) return {};
            std::vector<std::uint8_t> data(size);
            VS_FIXEDFILEINFO* info{};
            UINT infoSize{};
            if (!::GetFileVersionInfoW(path, 0, size, data.data()) ||
                !::VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &infoSize) || !info) return {};
            return std::format("{}.{}.{}.{}", HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                               HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
        }

        std::int64_t Ticks() {
            LARGE_INTEGER now{};
            ::QueryPerformanceCounter(&now);
            return now.QuadPart;
        }
        double TickSeconds(std::int64_t ticks) {
            static const double frequency = [] {
                LARGE_INTEGER f{};
                ::QueryPerformanceFrequency(&f);
                return static_cast<double>(f.QuadPart);
            }();
            return static_cast<double>(ticks) / frequency;
        }
    }

    std::filesystem::path CaptureDirectory() {
        PWSTR documents{};
        std::filesystem::path base;
        if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents))) base = documents;
        ::CoTaskMemFree(documents);
        return base / "My Games" / "Fallout4" / "F4SE" / "FusionUI" / "Captures";
    }

    Host& Host::Get() {
        static Host host;
        return host;
    }

    double Host::Now() const { return TickSeconds(Ticks()); }

    void Host::Start(nlohmann::json info) {
        if (started_) return;
        core_.frameMs = store_.Register(kFrameMs, "Frame time", "Frame", FUSIONUI_UNIT_MS, FUSIONUI_METRIC_HEADLINE);
        core_.fps = store_.Register(kFrameFps, "FPS", "Frame", FUSIONUI_UNIT_FPS, FUSIONUI_METRIC_HEADLINE);
        core_.gpuMs = store_.Register(kGpuMs, "GPU frame", "GPU", FUSIONUI_UNIT_MS, FUSIONUI_METRIC_HEADLINE);
        core_.cpu = store_.Register(kCpuPercent, "Process CPU", "Process", FUSIONUI_UNIT_PERCENT, FUSIONUI_METRIC_HEADLINE);
        core_.ram = store_.Register(kRamMb, "Working set", "Memory", FUSIONUI_UNIT_MB, FUSIONUI_METRIC_HEADLINE);
        core_.vram = store_.Register(kVramMb, "VRAM", "Memory", FUSIONUI_UNIT_MB, FUSIONUI_METRIC_HEADLINE);
        std::wstring exe;
        const auto version = ExecutableVersion(exe);
        info_ = std::move(info);
        if (!info_.is_object()) info_ = nlohmann::json::object();
        info_["fusionUI"] = std::to_string(FUSIONUI_VERSION_MAJOR) + "." + std::to_string(FUSIONUI_VERSION_MINOR) + "." + std::to_string(FUSIONUI_VERSION_PATCH);
        info_["game"] = {{"executable", Narrow(exe)}, {"version", version}};
        LoadSettings();
        RegisterPerfClients();
        started_ = true;
        ApplyOverlay();
    }

    void Host::LoadSettings() {
        std::scoped_lock guard(mutex_);
        const auto document = SettingsStore(settingsFile_).Load();
        settings_.mode = static_cast<std::uint32_t>(std::clamp<std::int64_t>(document.GetInt(kSection, "iMode", 0), 0, 3));
        settings_.anchorX = ReadFloat(document, "fAnchorX", 1.0F, 0.0F, 1.0F);
        settings_.anchorY = ReadFloat(document, "fAnchorY", 0.0F, 0.0F, 1.0F);
        settings_.captureSeconds = ReadFloat(document, "fCaptureSeconds", 30.0F, 0.0F, 3600.0F);
        settings_.captureDelaySeconds = ReadFloat(document, "fCaptureDelay", 0.0F, 0.0F, 600.0F);
        settings_.overlayWidth = ReadFloat(document, "fOverlayWidth", 0.5F, 0.15F, 1.0F);
        settings_.overlayOpacity = ReadFloat(document, "fOverlayOpacity", 0.8F, 0.0F, 1.0F);
        pins_.Load(document.GetString(kSection, "sPinned", ""), document.GetString(kSection, "sUnpinned", ""));
        mode_ = settings_.mode;
    }

    void Host::SaveSettings() {
        FUSIONUI_PerfSettings s{};
        std::string pinned, unpinned;
        {
            std::scoped_lock guard(mutex_);
            if (!settingsDirty_) return;
            settingsDirty_ = false;
            s = settings_;
            pinned = pins_.PinnedList();
            unpinned = pins_.UnpinnedList();
        }
        SettingsStore(settingsFile_).Update([&](auto& document) {
            document.Set(kSection, "iMode", static_cast<std::int64_t>(s.mode));
            document.Set(kSection, "fAnchorX", s.anchorX);
            document.Set(kSection, "fAnchorY", s.anchorY);
            document.Set(kSection, "fCaptureSeconds", s.captureSeconds);
            document.Set(kSection, "fCaptureDelay", s.captureDelaySeconds);
            document.Set(kSection, "fOverlayWidth", s.overlayWidth);
            document.Set(kSection, "fOverlayOpacity", s.overlayOpacity);
            document.Set(kSection, "sPinned", pinned);
            document.Set(kSection, "sUnpinned", unpinned);
        });
    }

    void Host::ApplyOverlay() {
        if (!started_) return;
        auto& overlay = OverlayClient();
        const bool want = mode_.load() != FUSIONUI_PERF_MODE_OFF;
        if (want && !fusionui::IsOpen(overlay)) fusionui::Open(overlay);
        else if (!want && fusionui::IsOpen(overlay)) fusionui::Close(overlay);
    }

    std::uint32_t Host::Active() const {
        std::uint32_t bits{};
        if (mode_.load(std::memory_order_relaxed) >= FUSIONUI_PERF_MODE_EXPANDED) bits |= FUSIONUI_PERF_OVERLAY;
        const auto state = captureState_.load(std::memory_order_relaxed);
        if (state == FUSIONUI_CAPTURE_WAITING || state == FUSIONUI_CAPTURE_RECORDING) bits |= FUSIONUI_PERF_CAPTURE;
        if (Now() - pageSeenAt_.load(std::memory_order_relaxed) < 1.0) bits |= FUSIONUI_PERF_PAGE;
        return bits;
    }

    FUSIONUI_PerfSettings Host::Settings() const {
        std::scoped_lock guard(mutex_);
        return settings_;
    }

    void Host::SetSettings(const FUSIONUI_PerfSettings& in) {
        {
            std::scoped_lock guard(mutex_);
            settings_.mode = std::min<std::uint32_t>(in.mode, FUSIONUI_PERF_MODE_FULL);
            settings_.anchorX = std::isfinite(in.anchorX) ? std::clamp(in.anchorX, 0.0F, 1.0F) : settings_.anchorX;
            settings_.anchorY = std::isfinite(in.anchorY) ? std::clamp(in.anchorY, 0.0F, 1.0F) : settings_.anchorY;
            settings_.captureSeconds = std::isfinite(in.captureSeconds) ? std::clamp(in.captureSeconds, 0.0F, 3600.0F) : settings_.captureSeconds;
            settings_.captureDelaySeconds = std::isfinite(in.captureDelaySeconds) ? std::clamp(in.captureDelaySeconds, 0.0F, 600.0F)
                                                                                  : settings_.captureDelaySeconds;
            settings_.overlayWidth = std::isfinite(in.overlayWidth) ? std::clamp(in.overlayWidth, 0.15F, 1.0F) : settings_.overlayWidth;
            settings_.overlayOpacity = std::isfinite(in.overlayOpacity) ? std::clamp(in.overlayOpacity, 0.0F, 1.0F) : settings_.overlayOpacity;
            mode_ = settings_.mode;
            settingsDirty_ = true;
            settingsChangedAt_ = Now();
        }
        ApplyOverlay();
    }

    void Host::SetMode(std::uint32_t mode) {
        auto s = Settings();
        s.mode = mode;
        SetSettings(s);
    }

    void Host::CycleMode() { SetMode((mode_.load() + 1) % 4); }

    bool Host::IsPinned(std::uint32_t handle) const {
        const auto* meta = store_.Meta(handle);
        if (!meta) return false;
        std::scoped_lock guard(mutex_);
        return pins_.IsPinned(meta->id, (meta->flags & FUSIONUI_METRIC_HEADLINE) != 0);
    }

    void Host::SetPinned(std::uint32_t handle, bool pinned) {
        const auto* meta = store_.Meta(handle);
        if (!meta) return;
        std::scoped_lock guard(mutex_);
        if (pins_.Set(meta->id, (meta->flags & FUSIONUI_METRIC_HEADLINE) != 0, pinned)) {
            settingsDirty_ = true;
            settingsChangedAt_ = Now();
        }
    }

    void Host::PageVisible() { pageSeenAt_ = Now(); }

    bool Host::StartCapture() {
        std::scoped_lock guard(mutex_);
        if (!started_ || captureState_.load() != FUSIONUI_CAPTURE_IDLE) return false;
        captureSeconds_ = settings_.captureSeconds;
        captureDelay_ = settings_.captureDelaySeconds;
        captureArmedAt_ = Now();
        captureElapsed_ = 0.0F;
        captureFrames_ = 0;
        stopRequested_ = false;
        captureState_.store(FUSIONUI_CAPTURE_WAITING, std::memory_order_release);
        return true;
    }

    void Host::StopCapture() {
        const auto state = captureState_.load();
        if (state == FUSIONUI_CAPTURE_WAITING || state == FUSIONUI_CAPTURE_RECORDING) stopRequested_ = true;
    }

    void Host::ToggleCapture() {
        const auto state = captureState_.load();
        if (state == FUSIONUI_CAPTURE_IDLE) StartCapture();
        else StopCapture();
    }

    Host::CaptureView Host::Capture() const {
        CaptureView view;
        view.state = captureState_.load();
        view.elapsedSeconds = captureElapsed_.load();
        view.frames = captureFrames_.load();
        std::scoped_lock guard(mutex_);
        view.csv = lastCsv_;
        view.json = lastJson_;
        view.baseline = baselinePath_;
        return view;
    }

    bool Host::LoadBaseline(const std::filesystem::path& file) {
        try {
            std::ifstream in(file, std::ios::binary);
            if (!in) return false;
            auto averages = ReadBaseline(nlohmann::json::parse(in));
            std::scoped_lock guard(mutex_);
            baseline_ = std::move(averages);
            baselinePath_ = Utf8(file);
            return true;
        } catch (...) {
            return false;
        }
    }

    void Host::ClearBaseline() {
        std::scoped_lock guard(mutex_);
        baseline_.clear();
        baselinePath_.clear();
    }

    std::optional<double> Host::Baseline(const std::string& id) const {
        std::scoped_lock guard(mutex_);
        const auto found = baseline_.find(id);
        return found == baseline_.end() ? std::nullopt : std::optional(found->second);
    }

    void Host::DeviceLost() {
        gpu_.Release();
        process_.Release();
        sampling_ = false;
    }

    void Host::OnPresent(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (!started_) return;
        const auto active = Active();
        const bool sample = mode_.load(std::memory_order_relaxed) != FUSIONUI_PERF_MODE_OFF || active != 0;
        const auto ticks = Ticks();
        const double now = TickSeconds(ticks);
        if (sample) {
            if (!sampling_) {
                sampling_ = true;
            } else {
                const double ms = TickSeconds(ticks - lastPresent_) * 1000.0;
                store_.Push(core_.frameMs, ms);
                if (ms > 0.0) store_.Push(core_.fps, 1000.0 / ms);
            }
            lastPresent_ = ticks;
            if (const auto gpu = gpu_.Frame(device, context)) store_.Push(core_.gpuMs, *gpu);
            if (active && now - lastProcessSample_ >= 1.0) {
                lastProcessSample_ = now;
                const auto p = process_.Read(device);
                if (p.cpuPercent) store_.Push(core_.cpu, *p.cpuPercent);
                if (p.ramMb) store_.Push(core_.ram, *p.ramMb);
                if (p.vramMb) store_.Push(core_.vram, *p.vramMb);
            }

            auto state = captureState_.load(std::memory_order_acquire);
            if (state == FUSIONUI_CAPTURE_WAITING) {
                if (stopRequested_.exchange(false)) {
                    captureState_ = state = FUSIONUI_CAPTURE_IDLE;
                } else if (now - captureArmedAt_ >= captureDelay_) {
                    capture_ = {};
                    captureStartedAt_ = now;
                    captureState_ = state = FUSIONUI_CAPTURE_RECORDING;
                }
            }
            const bool recording = state == FUSIONUI_CAPTURE_RECORDING;
            store_.CloseFrame(recording ? &row_ : nullptr, capture_.rows.empty(), now);
            if (recording) {
                const double elapsed = now - captureStartedAt_;
                capture_.times.push_back(elapsed);
                capture_.rows.push_back(std::move(row_));
                captureElapsed_ = static_cast<float>(elapsed);
                captureFrames_ = static_cast<std::uint32_t>(capture_.rows.size());
                if (stopRequested_.exchange(false) || (captureSeconds_ > 0.0F && elapsed >= captureSeconds_) ||
                    capture_.rows.size() >= kMaxCaptureFrames)
                    FinishCapture();
            }
        } else if (sampling_) {
            sampling_ = false;
            gpu_.Abandon(context);
        }
        bool save{};
        {
            std::scoped_lock guard(mutex_);
            save = settingsDirty_ && now - settingsChangedAt_ >= 1.0;
        }
        if (save) SaveSettings();
    }

    void Host::FinishCapture() {
        captureState_ = FUSIONUI_CAPTURE_WRITING;
        auto data = std::move(capture_);
        capture_ = {};
        for (std::uint32_t i = 1; i <= store_.Count(); ++i) data.columns.push_back(*store_.Meta(i));
        const auto wall = std::time(nullptr) - static_cast<std::time_t>(data.times.empty() ? 0.0 : data.times.back());
        std::tm local{};
        ::localtime_s(&local, &wall);
        const auto stem = CaptureStem(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
        {
            std::scoped_lock guard(mutex_);
            data.info = info_;
            data.info["timestamp"] = std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}", local.tm_year + 1900, local.tm_mon + 1,
                                                 local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
            data.info["settings"] = {{"mode", settings_.mode}, {"captureSeconds", captureSeconds_}, {"captureDelaySeconds", captureDelay_}};
            if (!baselinePath_.empty()) data.info["baseline"] = baselinePath_;
        }
        std::thread([this, data = std::move(data), stem] {
            std::string csv, json;
            try {
                const auto dir = CaptureDirectory();
                std::error_code error;
                std::filesystem::create_directories(dir, error);
                if (WriteCsv(data, dir / (stem + ".csv"))) csv = Utf8(dir / (stem + ".csv"));
                if (WriteJson(data, dir / (stem + ".json"))) json = Utf8(dir / (stem + ".json"));
            } catch (...) {}
            {
                std::scoped_lock guard(mutex_);
                lastCsv_ = std::move(csv);
                lastJson_ = std::move(json);
            }
            captureState_ = FUSIONUI_CAPTURE_IDLE;
        }).detach();
    }

    namespace {
        template <class T>
        void WriteSized(T* out, T value) {
            if (!out) return;
            const auto size = std::min<std::size_t>(out->size, sizeof(T));
            value.size = static_cast<std::uint32_t>(size);
            std::memcpy(out, &value, size);
        }

        FUSIONUI_MetricId ApiRegisterMetric(const FUSIONUI_MetricDesc* desc) noexcept {
            if (!desc || desc->size < offsetof(FUSIONUI_MetricDesc, flags) + sizeof(desc->flags) || !desc->id) return 0;
            try {
                return Host::Get().Store().Register(desc->id, desc->label ? desc->label : "", desc->group ? desc->group : "",
                                                    desc->unit, desc->flags);
            } catch (...) { return 0; }
        }
        void ApiPush(FUSIONUI_MetricId id, double value) noexcept { Host::Get().Store().Push(id, value); }
        void ApiPushText(FUSIONUI_MetricId id, const char* text) noexcept {
            try { Host::Get().Store().PushText(id, text ? text : ""); } catch (...) {}
        }
        std::uint32_t ApiActive() noexcept { return Host::Get().Active(); }
        std::uint32_t ApiMetrics(FUSIONUI_MetricInfo* out, std::uint32_t capacity) noexcept {
            auto& host = Host::Get();
            auto& store = host.Store();
            const auto count = store.Count();
            if (!out || !capacity) return count;
            const std::size_t stride = out->size;
            if (stride < offsetof(FUSIONUI_MetricInfo, text)) return count;
            thread_local std::vector<std::string> texts;
            try {
                texts.assign(std::min(count, capacity), {});
                for (std::uint32_t i = 0; i < count && i < capacity; ++i) {
                    const auto* meta = store.Meta(i + 1);
                    MetricStore::Snapshot snapshot;
                    store.Read(i + 1, snapshot);
                    texts[i] = std::move(snapshot.text);
                    FUSIONUI_MetricInfo info{};
                    info.id = i + 1;
                    info.key = meta->id.c_str();
                    info.owner = meta->owner.c_str();
                    info.label = meta->label.c_str();
                    info.group = meta->group.c_str();
                    info.unit = meta->unit;
                    info.flags = meta->flags;
                    info.pinned = host.IsPinned(i + 1) ? 1u : 0u;
                    info.hasValue = snapshot.hasValue ? 1u : 0u;
                    info.current = snapshot.current;
                    info.average = snapshot.stats.average;
                    info.minimum = snapshot.stats.minimum;
                    info.maximum = snapshot.stats.maximum;
                    info.p50 = snapshot.stats.p50;
                    info.p99 = snapshot.stats.p99;
                    info.text = texts[i].c_str();
                    if (const auto baseline = host.Baseline(meta->id)) {
                        info.hasBaseline = 1;
                        info.baselineAverage = *baseline;
                    }
                    auto* slot = reinterpret_cast<FUSIONUI_MetricInfo*>(reinterpret_cast<std::uint8_t*>(out) + stride * i);
                    info.size = static_cast<std::uint32_t>(std::min(stride, sizeof(info)));
                    std::memcpy(slot, &info, info.size);
                }
            } catch (...) {}
            return count;
        }
        std::uint32_t ApiHistoryTimed(FUSIONUI_MetricId id, float* values, double* seconds, std::uint32_t capacity) noexcept {
            return Host::Get().Store().HistoryTimed(id, values, seconds, capacity);
        }
        double ApiNow() noexcept { return Host::Get().Now(); }
        std::uint32_t ApiHistory(FUSIONUI_MetricId id, float* out, std::uint32_t capacity) noexcept {
            return Host::Get().Store().History(id, out, capacity);
        }
        void ApiGetSettings(FUSIONUI_PerfSettings* out) noexcept { WriteSized(out, Host::Get().Settings()); }
        void ApiSetSettings(const FUSIONUI_PerfSettings* in) noexcept {
            if (!in) return;
            auto merged = Host::Get().Settings();
            std::memcpy(&merged, in, std::min<std::size_t>(in->size, sizeof(merged)));
            merged.size = sizeof(merged);
            try { Host::Get().SetSettings(merged); } catch (...) {}
        }
        void ApiSetPinned(FUSIONUI_MetricId id, std::uint32_t pinned) noexcept {
            try { Host::Get().SetPinned(id, pinned != 0); } catch (...) {}
        }
        void ApiPageVisible() noexcept { Host::Get().PageVisible(); }
        std::uint32_t ApiCaptureStart() noexcept { return Host::Get().StartCapture() ? 1u : 0u; }
        void ApiCaptureStop() noexcept { Host::Get().StopCapture(); }
        void ApiCaptureStatus(FUSIONUI_CaptureStatus* out) noexcept {
            if (!out) return;
            thread_local Host::CaptureView view;
            try { view = Host::Get().Capture(); } catch (...) { return; }
            FUSIONUI_CaptureStatus status{sizeof(status), view.state, view.elapsedSeconds, view.frames, view.csv.c_str(),
                                       view.json.c_str(), view.baseline.c_str()};
            WriteSized(out, status);
        }
        std::uint32_t ApiLoadBaseline(const char* path) noexcept {
            if (!path) return 0;
            try {
                const std::string text(path);
                return Host::Get().LoadBaseline(std::filesystem::path(std::u8string(text.begin(), text.end()))) ? 1u : 0u;
            } catch (...) { return 0; }
        }
        void ApiClearBaseline() noexcept { Host::Get().ClearBaseline(); }
    }

    void FillHostApi(FUSIONUI_HostApi& t) {
        t.perfRegisterMetric = &ApiRegisterMetric;
        t.perfPush = &ApiPush;
        t.perfPushText = &ApiPushText;
        t.perfActive = &ApiActive;
        t.perfMetrics = &ApiMetrics;
        t.perfHistory = &ApiHistory;
        t.perfGetSettings = &ApiGetSettings;
        t.perfSetSettings = &ApiSetSettings;
        t.perfSetPinned = &ApiSetPinned;
        t.perfPageVisible = &ApiPageVisible;
        t.perfCaptureStart = &ApiCaptureStart;
        t.perfCaptureStop = &ApiCaptureStop;
        t.perfCaptureStatus = &ApiCaptureStatus;
        t.perfLoadBaseline = &ApiLoadBaseline;
        t.perfClearBaseline = &ApiClearBaseline;
        t.perfHistoryTimed = &ApiHistoryTimed;
        t.perfNow = &ApiNow;
    }
}
