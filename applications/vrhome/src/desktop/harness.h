#pragma once

#include "../math/mat4.h"
#include "../mcp/server.h"
#include "../mcp/tools.h"

#include <functional>
#include <string>
#include <thread>

struct HudEngine;
struct GLFWwindow;

// free-camera state the GLFW window drives: mouse-look plus WASD/QE fly
struct DashCam {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    double lastX = 0.0, lastY = 0.0;
    bool first = true;
};

void camFwd(const DashCam& c, float out[3]);
Mat4 camView(const DashCam& c);
void drawBackdrop(HudEngine* e, const Mat4& vp);
long long dashNowMs();

// swap interval is off so swaps never block (see dashWindow); the loops
// pace draws instead, true at most once per `ms`
inline bool dashDrawDue(long long now, long long* last, int ms) {
    if (now - *last < ms) return false;
    *last = now;
    return true;
}

// one running desktop dash window: GLFW state plus the hooks each backend
// needs to differ on. extraKey gets keys the core table doesn't own (K in
// the preview); kbdSet drives MCP's setKbd for backends with a real quad.
// While an app holds the input grab the grab* hooks own every event -
// they return true when the dash should stay out of it, and the backend
// wires its own toggle key inside grabKey
struct DashApp {
    GLFWwindow* win = nullptr;
    HudEngine* hud = nullptr;
    DashCam cam;
    std::function<void(int key, int scancode, int action)> extraKey;
    std::function<void(bool)> kbdSet;
    std::function<bool(int key, int scancode, int action, int mods)> grabKey;
    std::function<bool(double dx, double dy)> grabMotion;
    std::function<bool(int button, int action)> grabBtn;
    std::function<bool(double yoff)> grabScroll;
    std::function<bool()> grabbed;
    std::thread mcpTh;
};

GLFWwindow* dashWindow(DashApp* app, const char* title, int w, int h);
void dashCamTick(DashApp* app, float dt);
void dashStartMcp(DashApp* app);
void dashStop(DashApp* app);

// MCP jobs queue onto the render thread: mutations run before the tick,
// reads (state, screenshot) after the draw while the back buffer is valid
void dashDrainPre();
void dashDrainPost();
