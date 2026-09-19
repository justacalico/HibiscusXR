#pragma once

#include "../engine.h"
#include "../panels/panel.h"
#include "../dock/item.h"
#include "../common/config.h"

#include <android/native_window.h>

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
    // ShellBridge java object + cached method ids
    jobject bridge = nullptr;
    jmethodID mCreatePanel = nullptr, mPanelTex = nullptr, mLaunchPkg = nullptr,
              mAdopt = nullptr, mReleasePanel = nullptr,
              mTakeAdopt = nullptr, mTakeRelease = nullptr, mInjectTap = nullptr,
              mInjectTouch = nullptr,
              mRemoveTask = nullptr, mFocusTask = nullptr, mAppLabel = nullptr,
              mIsVr = nullptr, mLaunchVr = nullptr, mIsCovered = nullptr,
              mTakePins = nullptr, mSetPins = nullptr, mAppIcon = nullptr,
              mVrVer = nullptr, mRunningVr = nullptr, mDismiss = nullptr;
    jmethodID stUpdate = nullptr, stMatrix = nullptr;
    jclass pendingCls = nullptr;
    jfieldID fPendTask = nullptr, fPendPkg = nullptr;
    bool bridgeDead = false;

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

    int hover = -1;              // panel index under the gaze ray
    int hoverZone = ZONE_NONE;   // chrome zone under the gaze ray
    float hitX = 0, hitY = 0;    // display px coords of the hit
    bool launcherSpawned = false;

    // dock: rebuilt each frame by syncDock from the pin list, the live
    // panels and the immersive tasks the java poller sees. dockYaw anchors
    // the strip to the dash's centre yaw; dockPitch is its own elevation
    // band under the windows
    std::vector<DockItem> dock;
    float dockHW = 0.0f;
    std::vector<std::string> dockPins;
    bool dockPinsLoaded = false;
    std::vector<XrTask> dockXr;
    int dockXrVer = -1;
    std::map<std::string, DockIcon> dockIcons;
    float dockYaw = 0.0f, dockPitch = kDockPitchRest;
    float dockGrabYaw = 0.0f;      // dockYaw snapshot when a ring drag grabs
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

    bool confirmHeld = false;
    bool moveHeld = false;       // confirm held on a drag handle
    float moveGrabYaw = 0.0f;    // gaze yaw when the ring drag grabbed
    float moveGrabPitch = 0.0f;  // gaze pitch when the ring drag grabbed
    int dragDisp = -1;           // display a confirm-drag started on
    float dragX = 0, dragY = 0;  // last injected drag position, px
    float grabX = 0, grabY = 0;  // where the drag grabbed, px
    int pressDisp = -1;          // display the held confirm press started on
    int pressZone = ZONE_NONE;   // chrome zone that press started on

    // surface handoff: the render thread owns the window end of the
    // SurfaceView. window is a newly-posted ANativeWindow, windowGone a
    // request to tear the EGL window surface down
    std::atomic<ANativeWindow*> window{nullptr};
    std::atomic<bool> windowGone{false};
    std::atomic<bool> running{true};

    std::mutex keyMu;
    std::deque<KeyIn> keyQ;
};
