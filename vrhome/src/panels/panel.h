#pragma once

#include <string>

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
    std::string pkg;
    std::string label;        // resolved app label for the window bar
};
