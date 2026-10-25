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

// premultiplied RGBA image of one toplevel, composited out of the
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
// popups, subsurfaces, one seat, one output. dmabuf buffers land in
// EGLImages; wl_shm buffers get composited into per-task images, which
// is the only CPU path - everything else is just bookkeeping.
struct WlComp {
    wl_display* dpy = nullptr;
    wl_event_loop* loop = nullptr;
    std::string sock;

    // private session bus shared by every spawned client: remoting and
    // activation traffic (browsers, gtk single-instance apps) stays
    // inside the session because the host bus address never reaches them
    std::string busAddr;
    pid_t busPid = -1;
    bool busTried = false;

    std::vector<WlTask*> tasks;         // in creation order
    std::function<void(WlTask*)> onTaskMap;
    std::function<void(WlTask*)> onTaskDead;

    // seat state
    WlSurf* pointerFocus = nullptr;
    WlSurf* kbdFocus = nullptr;
    WlTask* active = nullptr;
    // task the pointer currently sits in plus its task-local point: the
    // composited cursor draws there (grabbed tasks use grabX/grabY)
    WlTask* ptrTask = nullptr;
    float ptrX = 0.0f, ptrY = 0.0f;
    // input grab: while set every key and pointer event goes straight
    // to this task's toplevel; grabX/grabY is the synthetic cursor in
    // buffer coords
    WlTask* grabTask = nullptr;
    float grabX = 0.0f, grabY = 0.0f;
    WlCompIn* in = nullptr;

    static WlComp* create();
    void destroy();
    void dispatch(int timeoutMs);   // pump client requests
    // push out replies, input events and queued fds. Runs every loop
    // turn so client requests never wait on a draw
    void flushEvents();
    // answer the queued frame callbacks: sent only after a presented
    // frame so clients pace their renders to our draw rate instead of
    // free-running against the poll loop
    void flushFrames();
    void flush();                   // flushEvents + flushFrames

    pid_t spawn(const std::vector<std::string>& argv,
                const std::vector<std::string>& extraEnv);

    uint32_t nextSerial();
    void pointerEvent(WlTask* t, float x, float y, int action);
    // drop pointer focus outright: the aim left every window, so the
    // focused surface gets its wl_pointer.leave and the cursor hides
    void pointerLeave();
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
    // repaint the task's dirty region into img. Deferred work: commits
    // only dirty rects, the caller recomposes once per update so a
    // burst of commits costs one blend, not one blend each
    void recompose(WlTask* t);
    // dmabuf task: retarget tex at its buffer's EGLImage. False for shm
    // tasks - those upload pixels into tex instead
    bool taskGpuBind(WlTask* t, unsigned tex);
};
