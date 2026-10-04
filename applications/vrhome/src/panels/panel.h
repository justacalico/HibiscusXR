#pragma once

#include <string>

// where on a panel's chrome a gaze hit lands
enum Zone {
    ZONE_NONE   = -1,
    ZONE_WINDOW = 0,   // the app surface
    ZONE_LABEL,        // the under-window pill on a docked window, off the
                       // buttons: drags it between the ring's three slots,
                       // a still release focuses the task
    ZONE_MIN,          // minimize button
    ZONE_CLOSE,        // close button
    ZONE_FLOAT,        // float button: unpin the window off the slot grid
    ZONE_RESIZE,       // corner grip on the window's bottom-right
    ZONE_PILL,         // pill body on a floating window: its move handle
};

// One floating window: a GL texture fed by a virtual display plus the task
// metadata the shell tracks. Resource handles are stored as void* so this
// header stays free of JNI/GL types and the layout logic can be unit tested
// on the host. panels.cpp casts them back to jobject/GLuint on device.
struct Panel {
    int displayId = -1;
    int taskId = -1;
    unsigned tex = 0;
    void* st = nullptr;       // SurfaceTexture global ref (jobject)
    void* stArr = nullptr;    // jfloatArray global ref, 16 floats
    float stMat[16] = {};
    float yaw = 0;            // world yaw of panel centre
    float pitch = 0;          // elevation on the ring; plane tilts to face you
    float scale = 1.0f;       // quad size multiplier; the display keeps its
                              // own px size, the window just scales on screen
    bool floating = false;    // off the slot grid: own yaw/pitch + move pill
    std::string pkg;
    std::string label;        // resolved app label for the window bar
    bool minimized = false;   // hidden window; task and display stay alive
    long long bornMs = 0;     // CLOCK_MONOTONIC at spawn; the scale/fade-in
                              // runs off it. 0 = predates anim, skip intro
    float minT = 0.0f;        // park flight 0..1: chases `minimized`, so a
                              // restore is just the flag dropping early.
                              // 1 = fully shrunk onto the shelf slot
    float grabYaw = 0;        // yaw snapped when a drag grabbed
    float grabPitch = 0;      // pitch snapped when a drag grabbed
    float grabScale = 0.0f;   // scale snapped when a resize drag grabbed
    float grabR = 0.0f;       // hit's distance from centre at grab, metres
};
