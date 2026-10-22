#pragma once

#include "vrmath.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define XR_USE_PLATFORM_EGL
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

struct HudEngine;

// the -vr overlay session: an XR_EXTX_overlay session that renders the
// dash per-eye into swapchain images and submits a projection layer, so
// panels live in real 3D stage space instead of a flat window. Composes
// over whatever the runtime's main session shows (WiVRn/Monado; SteamVR
// titles included when they run on WiVRn). All decisions live in vrmath -
// this file is EGL/OpenXR glue only.
struct VrDash {
    EGLDisplay edpy = EGL_NO_DISPLAY;
    EGLConfig  ecfg = nullptr;
    EGLContext ectx = EGL_NO_CONTEXT;
    EGLSurface esurf = EGL_NO_SURFACE;
    bool pbuffer = false;

    XrInstance inst = XR_NULL_HANDLE;
    XrSystemId sys = XR_NULL_SYSTEM_ID;
    XrSession sess = XR_NULL_HANDLE;
    XrSpace stage = XR_NULL_HANDLE;
    XrSpace view = XR_NULL_HANDLE;
    XrEnvironmentBlendMode blend = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    bool running = false;
    bool quit = false;

    XrFrameState fs{XR_TYPE_FRAME_STATE};
    XrTime predTime = 0;
    VrPose head;

    // per-eye state: poses+fovs for this frame's render and submission.
    // While xrLocateViews has no tracking, eyes[] holds the fallback pair
    // straddling the head. swap is one stereo texture-array swapchain;
    // fbo[eye][i] binds layer eye of image i
    XrViewConfigurationView cfgv[2] = {};
    VrPose eyes[2];
    XrFovf fovs[2] = {};
    int texW = 0, texH = 0;
    XrSwapchain swap = XR_NULL_HANDLE;
    GLuint fbo[2][8] = {{0}};
    GLuint depth[2][8] = {{0}};
    uint32_t nimg = 0;
    int img = -1;
    bool drawn = false;
    bool presented = false;

    // actions: one set, per-hand subpaths
    XrActionSet aset = XR_NULL_HANDLE;
    XrAction aAim = XR_NULL_HANDLE, aSel = XR_NULL_HANDLE;
    XrAction aSqueeze = XR_NULL_HANDLE, aAux = XR_NULL_HANDLE;
    XrAction aMenu = XR_NULL_HANDLE, aStick = XR_NULL_HANDLE;
    XrSpace aimSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrPath handL = XR_NULL_PATH, handR = XR_NULL_PATH;
    VrInput prev;
};

constexpr int kVrEyes = 2;
constexpr float kVrIpd = 0.064f;
constexpr float kVrNear = 0.05f, kVrFar = 100.0f;
// fallback fov for frames where the runtime reports no view state: a
// symmetric 90deg cone keeps the projection sane until tracking lands
constexpr float kVrFovFallback = 0.7853981634f;   // 45deg in radians

// full bring-up: EGL context, instance, overlay session, spaces,
// per-eye swapchains and action set. false = fatal, error already logged
bool vrInit(VrDash* v, const char* appName);
void vrShutdown(VrDash* v);

// pump runtime events. Return: 1 session running, 0 not yet/stopped
// (caller should sleep briefly), -1 runtime asked us to exit
int vrPoll(VrDash* v);

// xrWaitFrame + xrBeginFrame, locate the head and both views at the
// predicted time. Returns fs.shouldRender; vrEnd must follow either way
bool vrBeginFrame(VrDash* v);

// sync actions, update hud->input/ctrlPos/ctrlDir/eyePos/aimO/aimD and
// push key edges onto the hud's ctrlEv queue. Call after vrBeginFrame
void vrInput(VrDash* v, HudEngine* e);

// acquire one eye's swapchain image, bind its framebuffer and write the
// eye's view-projection matrix. false = no free image, skip that eye's
// draw. Both eyes still release in vrEnd
bool vrTarget(VrDash* v, int eye, Mat4* vp);

// release acquired images and end the frame, submitting a projection
// layer for the located views. Released images keep presenting through
// !shouldRender gaps so the dash doesn't blink
void vrEnd(VrDash* v);
