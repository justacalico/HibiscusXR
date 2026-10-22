#pragma once

#include "../input/input_state.h"
#include "../math/mat4.h"

// Pure math and arbitration behind the -vr overlay path: pose transforms,
// the per-eye projection the runtime's fov reports, tracking fallbacks and
// the controller sample -> dash event diff. No OpenXR/GL includes so the
// host unit tests exercise every decision; src/desktop/vr.cpp is glue only.

// a tracked pose in OpenXR stage space: meters, Y up, -Z forward
struct VrPose {
    float q[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float p[3] = {0.0f, 0.0f, 0.0f};
    bool ok = false;
};

// view matrix for a tracked pose (its inverse rigid transform)
Mat4 vrView(const VrPose& p);

// world-space ray out of a pose: origin is the position, direction the
// rotated -Z axis (the XR aim convention)
void vrRay(const VrPose& p, float o[3], float d[3]);

// projection matrix from the runtime's per-eye fov, given as the four
// tangent half-angles OpenXR reports (radians). Same clip-space layout
// as perspective(): -Z forward, GL depth range
Mat4 vrFovProj(float angL, float angR, float angD, float angU,
               float zn, float zf);

// eye poses when xrLocateViews has no tracking yet: straddle the head
// pose along its own right axis so the stereo pair stays coherent
void vrEyesFallback(const VrPose& head, float ipd, VrPose eyes[2]);

// one frame of controller state per hand (CTRL_LEFT/CTRL_RIGHT index)
struct VrInput {
    bool aim[2] = {false, false};    // aim pose located this frame
    bool sel[2] = {false, false};    // trigger/select -> confirm click
    bool sq[2] = {false, false};     // squeeze -> secondary click
    bool aux[2] = {false, false};    // aux button -> back/park
    bool menu[2] = {false, false};   // menu button -> recenter
    float stick[2][2] = {{0, 0}, {0, 0}};
};

// the semantic diff of two samples: which hand owns the pointer, the key
// edges to push on the hud's ctrlEv queue and the scroll delta for this
// frame. keys[] pairs each button edge with its code and down/up action,
// matching what ctrl.cpp feeds off the controller wire
struct VrEvents {
    InputEvent keys[8];
    int nKeys = 0;
    float scroll = 0.0f;
    int active = -1;                     // CTRL_LEFT/RIGHT or -1
    bool conn[2] = {false, false};       // per-hand aim validity
};

VrEvents vrInputTick(const VrInput& prev, const VrInput& cur,
                     int lastActive);
