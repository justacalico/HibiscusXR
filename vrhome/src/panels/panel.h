#pragma once

#include <string>

// where on a panel's chrome a gaze hit lands
enum Zone {
    ZONE_NONE   = -1,
    ZONE_WINDOW = 0,   // the app surface
    ZONE_LABEL,        // the top bar, off both buttons
    ZONE_MIN,          // minimize button
    ZONE_CLOSE,        // close button
    ZONE_HANDLE,       // the drag line under the window: moves the whole ring
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
    std::string pkg;
    std::string label;        // resolved app label for the window bar
    bool minimized = false;   // hidden window; task and display stay alive
    float grabYaw = 0;        // yaw snapped when a ring drag grabbed
    float grabPitch = 0;      // pitch snapped when a ring drag grabbed
};
