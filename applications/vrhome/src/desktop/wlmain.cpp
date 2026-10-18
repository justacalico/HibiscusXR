#define VRHOME_DESKTOP 1

#include "dmabuf.h"
#include "harness.h"
#include "wlbackend.h"
#include "wlcomp.h"

#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../render/resources.h"

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>
#include <wayland-server.h>

#include <cstring>
#include <poll.h>

int main(int argc, char** argv) {
    bool mcp = false;
    for (int i = 1; i < argc; ++i)
        mcp |= !strcmp(argv[i], "-mcp");

    WlComp* comp = WlComp::create();
    if (!comp) {
        LOGE("failed to create wayland display");
        return 1;
    }
    LOGI("clients connect with WAYLAND_DISPLAY=%s", comp->sock.c_str());

    DashApp app;
    if (!dashWindow(&app, "Hibiscus dash (wayland)", 1440, 810))
        return 1;
    // needs a current EGL context, so it goes after the window: clients
    // get zwp_linux_dmabuf_v1 only when the host EGL can import dmabufs
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
    hud.backend = &backend;
    hud.haveQuat = true;
    hud.dockAnchored = false;
    hud.summonMs = dashNowMs();
    if (initGlResources(&hud) != 0) return 1;

    const int wfd = wl_event_loop_get_fd(
        wl_display_get_event_loop(comp->dpy));

    if (mcp) dashStartMcp(&app);

    long long last = dashNowMs();
    while (!glfwWindowShouldClose(app.win)) {
        pollfd pfd{wfd, POLLIN, 0};
        if (comp->tasks.empty())
            poll(&pfd, 1, 8);
        else
            poll(&pfd, 1, 4);
        comp->dispatch(0);
        comp->flush();
        glfwPollEvents();
        dashDrainPre();
        const long long now = dashNowMs();
        const float dt = (now - last) / 1000.0f;
        last = now;
        dashCamTick(&app, dt);
        const Mat4 view = camView(app.cam);
        tickDash(&hud, view, now);

        glfwGetFramebufferSize(app.win, &hud.width, &hud.height);
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
        glfwSwapBuffers(app.win);
    }
    dashStop(&app);
    comp->destroy();
    return 0;
}
