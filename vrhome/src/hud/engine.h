#pragma once

#include "../engine.h"
#include "../panels/panel.h"

#include <android/native_window.h>

#include <atomic>
#include <deque>
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
              mLaunchLauncher = nullptr, mAdopt = nullptr, mReleasePanel = nullptr,
              mTakeAdopt = nullptr, mTakeRelease = nullptr, mInjectTap = nullptr,
              mInjectTouch = nullptr,
              mRemoveTask = nullptr, mFocusTask = nullptr, mAppLabel = nullptr,
              mIsVr = nullptr, mLaunchVr = nullptr, mIsCovered = nullptr;
    jmethodID stUpdate = nullptr, stMatrix = nullptr;
    jclass pendingCls = nullptr;
    jfieldID fPendTask = nullptr, fPendPkg = nullptr;
    bool bridgeDead = false;

    std::vector<Panel> panels;

    int hover = -1;              // panel index under the gaze ray
    int hoverZone = ZONE_NONE;   // chrome zone under the gaze ray
    float hitX = 0, hitY = 0;    // display px coords of the hit
    bool launcherSpawned = false;
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
