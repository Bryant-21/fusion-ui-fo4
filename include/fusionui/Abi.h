#pragma once
#include <stddef.h>
#include <stdint.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

#ifdef __cplusplus
extern "C" {
#endif

#define FUSIONUI_ABI_VERSION 1u

typedef uint32_t FUSIONUI_ClientId;
typedef void (*FUSIONUI_TaskCallback)(void* user);

enum { FUSIONUI_DEVICE_KEYBOARD_MOUSE = 0, FUSIONUI_DEVICE_GAMEPAD = 1 };
enum { FUSIONUI_KIND_MODAL = 0, FUSIONUI_KIND_OVERLAY = 1 };
enum { FUSIONUI_FLAG_PAUSES_GAME = 1u << 0, FUSIONUI_FLAG_SETTINGS = 1u << 1, FUSIONUI_FLAG_CAPTURES_CHARACTERS = 1u << 2 };
enum { FUSIONUI_FRAME_DRAW_CURSOR = 1u << 0 };

enum FUSIONUI_EventType {
    FUSIONUI_EV_MOUSE_BUTTON = 1, /* code = button 0..4, down */
    FUSIONUI_EV_MOUSE_WHEEL = 2,  /* y = +1 up / -1 down */
    FUSIONUI_EV_KEY = 3,          /* code = Win32 VK, down */
    FUSIONUI_EV_CHAR = 4,         /* code = UTF-16 unit */
    FUSIONUI_EV_PAD_BUTTON = 5,   /* code = FUSIONUI_PAD_*, down, x = analog value */
    FUSIONUI_EV_PAD_STICK = 6,    /* code = FUSIONUI_STICK_*, x, y in [-1, 1], +y = up */
    FUSIONUI_EV_FOCUS = 7         /* down = 1 window gained focus, 0 lost */
};

enum FUSIONUI_PadButton {
    FUSIONUI_PAD_A = 0, FUSIONUI_PAD_B, FUSIONUI_PAD_X, FUSIONUI_PAD_Y,
    FUSIONUI_PAD_LB, FUSIONUI_PAD_RB, FUSIONUI_PAD_LT, FUSIONUI_PAD_RT,
    FUSIONUI_PAD_BACK, FUSIONUI_PAD_START, FUSIONUI_PAD_LS, FUSIONUI_PAD_RS,
    FUSIONUI_PAD_DPAD_UP, FUSIONUI_PAD_DPAD_DOWN, FUSIONUI_PAD_DPAD_LEFT, FUSIONUI_PAD_DPAD_RIGHT,
    FUSIONUI_PAD_COUNT
};

enum { FUSIONUI_STICK_LEFT = 0, FUSIONUI_STICK_RIGHT = 1 };

typedef struct FUSIONUI_Event {
    uint32_t type;
    uint32_t code;
    uint32_t down;
    float x;
    float y;
} FUSIONUI_Event;

typedef struct FUSIONUI_Frame {
    uint32_t size;
    uint32_t flags;
    float displayWidth;
    float displayHeight;
    float deltaSeconds;
    float cursorX;
    float cursorY;
    uint32_t activeDevice;
    uint32_t focused;
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    const FUSIONUI_Event* events;
    uint32_t eventCount;
    uint32_t hudColor; /* player HUD color 0xAARRGGBB; alpha 0 (or a smaller `size`) = host did not send it */
} FUSIONUI_Frame;

typedef struct FUSIONUI_ClientDesc {
    uint32_t size;
    const char* name;
    uint32_t kind;
    uint32_t flags;
    void* user;
    void (*render)(void* user, const FUSIONUI_Frame* frame);
    void (*deviceLost)(void* user);
    void (*focusChanged)(void* user, uint32_t focused);
    const char* settingsLabel;
    const char* settingsIcon;
    const char* settingsCategory;
} FUSIONUI_ClientDesc;

typedef struct FUSIONUI_SettingsPanel {
    FUSIONUI_ClientId id;
    const char* label;
    uint32_t selected;
    const char* icon;
} FUSIONUI_SettingsPanel;

typedef struct FUSIONUI_KeybindingProvider {
    uint32_t size;
    const char* id;
    const char* label;
    void* user;
    /* UTF-8 JSON array, copied immediately. Callback and user live for the plugin's lifetime. */
    const char* (*snapshot)(void* user);
} FUSIONUI_KeybindingProvider;

/* Performance metrics (framework 14). Ids are "owner.group.name"; the owner is the text before the first dot. */
typedef uint32_t FUSIONUI_MetricId;

enum FUSIONUI_MetricUnit {
    FUSIONUI_UNIT_MS = 0, FUSIONUI_UNIT_COUNT = 1, FUSIONUI_UNIT_MB = 2, FUSIONUI_UNIT_PERCENT = 3,
    FUSIONUI_UNIT_FPS = 4, FUSIONUI_UNIT_RATIO = 5, FUSIONUI_UNIT_TEXT = 6
};
enum { FUSIONUI_METRIC_HEADLINE = 1u << 0 };
enum { FUSIONUI_PERF_OVERLAY = 1u << 0, FUSIONUI_PERF_CAPTURE = 1u << 1, FUSIONUI_PERF_PAGE = 1u << 2 };
enum { FUSIONUI_PERF_MODE_OFF = 0, FUSIONUI_PERF_MODE_COMPACT = 1, FUSIONUI_PERF_MODE_EXPANDED = 2, FUSIONUI_PERF_MODE_FULL = 3 };
enum { FUSIONUI_CAPTURE_IDLE = 0, FUSIONUI_CAPTURE_WAITING = 1, FUSIONUI_CAPTURE_RECORDING = 2, FUSIONUI_CAPTURE_WRITING = 3 };

typedef struct FUSIONUI_MetricDesc {
    uint32_t size;
    const char* id;
    const char* label;
    const char* group;
    uint32_t unit;
    uint32_t flags;
} FUSIONUI_MetricDesc;

/* Strings live until the next perfMetrics call on the same thread. */
typedef struct FUSIONUI_MetricInfo {
    uint32_t size;
    FUSIONUI_MetricId id;
    const char* key;
    const char* owner;
    const char* label;
    const char* group;
    uint32_t unit;
    uint32_t flags;
    uint32_t pinned;
    uint32_t hasValue;
    double current, average, minimum, maximum, p50, p99;
    const char* text;
    uint32_t hasBaseline;
    double baselineAverage;
} FUSIONUI_MetricInfo;

typedef struct FUSIONUI_PerfSettings {
    uint32_t size;
    uint32_t mode;
    float anchorX, anchorY;
    float captureSeconds;
    float captureDelaySeconds;
    float overlayWidth;   /* Full overlay width as a share of the screen width */
    float overlayOpacity; /* overlay background alpha */
} FUSIONUI_PerfSettings;

typedef struct FUSIONUI_CaptureStatus {
    uint32_t size;
    uint32_t state;
    float elapsedSeconds;
    uint32_t frames;
    const char* lastCsvPath;  /* UTF-8, empty until a capture finished; lives until the next call */
    const char* lastJsonPath;
    const char* baselinePath;
} FUSIONUI_CaptureStatus;

typedef struct FUSIONUI_HostApi {
    uint32_t size;
    uint32_t abiVersion;
    FUSIONUI_ClientId (*registerClient)(const FUSIONUI_ClientDesc* desc);
    uint32_t (*open)(FUSIONUI_ClientId id);
    void (*close)(FUSIONUI_ClientId id);
    uint32_t (*isOpen)(FUSIONUI_ClientId id);
    uint32_t (*available)(void);
    uint32_t (*activeDevice)(void);
    void (*setCursor)(float x, float y);
    void (*setPausesGame)(FUSIONUI_ClientId id, uint32_t pausesGame);
    /* Takes ownership of user; destroy runs in the caller's module after execution or discard. */
    void (*queueGameTask)(void* user, FUSIONUI_TaskCallback run, FUSIONUI_TaskCallback destroy);
    /* Returns the total count; writes up to capacity entries. Labels live for the host's lifetime. */
    uint32_t (*settingsPanels)(FUSIONUI_SettingsPanel* panels, uint32_t capacity);
    uint32_t (*openSettings)(FUSIONUI_ClientId id);
    const char* (*settingsCategory)(FUSIONUI_ClientId id);
    uint32_t (*registerKeybindings)(const FUSIONUI_KeybindingProvider* provider);
    /* Framework 14. Registering an existing id returns the same handle. Push is any-thread; values
       pushed between two Presents belong to the frame that the second Present ends. */
    FUSIONUI_MetricId (*perfRegisterMetric)(const FUSIONUI_MetricDesc* desc);
    void (*perfPush)(FUSIONUI_MetricId id, double value);
    void (*perfPushText)(FUSIONUI_MetricId id, const char* utf8);
    /* FUSIONUI_PERF_* bits; zero means nobody is looking and providers must do no work. */
    uint32_t (*perfActive)(void);
    /* Returns the total count; writes up to capacity entries. */
    uint32_t (*perfMetrics)(FUSIONUI_MetricInfo* out, uint32_t capacity);
    /* Oldest first; returns the count written. */
    uint32_t (*perfHistory)(FUSIONUI_MetricId id, float* out, uint32_t capacity);
    void (*perfGetSettings)(FUSIONUI_PerfSettings* out);
    void (*perfSetSettings)(const FUSIONUI_PerfSettings* settings);
    void (*perfSetPinned)(FUSIONUI_MetricId id, uint32_t pinned);
    /* A page that is drawn this frame keeps FUSIONUI_PERF_PAGE set for about a second. */
    void (*perfPageVisible)(void);
    uint32_t (*perfCaptureStart)(void);
    void (*perfCaptureStop)(void);
    void (*perfCaptureStatus)(FUSIONUI_CaptureStatus* out);
    uint32_t (*perfLoadBaseline)(const char* jsonPathUtf8);
    void (*perfClearBaseline)(void);
    /* Framework 15. As perfHistory, with each sample's frame time on the host clock (seconds). */
    uint32_t (*perfHistoryTimed)(FUSIONUI_MetricId id, float* values, double* seconds, uint32_t capacity);
    /* The host clock now, in seconds. */
    double (*perfNow)(void);
} FUSIONUI_HostApi;

#define FUSIONUI_HOSTAPI_MIN_SIZE ((uint32_t)(offsetof(FUSIONUI_HostApi, setCursor) + sizeof(void*)))
#define FUSIONUI_CLIENTDESC_MIN_SIZE ((uint32_t)(offsetof(FUSIONUI_ClientDesc, focusChanged) + sizeof(void*)))

#ifdef __cplusplus
}
#endif
