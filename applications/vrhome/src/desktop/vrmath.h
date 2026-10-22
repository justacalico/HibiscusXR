#pragma once

#include "../input/input_state.h"
#include "../math/mat4.h"

// Pure math and arbitration behind the -vr overlay path: pose transforms,
// the presentation quad's placement and hit test, and the per-frame
// controller sample -> dash event diff. No OpenXR/GL includes so the host
// unit tests exercise every decision; src/desktop/vr.cpp is glue only.

// a tracked pose in OpenXR stage space: meters, Y up, -Z forward
struct VrPose {
    float q[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float p[3] = {0.0f, 0.0f, 0.0f};
    bool ok = false;
};

// the quad the dash texture is presented on: world-locked, yaw-only, so
// panels stay upright when the user pitches
struct VrQuad {
    float p[3] = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float w = 0.0f, h = 0.0f;   // meters
};

// put the quad in front of the head: horizontal forward at dist, centered
// a touch below eye height so the frame isn't in the face. The quad's own
// -Z faces the user
void vrQuadPlace(const VrPose& head, float dist, float w, float h,
                 VrQuad* out);

// view matrix for a tracked pose (its inverse rigid transform)
Mat4 vrView(const VrPose& p);

// world-space ray out of a pose: origin is the position, direction the
// rotated -Z axis (the XR aim convention)
void vrRay(const VrPose& p, float o[3], float d[3]);

// intersect a ray with the quad's front face. Fills u/v (0..1, u left to
// right, v top to bottom) and the ray distance t. false: miss or behind
bool vrQuadHit(const VrQuad& qd, const float o[3], const float d[3],
               float* u, float* v, float* t);

// the aim ray inside the dash scene for a uv on the presented quad: the
// quad shows what the fixed dash camera renders, so (u,v) unprojects
// through a camera at the origin facing -Z with the given vertical fov
// and aspect. The dash scene camera never moves in vr mode
void vrUvRay(float u, float v, float fovYDeg, float aspect,
             float o[3], float d[3]);

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
