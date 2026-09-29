#pragma once

#include "../panels/panel.h"

#include <vector>

struct HudEngine;

// where on the quad a gaze hit landed
enum KZone {
    KZONE_KEY = 0,    // a key, taps inject into the IME's display
    KZONE_HANDLE,     // the drag pill under the quad: moves the keyboard
};

// The floating keyboard quad: the IME app draws its keys into a texture
// this process owns (a Surface handed over broadcast), rendered under the
// panel that owns the text field. Resource handles are void* like Panel's
// so the header stays host-buildable.
struct Kbd {
    int displayId = -1;      // the IME's own virtual display, for touches
    int hostDisp = -1;       // panel the quad hangs under
    unsigned tex = 0;
    void* st = nullptr;      // SurfaceTexture global ref (jobject)
    void* stArr = nullptr;   // jfloatArray global ref, 16 floats
    float stMat[16] = {};
    bool shown = false;      // IME says the quad is live
    bool only = false;       // window is up over a covered app just for it
    float yaw = 0.0f;        // gaze yaw captured when "only" pops up
    bool was = false;        // last frame's only, for the capture edge
    bool hover = false;      // aim ray is on the quad or its pill
    int zone = KZONE_KEY;    // which part the aim is on
    float u = 0, v = 0;      // quad coords of the hit, -1..1 (pill is past 1)
    bool pressed = false;    // a confirm press started on a key
    float px = 0, py = 0;    // display px of that press
    // the quad's own placement, set by dragging its pill: a yaw swing around
    // the viewer and a slide along its up axis, both on top of wherever the
    // host anchor puts it - so a dash ring-drag carries the keyboard along
    // with the windows and the pill repositions it alone
    float offYaw = 0.0f, offY = 0.0f;
    bool moveHeld = false;       // confirm is down on the pill
    float grabAimYaw = 0.0f, grabAimPitch = 0.0f;
    float grabOffYaw = 0.0f, grabOffY = 0.0f;
};

// index of the panel the keyboard hangs under: the window that last took
// a tap (hostDisp), else the first visible window, else -1
int kbdHostIndex(const std::vector<Panel>& panels, int hostDisp);

// world frame for the keyboard quad under panel p: same facing basis as the
// window, centre dropped below the window's bottom edge and pulled toward
// the viewer to kKbdDist, then swung/raised by the user's pill offsets
void kbdCenter(const Panel& p, const float origin[3], float offYaw,
               float offY, float c[3], float r[3], float up[3]);

// no window to host the quad (the field belongs to a covered app on the
// physical display): hang it where the dash's centre already is, below
// where a panel would sit
void kbdFreeCenter(float yaw, const float origin[3], float offYaw,
                   float offY, float c[3], float r[3], float up[3]);

// the quad's world frame for this frame: under the host panel when one
// exists, else free at freeYaw. `free` forces the free path even with
// panels around (kbdOnly draws no panels, so there is nothing to hang
// under). Pick and draw both call this so they can never disagree on
// where the keys are
void kbdFrame(const std::vector<Panel>& panels, int hostDisp, bool free,
              float freeYaw, float offYaw, float offY,
              const float origin[3], float c[3], float r[3], float up[3]);

// how far under the quad's centre the drag pill floats (world units)
float kbdHandleDrop();

// is a quad-space hit on the drag pill? u/v come from rayQuad with the
// quad's half extents - the pill lives below v=-1 so this checks before
// the in-bounds test, same shape as the dash's handle
bool onKbdHandle(float u, float v);

// quad-space hit coords -> display px for injection
void kbdHitPx(float u, float v, float* x, float* y);

// GL/JNI glue: create the shared surface + texture once, then sync the
// IME's shown/display state and pull the newest frame. Render thread only.
void kbdTick(HudEngine* e);
