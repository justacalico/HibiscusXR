#pragma once

#include "vrmath.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define XR_USE_PLATFORM_EGL
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

struct HudEngine;

// the -vr overlay session: a WayVR-style OpenXR overlay. The dash renders
// to one swapchain texture submitted as a world-locked quad layer under
// XR_EXTX_overlay, so it composes over whatever the runtime's main session
// shows (WiVRn/Monado; SteamVR titles included when they run on WiVRn).
// All decisions live in vrmath - this is EGL/OpenXR glue only.
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

    // the presented quad and its swapchain (the dash's render target)
    VrQuad quad;
    bool placed = false;
    XrSwapchain swap = XR_NULL_HANDLE;
    GLuint fbo[8] = {0};
    GLuint depth[8] = {0};
    uint32_t nimg = 0;
    int img = -1;
    bool drawn = false;
    int texW = 0, texH = 0;

    // actions: one set, per-hand subpaths
    XrActionSet aset = XR_NULL_HANDLE;
    XrAction aAim = XR_NULL_HANDLE, aSel = XR_NULL_HANDLE;
    XrAction aSqueeze = XR_NULL_HANDLE, aAux = XR_NULL_HANDLE;
    XrAction aMenu = XR_NULL_HANDLE, aStick = XR_NULL_HANDLE;
    XrSpace aimSpace[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrPath handL = XR_NULL_PATH, handR = XR_NULL_PATH;
    VrInput prev;
};

// render size the quad layer presents; 16:9 like the desktop window
constexpr int kVrTexW = 1920, kVrTexH = 1080;
constexpr float kVrQuadDist = 1.4f;
constexpr float kVrQuadW = 1.6f, kVrQuadH = 0.9f;
constexpr float kVrFovY = 50.0f;   // dash camera fov inside the texture

// full bring-up: EGL context, instance, overlay session, spaces,
// swapchain and action set. false = fatal, error already logged
bool vrInit(VrDash* v, const char* appName);
void vrShutdown(VrDash* v);

// pump runtime events. Return: 1 session running, 0 not yet/stopped
// (caller should sleep briefly), -1 runtime asked us to exit
int vrPoll(VrDash* v);

// xrWaitFrame + xrBeginFrame and locate the head/view at the predicted
// time. Returns fs.shouldRender; vrEnd must follow either way
bool vrBeginFrame(VrDash* v);

// sync actions, update hud->input/ctrlPos/ctrlDir/eyePos/aimO/aimD and
// push key edges onto the hud's ctrlEv queue. Call after vrBeginFrame
void vrInput(VrDash* v, HudEngine* e);

// place the quad in front of the current head pose (first frame and
// whenever the dash recenters)
void vrRecenter(VrDash* v);

// acquire the swapchain image and bind its framebuffer; returns false if
// the runtime has no free image this frame (skip the draw, still vrEnd)
bool vrTarget(VrDash* v);

// release the image and end the frame. Once a frame has been released the
// quad layer resubmits itself automatically, so stale frames stay visible
// through !shouldRender gaps
void vrEnd(VrDash* v);
