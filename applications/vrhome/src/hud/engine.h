#pragma once

#include "../engine.h"
#include "../math/mat4.h"
#include "../panels/panel.h"
#include "../dock/item.h"
#include "../grid/item.h"
#include "../notif/item.h"
#include "../sysmsg/item.h"
#include "../quick/item.h"
#include "../set/item.h"
#include "../kbd/kbd.h"
#include "../common/config.h"
#include "../input/ctrl_state.h"
#include "../input/input_state.h"
#include "../shell/backend.h"

#ifdef __ANDROID__
#include <android/native_window.h>
#endif

#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <vector>

// one key press handed from the service's window to the render thread
struct KeyIn { int code, action, repeat; };

// HUD-side state: everything the overlay needs on top of the shared render
// state. The surface arrives and leaves via SurfaceHolder callbacks on the
// main thread; the render thread picks it up here.
struct HudEngine : Engine {
    // Host-process services behind the dash. Android supplies ShellBridge;
    // the desktop preview supplies a fixture backend. The dash logic below
    // never sees which one is attached.
    DashBackend* backend = nullptr;

    std::vector<Panel> panels;

    // ringPos: the world point the whole panel ring hangs around. Recenter
    // and summon re-anchor it to the head's position so the dash opens in
    // front of where the user is; between anchors the ring is world-locked.
    // eyePos: the head's live position each frame, used as the gaze-ray
    // origin - world origin when position tracking is out.
    float ringPos[3] = {0.0f, 0.0f, 0.0f};
    float eyePos[3] = {0.0f, 0.0f, 0.0f};

    // summon-key hold state: holdStartMs is the CLOCK_MONOTONIC ms the key
    // went down (0 = not held); holdP is the 0..1 fill the ring draws
    long long holdStartMs = 0;
    float holdP = 0.0f;

    // frame clock for the animation state: frameMs is this frame's
    // CLOCK_MONOTONIC stamp, animMs the previous tick's - their delta
    // drives every transition; summonMs stamps the last window bring-up
    // and feeds the strip's slide/fade-in
    long long frameMs = 0;
    long long animMs = 0;
    long long summonMs = 0;

    // quiet-frame throttle: lastBusyMs stamps the last frame that saw
    // anything animate or come in; while covered and quiet the draw runs
    // at a low cadence instead of every vsync. prevAim tracks controller
    // pose deltas; quietDiv counts draws between idle presents
    long long lastBusyMs = 0;
    float prevAimO[3] = {0.0f, 0.0f, 0.0f};
    float prevAimD[3] = {0.0f, 0.0f, -1.0f};
    int quietDiv = 0;

    int hover = -1;              // panel index under the gaze ray
    int hoverZone = ZONE_NONE;   // chrome zone under the gaze ray
    float hitX = 0, hitY = 0;    // display px coords of the hit
    float hitU = 0, hitV = 0;    // raw panel coords of the hit

    // the app-grid overlay: opened from the dock's grid button, closes on
    // BACK, its close disc, a re-tap of the button or a launched app
    GridOverlay grid;

    // the quick-settings overlay: opened from the dock's status pill,
    // closes on BACK, its close disc, a re-tap or the gear launch. Only
    // one card overlay is up at a time - opening this drops the grid
    QuickPanel quick;

    // the settings card: the full settings surface inside the dash, from
    // the quick panel's gear or About tile. Same overlay-exclusion rules
    // as the other cards
    SetOverlay set;

    // dock: rebuilt each frame by syncDock from the pin list, the live
    // panels and the immersive tasks the java poller sees. dockYaw anchors
    // the strip to the dash's centre yaw; dockPitch is its own elevation
    // band under the windows
    std::vector<DockItem> dock;
    float dockHW = 0.0f;
    std::vector<std::string> dockPins;
    std::vector<XrTask> dockXr;
    int dockXrVer = -1;
    std::map<std::string, DockIcon> dockIcons;
    float dockYaw = 0.0f, dockPitch = kDockPitchRest;
    float dockGrabYaw = 0.0f;      // dockYaw snapshot when a ring drag grabs
    float dockGrabPitch = 0.0f;    // dockPitch snapshot on the same grab
    float ringGrabPitch = 0.0f;    // the ring's elevation at the grab
    bool dockAnchored = false;
    int dockHover = -1;            // item under the gaze ray
    int dockZone = DZONE_NONE;
    float dockU = 0.0f, dockV = 0.0f;   // bar coords of the hit
    int dockPress = -1;            // item a confirm press started on
    int dockPressZone = DZONE_NONE;
    std::string dockPressPkg;      // guards against a rebuild mid-press
    long long dockPressMs = 0;
    bool dockPinDone = false;      // long-press already toggled the pin
    float dockPinP = 0.0f;         // pin hold fill 0..1
    bool sysPress = false;         // confirm armed on the status pill

    // minimized-window shelf: hidden panels parked on a pill above the
    // dock bar, rebuilt each frame by syncDock. hover/press mirror the
    // dock's gesture state; shelfPressDisp guards against the panel list
    // shifting under a held press
    std::vector<ShelfItem> shelf;
    float shelfHW = 0.0f;
    int shelfHover = -1;
    int shelfPress = -1;
    int shelfPressDisp = -1;

    // status cluster on the strip's left end: dockSys carries the layout
    // positions, the rest is the last bridge pull + the formatted clock
    DockStatus dockSys;
    char sysClock[8] = "--:--";
    int sysNet = 0;                // uplink kind: a DockNet value
    int sysBatt = -1;              // battery percent 0..100, -1: no battery
    int sysChg = 0;                // charging flag

    // notification cards: notifsAll is the live record set rebuilt when
    // the listener's version bumps; notifs is the visible subset filtered
    // each frame by postMs age so cards age out of the dash on their own.
    // hover/press mirror the dock's gesture state. toastOnly means the
    // window is up over a covered app for a heads-up: only the stack
    // draws, anchored on the gaze yaw captured when the toast popped
    std::vector<NotifItem> notifsAll;
    std::vector<NotifItem> notifs;
    int notifVer = -1;
    int notifHover = -1;
    int notifZone = NZONE_NONE;
    int notifPress = -1;
    int notifPressZone = NZONE_NONE;
    std::string notifPressKey;   // guards against a rebuild mid-press
    bool toastOnly = false;
    bool toastWas = false;
    float toastYaw = 0.0f;

    // system-message cards: crash/ANR entries from the dropbox watcher,
    // rebuilt when its version bumps; only the front card draws and picks.
    // sysMsgOnly means the window is up over a covered app just for the
    // dialog - anchored on sysMsgYaw, the gaze yaw when it popped
    std::vector<SysMsgItem> sysMsgs;
    int sysMsgVer = -1;
    int sysMsgHover = -1;      // 0 while the ray is on the front card
    int sysMsgZone = MZONE_NONE;
    int sysMsgBtn = -1;
    int sysMsgPress = -1;
    int sysMsgPressZone = MZONE_NONE;
    int sysMsgPressBtn = -1;
    long long sysMsgPressId = 0;   // guards against a rebuild mid-press
    bool sysMsgOnly = false;
    bool sysMsgWas = false;
    float sysMsgYaw = 0.0f;

    // localized chrome labels pulled from java on a version bump, same
    // pattern as the notif/sysmsg feeds; uiLib is the app-grid button's
    // label - empty until the first pull lands, draw sites fall back to
    // the English text
    int uiStrVer = -1;
    std::string uiLib;
    // the quick panel's labels: title, on/off/not-connected, then one per
    // tile in QsId order - the fallback table in quick.cpp covers a pull
    // that hasn't landed yet
    std::string uiQs[UIQS_COUNT];
    // the settings card's labels: UiSetStr slots
    std::string uiSet[UISET_COUNT];

    // developer-settings debug line: debugHud is the toggle itself;
    // debugOnly means the window is up over a covered app solely for that
    // line, so the scene draws nothing but the hold ring
    bool debugHud = false;
    bool debugOnly = false;

    // controllers: the shared-memory map plus the arbitration state it
    // feeds. ctrlPos/ctrlDir are each live controller's world aim ray;
    // aimO/aimD is whichever pointer owns the dash this frame - the active
    // controller, or the gaze ray while hmdInput() is true
    ctrl_share ctrlMem;
    bool ctrlOpen = false;
    bool ctrlLogged = false;
    long long ctrlRetryMs = 0;
    InputState input;
    ctrl_live ctrlLive[2];       // per-block write-activity trackers
    ctrl_probe ctrlProbe;        // channel flag-edge probe (async)
    long long ctrlProbeNs = 0;   // next probe slot
    long long ctrlQuietMs = 0;   // channel silent since; 0 = writing
    long long ctrlPokeMs = 0;    // last service start-thread poke
    ctrl_state ctrl[2];
    float ctrlPos[2][3] = {{0}};
    float ctrlDir[2][3] = {{0}};
    Mat4 ctrlMat[2];             // per-controller model rotation
    float aimO[3] = {0, 0, 0};
    float aimD[3] = {0, 0, -1};
    float aimYaw = 0.0f, aimPitch = 0.0f;
    float aimHitT = -1.0f;       // ray distance to the picked surface
    std::deque<InputEvent> ctrlEv;
    GLuint ctrlVbo[2] = {0, 0};  // loaded meshes, one per hand
    int ctrlVerts[2] = {0, 0};
    GLuint ctrlBeamVbo = 0;      // rewritten every frame the beam shows

    bool confirmHeld = false;
    bool moveHeld = false;       // confirm held on a drag handle
    float moveGrabYaw = 0.0f;    // gaze yaw when the ring drag grabbed
    float moveGrabPitch = 0.0f;  // gaze pitch when the ring drag grabbed
    int dragDisp = -1;           // display a confirm-drag started on
    float dragX = 0, dragY = 0;  // last injected drag position, px
    float grabX = 0, grabY = 0;  // where the drag grabbed, px
    int pressDisp = -1;          // display the held confirm press started on
    int pressZone = ZONE_NONE;   // chrome zone that press started on

    // floating keyboard: the IME draws into a texture we own and reports
    // its virtual display id over broadcast; see src/kbd and the hud's
    // ACTION_KBD contract
    Kbd kbd;

#ifdef __ANDROID__
    // surface handoff: the render thread owns the window end of the
    // SurfaceView. window is a newly-posted ANativeWindow, windowGone a
    // request to tear the EGL window surface down
    std::atomic<ANativeWindow*> window{nullptr};
    std::atomic<bool> windowGone{false};
#endif
    std::atomic<bool> running{true};

    std::mutex keyMu;
    std::deque<KeyIn> keyQ;
};
