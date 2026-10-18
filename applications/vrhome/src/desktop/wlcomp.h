#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unistd.h>

struct wl_display;
struct wl_event_loop;
struct wl_resource;
struct WlSurf;
struct WlCompIn;

// straight-alpha RGBA image of one toplevel, composited out of the
// client's shared-memory buffers
struct WlImage {
    int w = 0, h = 0;
    std::vector<unsigned char> rgba;
};

// one xdg_toplevel: the dash's "task". img is the composited pixels;
// dirty* is the union of content changed since the last GL upload.
struct WlTask {
    int taskId = -1;
    int displayId = -1;        // panel slot it is bound to, -1 unbound
    std::string appId, title;
    pid_t pid = -1;
    WlSurf* surf = nullptr;
    WlImage img;
    bool mapped = false;         // first buffer committed
    bool dirty = false;
    int dx0 = 0, dy0 = 0, dx1 = 0, dy1 = 0;
    int bufW = 0, bufH = 0;    // current toplevel buffer size
    int cfgW = 0, cfgH = 0;    // size the toplevel was last configured to
};

// A minimal Wayland compositor: SHM surfaces, xdg-shell toplevels and
// popups, subsurfaces, one seat, one output. No dmabuf - clients fall
// back to wl_shm and we copy pixels into per-task images for the dash.
struct WlComp {
    wl_display* dpy = nullptr;
    wl_event_loop* loop = nullptr;
    std::string sock;

    std::vector<WlTask*> tasks;         // in creation order
    std::function<void(WlTask*)> onTaskMap;
    std::function<void(WlTask*)> onTaskDead;

    // seat state
    WlSurf* pointerFocus = nullptr;
    WlSurf* kbdFocus = nullptr;
    WlTask* active = nullptr;
    // input grab: while set every key and pointer event goes straight
    // to this task's toplevel; grabX/grabY is the synthetic cursor in
    // buffer coords
    WlTask* grabTask = nullptr;
    float grabX = 0.0f, grabY = 0.0f;
    WlCompIn* in = nullptr;

    static WlComp* create();
    void destroy();
    void dispatch(int timeoutMs);   // pump client requests
    void flush();                   // push out events + frame callbacks

    pid_t spawn(const std::vector<std::string>& argv,
                const std::vector<std::string>& extraEnv);

    uint32_t nextSerial();
    void pointerEvent(WlTask* t, float x, float y, int action);
    void keyEvent(uint32_t evdev, bool down);
    void setActive(WlTask* t, bool on);
    void setGrab(WlTask* t);
    void grabMotion(float dx, float dy);
    void grabButton(uint32_t evdevBtn, bool down);
    void grabAxis(float yoff);
    void requestClose(WlTask* t);
    // resize a task's toplevel: keeps its activated state, no-op when the
    // size already matches
    void configureTask(WlTask* t, int w, int h);
    void taskDirtyRect(WlTask* t, int x, int y, int w, int h);
    void recompose(WlTask* t);
    // dmabuf task: retarget tex at its buffer's EGLImage. False for shm
    // tasks - those upload pixels into tex instead
    bool taskGpuBind(WlTask* t, unsigned tex);
};
