#define VRHOME_DESKTOP 1

#include "cmdline.h"
#include "dmabuf.h"
#include "harness.h"
#include "vr.h"
#include "wlbackend.h"
#include "wlcomp.h"

#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../render/icons.h"
#include "../render/resources.h"

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>
#include <wayland-server.h>
#include <linux/input-event-codes.h>

#include <cstring>
#include <poll.h>

int main(int argc, char** argv) {
    const bool headless = argFlag(argc, argv, "-headless");
    const bool mcp = headless || argFlag(argc, argv, "-mcp");
    const bool vr = argFlag(argc, argv, "-vr");

    WlComp* comp = WlComp::create();
    if (!comp) {
        LOGE("failed to create wayland display");
        return 1;
    }
    LOGI("clients connect with WAYLAND_DISPLAY=%s", comp->sock.c_str());

    DashApp app;
    VrDash xr;
    if (vr) {
        // the overlay session's EGL context must be current before
        // dmabufInit probes the host EGL
        if (!vrInit(&xr, "vrdash")) return 1;
    } else if (headless) {
        if (!dashHeadless(&app, 1440, 810)) return 1;
    } else if (!dashWindow(&app, "Hibiscus dash (wayland)", 1440, 810))
        return 1;
    // needs a current EGL context, so it goes after the window/headless
    // init: clients get zwp_linux_dmabuf_v1 only when the host EGL can
    // import dmabufs
    dmabufInit(comp);

    HudEngine hud;
    WlBackend backend(comp);
    app.hud = &hud;
    // GLFW scancodes are evdev codes under both its Wayland and X11
    // paths, which is exactly what wl_keyboard.key wants. Keys the dash
    // doesn't own go to the focused app.
    app.extraKey = [&backend](int key, int scancode, int action) {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT ||
            key == GLFW_KEY_W || key == GLFW_KEY_S ||
            key == GLFW_KEY_A || key == GLFW_KEY_D ||
            key == GLFW_KEY_Q || key == GLFW_KEY_E)
            return;   // camera fly keys stay ours
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
            backend.sendKey((uint32_t)scancode,
                            action == GLFW_PRESS);
    };
    // ` toggles the input grab on the window under the gaze (the focused
    // one as a fallback): while it holds, every key and the mouse go
    // straight to the app, which is what games want. shift+` always
    // releases.
    app.grabbed = [comp] { return comp->grabTask != nullptr; };
    app.grabKey = [comp, &backend, &hud](int key, int scancode, int action,
                                         int mods) {
        if (key == GLFW_KEY_GRAVE_ACCENT) {
            if (action != GLFW_PRESS) return true;
            if (grabAction((mods & GLFW_MOD_SHIFT) != 0) == kGrabOff ||
                    comp->grabTask) {
                comp->setGrab(nullptr);
                return true;
            }
            if (hud.hover >= 0 && hud.hover < (int)hud.panels.size() &&
                    hud.panels[hud.hover].taskId >= 0)
                backend.focusTask(hud.panels[hud.hover].taskId);
            comp->setGrab(comp->active);
            if (comp->grabTask) hudKey(&hud, kKeyEnter, kKeyUp, 0);
            return true;
        }
        if (!comp->grabTask) return false;
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            backend.sendKey((uint32_t)scancode, true);
        else if (action == GLFW_RELEASE)
            backend.sendKey((uint32_t)scancode, false);
        return true;
    };
    app.grabMotion = [comp](double dx, double dy) {
        if (!comp->grabTask) return false;
        comp->grabMotion((float)dx, (float)dy);
        return true;
    };
    app.grabBtn = [comp](int button, int action) {
        if (!comp->grabTask) return false;
        uint32_t ev = 0;
        if (button == GLFW_MOUSE_BUTTON_LEFT) ev = BTN_LEFT;
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) ev = BTN_RIGHT;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) ev = BTN_MIDDLE;
        else if (button == GLFW_MOUSE_BUTTON_4) ev = BTN_SIDE;
        else if (button == GLFW_MOUSE_BUTTON_5) ev = BTN_EXTRA;
        if (ev && action != GLFW_REPEAT)
            comp->grabButton(ev, action == GLFW_PRESS);
        return true;
    };
    app.grabScroll = [comp](double yoff) {
        if (!comp->grabTask) return false;
        comp->grabAxis((float)yoff);
        return true;
    };
    hud.backend = &backend;
    hud.haveQuat = true;
    hud.dockAnchored = false;
    hud.summonMs = dashNowMs();
    if (initGlResources(&hud) != 0) return 1;
    loadGlyphs(&hud);

    const int wfd = wl_event_loop_get_fd(
        wl_display_get_event_loop(comp->dpy));

    if (mcp) dashStartMcp(&app);

    // -vr: the same compositor and dash, presented as an OpenXR overlay
    // projection layer in stage space. XR pacing replaces the window's
    // draw throttle; the wayland fd still gets serviced every frame
    if (vr) {
        while (!app.quit) {
            const int st = vrPoll(&xr);
            if (st < 0) break;
            pollfd pfd{wfd, POLLIN, 0};
            poll(&pfd, 1, st ? 0 : 50);
            comp->dispatch(0);
            comp->flush();
            dashDrainPre();
            if (!st) continue;
            const bool draw = vrBeginFrame(&xr);
            vrInput(&xr, &hud);
            const long long now = dashNowMs();
            if (takeWantRecenter()) wantRecenter();
            tickDash(&hud, vrView(xr.head), now);
            if (draw) {
                for (int eye = 0; eye < kVrEyes; ++eye) {
                    Mat4 vp;
                    if (!vrTarget(&xr, eye, &vp)) continue;
                    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    drawDashScene(&hud, vp);
                }
            }
            dashDrainPost();
            vrEnd(&xr);
        }
        vrShutdown(&xr);
        dashStop(&app);
        comp->destroy();
        return 0;
    }

    long long last = dashNowMs();
    long long lastDraw = 0;
    while (dashAlive(&app)) {
        pollfd pfd{wfd, POLLIN, 0};
        if (comp->tasks.empty())
            poll(&pfd, 1, 8);
        else
            poll(&pfd, 1, 4);
        comp->dispatch(0);
        comp->flush();
        if (!app.headless) glfwPollEvents();
        dashDrainPre();
        const long long now = dashNowMs();
        const float dt = (now - last) / 1000.0f;
        last = now;
        dashCamTick(&app, dt);
        const Mat4 view = camView(app.cam);
        tickDash(&hud, view, now);

        if (dashDrawDue(now, &lastDraw, 14)) {
            dashTarget(&app, &hud);
            glViewport(0, 0, hud.width, hud.height);
            glClearColor(0.02f, 0.025f, 0.035f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            const Mat4 proj = perspective(70.0f,
                hud.height ? (float)hud.width / hud.height : 1.0f,
                0.05f, 100.0f);
            const Mat4 vp = multiply(proj, view);
            drawBackdrop(&hud, vp);
            drawDashScene(&hud, vp);
            dashDrainPost();
            dashPresent(&app);
        }
    }
    dashStop(&app);
    comp->destroy();
    return 0;
}
