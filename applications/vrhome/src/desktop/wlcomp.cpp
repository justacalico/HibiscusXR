#include "wlcomp.h"

#include "cmdline.h"
#include "dmabuf.h"
#include "pixconv.h"
#include "popuppos.h"
#include "wlegl.h"
#include "../common/config.h"
#include "../common/log.h"

#include <wayland-server.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include "xdg-shell-server-protocol.h"
#include "xdg-activation-v1-server-protocol.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>

struct WlRect { int x = 0, y = 0, w = 0, h = 0; };

enum SurfRole {
    ROLE_NONE = 0,
    ROLE_TOP,
    ROLE_POPUP,
    ROLE_SUB,
    ROLE_CURSOR,
    ROLE_OTHER,
};

// one wl_buffer we currently read pixels from: released back to the
// client once the surface's next attach replaces it
struct WlBuf {
    wl_resource* res = nullptr;
    void* data = nullptr;
    int w = 0, h = 0, stride = 0;
    uint32_t fmt = 0;
    void* eglImg = nullptr;      // dmabuf import; texture path, no pixels
    wl_listener gone = {};
};

struct WlSurf {
    wl_resource* res = nullptr;
    WlComp* c = nullptr;
    WlBuf* buf = nullptr;
    int role = ROLE_NONE;
    wl_resource* xdgRes = nullptr;
    wl_resource* roleRes = nullptr;
    WlSurf* parent = nullptr;
    std::vector<WlSurf*> below, above, pops;
    int offX = 0, offY = 0;      // origin in parent surface coords
    WlTask* task = nullptr;      // ROLE_TOP only
    bool mapped = false;
    std::vector<wl_resource*> frames;
    // pending commit state
    wl_resource* pendBuf = nullptr;
    bool pendAttach = false;
    std::vector<WlRect> pendDamage;
    // xdg_popup placement, copied off the positioner at get_popup
    PopupRect anchorRect;
    int anchorEdge = 0, gravity = 0, popDx = 0, popDy = 0;
    int popW = 0, popH = 0;
};

struct WlPoser {                 // xdg_positioner
    PopupRect anchor;
    int w = 0, h = 0;
    int anchorEdge = 0, gravity = 0, dx = 0, dy = 0;
};

struct SeatRes {
    std::vector<wl_resource*> pointers;
    std::vector<wl_resource*> keyboards;
};

struct PendFrame;

struct WlCompIn {
    std::map<wl_client*, SeatRes> seats;
    std::vector<PendFrame*> pendingFrames;
    std::vector<int> pendingFds;
    std::set<wl_client*> kmSent;
    std::set<std::string> actTokens;
    int actTokN = 0;
    xkb_context* xctx = nullptr;
    xkb_keymap* keymap = nullptr;
    xkb_state* kstate = nullptr;
    std::string kmStr;
    uint32_t depressed = 0, latched = 0, locked = 0, group = 0;
    WlSurf* ptrGrab = nullptr;
    int nextTaskId = 1;
};

// a queued frame callback: the client can destroy the wl_callback
// underneath us (surface teardown, disconnect), so each entry rides a
// destroy listener that pulls it back out of the queue
struct PendFrame {
    WlCompIn* in = nullptr;
    wl_resource* res = nullptr;
    wl_listener gone;
};

static void pendFrameGone(wl_listener* lis, void*) {
    PendFrame* pf = wl_container_of(lis, pf, gone);
    auto& v = pf->in->pendingFrames;
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == pf) { v.erase(v.begin() + i); break; }
    delete pf;
}

static void queueFrame(WlComp* c, wl_resource* res) {
    PendFrame* pf = new PendFrame;
    pf->in = c->in;
    pf->res = res;
    pf->gone.notify = pendFrameGone;
    wl_resource_add_destroy_listener(res, &pf->gone);
    c->in->pendingFrames.push_back(pf);
}

static uint32_t nowMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static WlSurf* surfOf(wl_resource* res) {
    return (WlSurf*)wl_resource_get_user_data(res);
}
static WlComp* compOf(wl_resource* res) {
    return (WlComp*)wl_resource_get_user_data(res);
}

// surface origin inside its task's (toplevel) buffer coords
static void surfOrigin(const WlSurf* s, int* ox, int* oy) {
    int x = 0, y = 0;
    for (const WlSurf* p = s; p && p->role != ROLE_TOP; p = p->parent) {
        x += p->offX; y += p->offY;
    }
    *ox = x; *oy = y;
}

static WlSurf* surfTop(WlSurf* s) {
    while (s && s->role != ROLE_TOP && s->parent) s = s->parent;
    return s;
}

uint32_t WlComp::nextSerial() { return wl_display_next_serial(dpy); }

void WlComp::taskDirtyRect(WlTask* t, int x, int y, int w, int h) {
    if (!t || w <= 0 || h <= 0) return;
    if (!t->dirty) {
        t->dx0 = x; t->dy0 = y; t->dx1 = x + w; t->dy1 = y + h;
        t->dirty = true;
        return;
    }
    if (x < t->dx0) t->dx0 = x;
    if (y < t->dy0) t->dy0 = y;
    if (x + w > t->dx1) t->dx1 = x + w;
    if (y + h > t->dy1) t->dy1 = y + h;
}

// paint order under a node: subs below, node, subs above, popups last
static void paintDfs(WlSurf* n, std::vector<WlSurf*>* out) {
    for (WlSurf* b : n->below) paintDfs(b, out);
    out->push_back(n);
    for (WlSurf* a : n->above) paintDfs(a, out);
    for (WlSurf* p : n->pops) paintDfs(p, out);
}

void WlComp::recompose(WlTask* t) {
    if (!t || !t->surf || !t->surf->buf || !t->dirty) return;
    WlBuf* bb = t->surf->buf;
    if (!bb->data) return;   // gpu buffer: the texture path owns it
    const int bw = bb->w, bh = bb->h;
    if (bw <= 0 || bh <= 0) return;
    if (t->img.w != bw || t->img.h != bh) {
        t->img.w = bw; t->img.h = bh;
        t->img.rgba.assign((size_t)bw * bh * 4, 0);
        t->dx0 = t->dy0 = 0; t->dx1 = bw; t->dy1 = bh;
    }
    WlRect rc{t->dx0, t->dy0, t->dx1 - t->dx0, t->dy1 - t->dy0};
    if (rc.x < 0) { rc.w += rc.x; rc.x = 0; }
    if (rc.y < 0) { rc.h += rc.y; rc.y = 0; }
    if (rc.x + rc.w > bw) rc.w = bw - rc.x;
    if (rc.y + rc.h > bh) rc.h = bh - rc.y;
    if (rc.w <= 0 || rc.h <= 0) { t->dirty = false; return; }
    pxFill(t->img.rgba.data(), bw * 4, bw, bh, rc.x, rc.y, rc.w, rc.h,
           0, 0, 0, 0);
    std::vector<WlSurf*> order;
    paintDfs(t->surf, &order);
    for (WlSurf* s : order) {
        if (!s->buf || !s->buf->data || !s->mapped) continue;
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        const int sx0 = ox > rc.x ? ox : rc.x;
        const int sy0 = oy > rc.y ? oy : rc.y;
        const int sx1 = ox + s->buf->w < rc.x + rc.w ? ox + s->buf->w
                                                   : rc.x + rc.w;
        const int sy1 = oy + s->buf->h < rc.y + rc.h ? oy + s->buf->h
                                                   : rc.y + rc.h;
        if (sx1 <= sx0 || sy1 <= sy0) continue;
        pxBlend((const unsigned char*)s->buf->data, s->buf->stride,
                s->buf->fmt, sx0 - ox, sy0 - oy,
                t->img.rgba.data(), bw * 4, bw, bh,
                sx0, sy0, sx1 - sx0, sy1 - sy0);
    }
}

static void releaseBuf(WlSurf* s) {
    if (!s->buf) return;
    if (s->buf->eglImg) { wlEglFree(s->buf->eglImg); s->buf->eglImg = nullptr; }
    if (s->buf->res) {
        // the buffer resource is still alive, so our destroy listener
        // is still parked on it: unlink before the WlBuf goes away
        wl_list_remove(&s->buf->gone.link);
        wl_buffer_send_release(s->buf->res);
    }
    delete s->buf;
    s->buf = nullptr;
}

static void bufGone(wl_listener* lis, void*) {
    WlBuf* b = wl_container_of(lis, b, gone);
    b->res = nullptr;
    b->data = nullptr;
}

// roll surface damage into the owning task's dirty rect
static void surfApplyDamage(WlSurf* s, bool forceFull) {
    WlSurf* top = surfTop(s);
    if (!top || !top->task) return;
    WlTask* t = top->task;
    int ox, oy;
    surfOrigin(s, &ox, &oy);
    if (forceFull || s->pendDamage.empty()) {
        if (s->buf)
            s->c->taskDirtyRect(t, ox, oy, s->buf->w, s->buf->h);
    } else {
        for (const WlRect& r : s->pendDamage)
            s->c->taskDirtyRect(t, ox + r.x, oy + r.y, r.w, r.h);
    }
}

static void commitSurf(WlSurf* s) {
    if (s->pendAttach) {
        s->pendAttach = false;
        const int ow = s->buf ? s->buf->w : 0;
        const int oh = s->buf ? s->buf->h : 0;
        releaseBuf(s);
        if (s->pendBuf) {
            WlBuf* b = new WlBuf;
            b->res = s->pendBuf;
            if (wl_shm_buffer* shm = wl_shm_buffer_get(s->pendBuf)) {
                b->data = wl_shm_buffer_get_data(shm);
                b->w = wl_shm_buffer_get_width(shm);
                b->h = wl_shm_buffer_get_height(shm);
                b->stride = wl_shm_buffer_get_stride(shm);
                b->fmt = wl_shm_buffer_get_format(shm);
            } else if (DmaBuf* d = dmabufOf(s->pendBuf)) {
                b->w = d->w;
                b->h = d->h;
                b->eglImg = wlEglImport(d);
            }
            b->gone.notify = bufGone;
            wl_resource_add_destroy_listener(b->res, &b->gone);
            s->buf = b;
        }
        s->pendBuf = nullptr;
        const int nw = s->buf ? s->buf->w : 0;
        const int nh = s->buf ? s->buf->h : 0;
        if (nw == ow && nh == oh) {
            // footprint kept: the client's damage rects say what to
            // repaint - the per-commit cost drops to the damaged band
            surfApplyDamage(s, false);
        } else {
            // footprint moved: repaint where it was and where it is
            WlSurf* top = surfTop(s);
            if (top && top->task) {
                int ox, oy;
                surfOrigin(s, &ox, &oy);
                s->c->taskDirtyRect(top->task, ox, oy, ow, oh);
                s->c->taskDirtyRect(top->task, ox, oy, nw, nh);
            }
        }
    } else {
        surfApplyDamage(s, false);
    }
    s->pendDamage.clear();

    if (s->buf && (s->buf->data || s->buf->eglImg) && !s->mapped) {
        s->mapped = true;
        if (s->task) s->task->mapped = true;
        if (s->role == ROLE_TOP && s->task && s->c->onTaskMap)
            s->c->onTaskMap(s->task);
    }
    if (!s->buf && s->role == ROLE_TOP && s->task && s->mapped) {
        // commit(NULL) unmaps the toplevel: the panel dies with it
        s->mapped = false;
        if (s->c->onTaskDead) s->c->onTaskDead(s->task);
    }
    if (!s->frames.empty()) {
        for (wl_resource* f : s->frames) queueFrame(s->c, f);
        s->frames.clear();
    }
    if (s->task && s->buf) {
        s->task->bufW = s->buf->w;
        s->task->bufH = s->buf->h;
    }
    // no recompose here: the update pass repaints the dirty union once,
    // so a burst of commits costs one blend instead of one each
}

static void unlinkKid(WlSurf* s) {
    if (!s->parent) return;
    auto drop = [&](std::vector<WlSurf*>& v) {
        for (size_t i = 0; i < v.size(); ++i)
            if (v[i] == s) { v.erase(v.begin() + i); return; }
    };
    WlSurf* top = surfTop(s->parent);
    if (top && top->task) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        const int w = s->buf ? s->buf->w : 0;
        const int h = s->buf ? s->buf->h : 0;
        s->c->taskDirtyRect(top->task, ox, oy, w, h);
    }
    drop(s->parent->below);
    drop(s->parent->above);
    drop(s->parent->pops);
    s->parent = nullptr;
}

// task teardown shared by surface destroy and xdg_surface destroy:
// kills the task record and reports the panel dead
static void taskTeardown(WlSurf* s) {
    if (!s->task) return;
    if (s->c->active == s->task) {
        s->c->kbdFocus = nullptr;
        s->c->active = nullptr;
    }
    if (s->c->grabTask == s->task) s->c->grabTask = nullptr;
    if (s->c->onTaskDead) s->c->onTaskDead(s->task);
    for (size_t i = 0; i < s->c->tasks.size(); ++i)
        if (s->c->tasks[i] == s->task) {
            s->c->tasks.erase(s->c->tasks.begin() + i);
            break;
        }
    delete s->task;
    s->task = nullptr;
}

static void freeSurf(WlSurf* s) {
    unlinkKid(s);
    for (WlSurf* k : s->below) k->parent = nullptr;
    for (WlSurf* k : s->above) k->parent = nullptr;
    for (WlSurf* k : s->pops) k->parent = nullptr;
    s->below.clear(); s->above.clear(); s->pops.clear();
    taskTeardown(s);
    if (s->c->pointerFocus == s) s->c->pointerFocus = nullptr;
    if (s->c->kbdFocus == s) s->c->kbdFocus = nullptr;
    if (s->c->in->ptrGrab == s) s->c->in->ptrGrab = nullptr;
    // frame callbacks the surface never got answered die with it, same
    // as a presented callback would
    for (wl_resource* f : s->frames) wl_resource_destroy(f);
    releaseBuf(s);
    delete s;
}

// ---------------------------------------------------------- wl_surface

static void reqSurfDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqSurfAttach(wl_client*, wl_resource* res,
                          wl_resource* buf, int32_t, int32_t) {
    WlSurf* s = surfOf(res);
    s->pendBuf = buf;
    s->pendAttach = true;
}
static void reqSurfDamage(wl_client*, wl_resource* res,
                          int32_t x, int32_t y, int32_t w, int32_t h) {
    surfOf(res)->pendDamage.push_back({x, y, w, h});
}
static void reqSurfFrame(wl_client* cl, wl_resource* res, uint32_t cb) {
    WlSurf* s = surfOf(res);
    s->frames.push_back(
        wl_resource_create(cl, &wl_callback_interface, 1, cb));
}
static void reqSurfNoop(wl_client*, wl_resource*, wl_resource*) {}
static void reqSurfCommit(wl_client*, wl_resource* res) {
    commitSurf(surfOf(res));
}
static void reqSurfNoopI(wl_client*, wl_resource*, int32_t) {}
static void reqSurfNoopII(wl_client*, wl_resource*, int32_t, int32_t) {}

static const struct wl_surface_interface surfImpl = {
    reqSurfDestroy, reqSurfAttach, reqSurfDamage, reqSurfFrame,
    reqSurfNoop /*opaque*/, reqSurfNoop /*input*/, reqSurfCommit,
    reqSurfNoopI /*transform*/, reqSurfNoopI /*scale*/,
    reqSurfDamage /*damage_buffer*/, reqSurfNoopII /*offset*/,
};

static void surfResDestroy(wl_resource* res) { freeSurf(surfOf(res)); }

// ------------------------------------------------------------- regions

static void reqRegionDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqRegionNoop(wl_client*, wl_resource*, int32_t, int32_t,
                          int32_t, int32_t) {}
static const struct wl_region_interface regionImpl = {
    reqRegionDestroy, reqRegionNoop, reqRegionNoop,
};

// -------------------------------------------------------- wl_compositor

static void reqCreateSurface(wl_client* cl, wl_resource* res,
                             uint32_t id) {
    WlComp* c = compOf(res);
    WlSurf* s = new WlSurf;
    s->c = c;
    s->res = wl_resource_create(cl, &wl_surface_interface,
                                wl_resource_get_version(res), id);
    wl_resource_set_implementation(s->res, &surfImpl, s,
                                   surfResDestroy);
}

static void reqCreateRegion(wl_client* cl, wl_resource* res,
                            uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_region_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(r, &regionImpl, nullptr, nullptr);
}

static const struct wl_compositor_interface compImpl = {
    reqCreateSurface, reqCreateRegion,
};

// -------------------------------------------------------------- wl_seat

static void reqPtrRelease(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqPtrSetCursor(wl_client*, wl_resource*, uint32_t,
                            wl_resource* surfRes, int32_t, int32_t) {
    if (surfRes) surfOf(surfRes)->role = ROLE_CURSOR;
}
static const struct wl_pointer_interface ptrImpl = {
    reqPtrSetCursor, reqPtrRelease,
};

static void reqKbdRelease(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct wl_keyboard_interface kbdImpl = { reqKbdRelease };

static void keymapTo(wl_client* cl, wl_resource* kres) {
    WlComp* c = compOf(kres);
    if (!c || c->in->kmStr.empty()) return;
    int fd = memfd_create("dash-keymap", MFD_CLOEXEC);
    if (fd < 0) return;
    const std::string& s = c->in->kmStr;
    if (write(fd, s.data(), s.size() + 1) != (ssize_t)s.size() + 1) {
        close(fd);
        return;
    }
    lseek(fd, 0, SEEK_SET);
    wl_keyboard_send_keymap(kres, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                            fd, (uint32_t)s.size() + 1);
    c->in->pendingFds.push_back(fd);
    c->in->kmSent.insert(cl);
}

static void reqSeatPointer(wl_client* cl, wl_resource* res,
                           uint32_t id) {
    WlComp* c = compOf(res);
    wl_resource* p = wl_resource_create(cl, &wl_pointer_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(p, &ptrImpl, c,
        [](wl_resource* r) {
            WlComp* cc = (WlComp*)wl_resource_get_user_data(r);
            auto& v = cc->in->seats[wl_resource_get_client(r)].pointers;
            for (size_t i = 0; i < v.size(); ++i)
                if (v[i] == r) { v.erase(v.begin() + i); return; }
        });
    c->in->seats[cl].pointers.push_back(p);
}
static void reqSeatKeyboard(wl_client* cl, wl_resource* res,
                            uint32_t id) {
    WlComp* c = compOf(res);
    wl_resource* k = wl_resource_create(cl, &wl_keyboard_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(k, &kbdImpl, c,
        [](wl_resource* r) {
            WlComp* cc = (WlComp*)wl_resource_get_user_data(r);
            auto& v = cc->in->seats[wl_resource_get_client(r)].keyboards;
            for (size_t i = 0; i < v.size(); ++i)
                if (v[i] == r) { v.erase(v.begin() + i); return; }
        });
    c->in->seats[cl].keyboards.push_back(k);
    keymapTo(cl, k);
}
static void reqSeatTouch(wl_client* cl, wl_resource* res, uint32_t id) {
    // no touch seats: give the client an inert object, we never send
    // caps for it anyway
    wl_resource* t = wl_resource_create(cl, &wl_touch_interface,
                                        wl_resource_get_version(res), id);
    static const struct wl_touch_interface ti = { reqKbdRelease };
    wl_resource_set_implementation(t, &ti, nullptr, nullptr);
}
static void reqSeatRelease(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct wl_seat_interface seatImpl = {
    reqSeatPointer, reqSeatKeyboard, reqSeatTouch, reqSeatRelease,
};

// -------------------------------------------------------------- wl_output

static void reqOutRelease(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct wl_output_interface outImpl = { reqOutRelease };

// -------------------------------------------------------- subsurfaces

static void reqSubDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqSubSetPos(wl_client*, wl_resource* res,
                         int32_t x, int32_t y) {
    WlSurf* s = surfOf(res);
    WlSurf* top = surfTop(s);
    if (top && top->task && s->buf) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        s->c->taskDirtyRect(top->task, ox, oy,
                            s->buf->w, s->buf->h);
    }
    s->offX = x; s->offY = y;
    if (top && top->task && s->buf) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        s->c->taskDirtyRect(top->task, ox, oy,
                            s->buf->w, s->buf->h);
    }
}
static void moveSub(WlSurf* s, WlSurf* sib, bool below) {
    if (!s->parent) return;
    std::vector<WlSurf*>& lo = s->parent->below;
    std::vector<WlSurf*>& hi = s->parent->above;
    auto drop = [&](std::vector<WlSurf*>& v) {
        for (size_t i = 0; i < v.size(); ++i)
            if (v[i] == s) { v.erase(v.begin() + i); return; }
    };
    auto place = [&](std::vector<WlSurf*>& v, bool after) {
        for (size_t i = 0; i < v.size(); ++i)
            if (v[i] == sib) {
                v.insert(v.begin() + i + (after ? 1 : 0), s);
                return;
            }
        v.push_back(s);
    };
    drop(lo); drop(hi);
    if (below) place(lo, false); else place(hi, true);
    WlSurf* top = surfTop(s);
    if (top && top->task && s->buf) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        s->c->taskDirtyRect(top->task, ox, oy,
                            s->buf->w, s->buf->h);
    }
}
static void reqSubAbove(wl_client*, wl_resource* res, wl_resource* sib) {
    moveSub(surfOf(res), surfOf(sib), false);
}
static void reqSubBelow(wl_client*, wl_resource* res, wl_resource* sib) {
    moveSub(surfOf(res), surfOf(sib), true);
}
static void reqSubSync(wl_client*, wl_resource*) {}
static void reqSubDesync(wl_client*, wl_resource*) {}
static const struct wl_subsurface_interface subImpl = {
    reqSubDestroy, reqSubSetPos, reqSubAbove, reqSubBelow,
    reqSubSync, reqSubDesync,
};

static void reqGetSub(wl_client* cl, wl_resource* res, uint32_t id,
                      wl_resource* surfRes, wl_resource* parentRes) {
    WlSurf* s = surfOf(surfRes);
    WlSurf* p = surfOf(parentRes);
    if (!s || !p || s == p || s->role != ROLE_NONE) {
        wl_resource_post_error(res,
            WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE, "bad subsurface");
        return;
    }
    s->role = ROLE_SUB;
    s->parent = p;
    p->above.push_back(s);
    s->roleRes = wl_resource_create(cl, &wl_subsurface_interface,
                                    wl_resource_get_version(res), id);
    wl_resource_set_implementation(s->roleRes, &subImpl, s,
        [](wl_resource* r) {
            WlSurf* ss = surfOf(r);
            unlinkKid(ss);
            ss->role = ROLE_NONE;
            ss->roleRes = nullptr;
            ss->mapped = false;
        });
}

static void reqSubcompDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct wl_subcompositor_interface subcompImpl = {
    reqSubcompDestroy, reqGetSub,
};

// ---------------------------------------------------------- data devices

static void reqDataDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqOfferAccept(wl_client*, wl_resource*, uint32_t,
                           const char*) {}
static void reqOfferReceive(wl_client*, wl_resource*, const char*,
                            int32_t fd) {
    if (fd >= 0) close(fd);
}
static void reqOfferNoop(wl_client*, wl_resource*) {}
static void reqOfferAct(wl_client*, wl_resource*, uint32_t, uint32_t) {}
static const struct wl_data_offer_interface offerImpl = {
    reqOfferAccept, reqOfferReceive, reqDataDestroy,
    reqOfferNoop /*finish*/, reqOfferAct,
};

static void reqSrcOffer(wl_client*, wl_resource*, const char*) {}
static void reqSrcAct(wl_client*, wl_resource*, uint32_t) {}
static const struct wl_data_source_interface srcImpl = {
    reqSrcOffer, reqDataDestroy, reqSrcAct,
};

static void reqDevDrag(wl_client*, wl_resource*, wl_resource*,
                       wl_resource*, wl_resource*, uint32_t) {}
static void reqDevSel(wl_client*, wl_resource*, wl_resource*, uint32_t) {}
static const struct wl_data_device_interface devImpl = {
    reqDevDrag, reqDevSel, reqDataDestroy,
};

static void reqCreateSrc(wl_client* cl, wl_resource* res, uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_data_source_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(r, &srcImpl, nullptr, nullptr);
}
static void reqGetDev(wl_client* cl, wl_resource* res, uint32_t id,
                      wl_resource*) {
    wl_resource* r = wl_resource_create(cl, &wl_data_device_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(r, &devImpl, nullptr, nullptr);
}
static const struct wl_data_device_manager_interface ddmImpl = {
    reqCreateSrc, reqGetDev,
};

// -------------------------------------------------------------- xdg shell

static void reqPoserDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqPoserSize(wl_client*, wl_resource* res,
                         int32_t w, int32_t h) {
    WlPoser* p = (WlPoser*)wl_resource_get_user_data(res);
    p->w = w; p->h = h;
}
static void reqPoserAnchorRect(wl_client*, wl_resource* res,
                               int32_t x, int32_t y,
                               int32_t w, int32_t h) {
    WlPoser* p = (WlPoser*)wl_resource_get_user_data(res);
    p->anchor = {x, y, w, h};
}
static void reqPoserAnchor(wl_client*, wl_resource* res, uint32_t a) {
    ((WlPoser*)wl_resource_get_user_data(res))->anchorEdge = (int)a;
}
static void reqPoserGravity(wl_client*, wl_resource* res, uint32_t g) {
    ((WlPoser*)wl_resource_get_user_data(res))->gravity = (int)g;
}
static void reqPoserConstraint(wl_client*, wl_resource*, uint32_t) {}
static void reqPoserOffset(wl_client*, wl_resource* res,
                           int32_t x, int32_t y) {
    WlPoser* p = (WlPoser*)wl_resource_get_user_data(res);
    p->dx = x; p->dy = y;
}
static void reqPoserReactive(wl_client*, wl_resource*) {}
static void reqPoserParentSize(wl_client*, wl_resource*,
                               int32_t, int32_t) {}
static void reqPoserParentCfg(wl_client*, wl_resource*, uint32_t) {}
static const struct xdg_positioner_interface poserImpl = {
    reqPoserDestroy, reqPoserSize, reqPoserAnchorRect, reqPoserAnchor,
    reqPoserGravity, reqPoserConstraint, reqPoserOffset,
    reqPoserReactive, reqPoserParentSize, reqPoserParentCfg,
};

static void sendXdgCfg(WlSurf* s) {
    xdg_surface_send_configure(s->xdgRes, s->c->nextSerial());
}

static void sendTopCfg(WlSurf* s, int w, int h, bool active) {
    wl_array states;
    wl_array_init(&states);
    if (active) {
        uint32_t* st = (uint32_t*)wl_array_add(&states, sizeof(uint32_t));
        *st = XDG_TOPLEVEL_STATE_ACTIVATED;
    }
    xdg_toplevel_send_configure(s->roleRes, w, h, &states);
    wl_array_release(&states);
    sendXdgCfg(s);
}

static void reqTopDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqTopSetParent(wl_client*, wl_resource*, wl_resource*) {}
static void reqTopSetTitle(wl_client*, wl_resource* res,
                           const char* t) {
    WlSurf* s = surfOf(res);
    if (s->task) s->task->title = t ? t : "";
}
static void reqTopSetAppId(wl_client*, wl_resource* res,
                           const char* a) {
    WlSurf* s = surfOf(res);
    if (s->task) s->task->appId = a ? a : "";
}
static void reqTopMenu(wl_client*, wl_resource*, wl_resource*,
                       uint32_t, int32_t, int32_t) {}
static void reqTopMove(wl_client*, wl_resource*, wl_resource*,
                       uint32_t) {}
static void reqTopGrab(wl_client*, wl_resource*, wl_resource*,
                       uint32_t, uint32_t) {}
static void reqTopNoopII(wl_client*, wl_resource*, int32_t, int32_t) {}
static void reqTopNoopR(wl_client*, wl_resource*, wl_resource*) {}
static void reqTopNoop(wl_client*, wl_resource*) {}
static const struct xdg_toplevel_interface topImpl = {
    reqTopDestroy, reqTopSetParent, reqTopSetTitle, reqTopSetAppId,
    reqTopMenu /*show_window_menu*/, reqTopMove /*move*/,
    reqTopGrab /*resize*/, reqTopNoopII /*set_max_size*/,
    reqTopNoopII /*set_min_size*/, reqTopNoop /*max*/,
    reqTopNoop /*unmax*/, reqTopNoopR /*fullscreen*/,
    reqTopNoop /*unfullscreen*/, reqTopNoop /*minimized*/,
};

static void reqPopupDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqPopupGrab(wl_client*, wl_resource*, wl_resource*,
                         uint32_t) {}
static void reqPopupRepos(wl_client*, wl_resource* res,
                          wl_resource* poserRes, uint32_t) {
    WlSurf* s = surfOf(res);
    WlPoser* p = (WlPoser*)wl_resource_get_user_data(poserRes);
    int x, y;
    popupPos(p->anchor, p->w, p->h, p->anchorEdge, p->gravity,
             p->dx, p->dy, &x, &y);
    WlSurf* top = surfTop(s);
    if (top && top->task && s->buf) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        s->c->taskDirtyRect(top->task, ox, oy, s->buf->w, s->buf->h);
    }
    s->offX = x; s->offY = y;
    if (top && top->task && s->buf) {
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        s->c->taskDirtyRect(top->task, ox, oy, s->buf->w, s->buf->h);
    }
    xdg_popup_send_configure(s->roleRes, x, y, p->w, p->h);
    sendXdgCfg(s);
}
static const struct xdg_popup_interface popupImpl = {
    reqPopupDestroy, reqPopupGrab, reqPopupRepos,
};

static void reqXsurfDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqGetTop(wl_client* cl, wl_resource* res, uint32_t id) {
    WlSurf* s = surfOf(res);
    if (s->role != ROLE_NONE) {
        wl_resource_post_error(res, XDG_WM_BASE_ERROR_ROLE,
                               "surface already has a role");
        return;
    }
    s->role = ROLE_TOP;
    s->roleRes = wl_resource_create(cl, &xdg_toplevel_interface,
                                    wl_resource_get_version(res), id);
    wl_resource_set_implementation(s->roleRes, &topImpl, s, nullptr);
    s->task = new WlTask;
    s->task->taskId = s->c->in->nextTaskId++;
    s->task->surf = s;
    s->task->cfgW = kVdW;
    s->task->cfgH = kVdH;
    pid_t pid = -1;
    wl_client_get_credentials(cl, &pid, nullptr, nullptr);
    s->task->pid = pid;
    s->c->tasks.push_back(s->task);
    sendTopCfg(s, kVdW, kVdH, false);
}
static void reqGetPopup(wl_client* cl, wl_resource* res, uint32_t id,
                        wl_resource* parentRes, wl_resource* poserRes) {
    WlSurf* s = surfOf(res);
    WlSurf* p = parentRes ? surfOf(parentRes) : nullptr;
    if (s->role != ROLE_NONE || !p) {
        wl_resource_post_error(res, XDG_WM_BASE_ERROR_ROLE,
                               "bad popup");
        return;
    }
    WlPoser* pos = (WlPoser*)wl_resource_get_user_data(poserRes);
    int x, y;
    popupPos(pos->anchor, pos->w, pos->h, pos->anchorEdge, pos->gravity,
             pos->dx, pos->dy, &x, &y);
    s->role = ROLE_POPUP;
    s->parent = p;
    p->pops.push_back(s);
    s->offX = x; s->offY = y;
    s->roleRes = wl_resource_create(cl, &xdg_popup_interface,
                                    wl_resource_get_version(res), id);
    wl_resource_set_implementation(s->roleRes, &popupImpl, s,
        [](wl_resource* r) {
            WlSurf* ss = surfOf(r);
            unlinkKid(ss);
            ss->role = ROLE_NONE;
            ss->roleRes = nullptr;
            ss->mapped = false;
        });
    xdg_popup_send_configure(s->roleRes, x, y, pos->w, pos->h);
    sendXdgCfg(s);
}
static void reqXsurfGeom(wl_client*, wl_resource*,
                         int32_t, int32_t, int32_t, int32_t) {}
static void reqXsurfAck(wl_client*, wl_resource*, uint32_t) {}
static const struct xdg_surface_interface xsurfImpl = {
    reqXsurfDestroy, reqGetTop, reqGetPopup, reqXsurfGeom, reqXsurfAck,
};

static void xsurfResDestroy(wl_resource* res) {
    WlSurf* s = surfOf(res);
    s->xdgRes = nullptr;
    // the wl_surface outlives its xdg_surface by one destroy call;
    // task/role cleanup is all that has to happen here
    taskTeardown(s);
    unlinkKid(s);
    s->role = ROLE_NONE;
    s->roleRes = nullptr;
    s->mapped = false;
}

static void reqWmDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqCreatePoser(wl_client* cl, wl_resource* res,
                           uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &xdg_positioner_interface,
                                        wl_resource_get_version(res), id);
    wl_resource_set_implementation(r, &poserImpl, new WlPoser,
        [](wl_resource* rr) {
            delete (WlPoser*)wl_resource_get_user_data(rr);
        });
}
static void reqGetXsurf(wl_client* cl, wl_resource* res, uint32_t id,
                        wl_resource* surfRes) {
    WlSurf* s = surfOf(surfRes);
    if (!s || s->xdgRes) {
        wl_resource_post_error(res, XDG_WM_BASE_ERROR_ROLE,
                               "not a plain surface");
        return;
    }
    s->xdgRes = wl_resource_create(cl, &xdg_surface_interface,
                                   wl_resource_get_version(res), id);
    wl_resource_set_implementation(s->xdgRes, &xsurfImpl, s,
                                   xsurfResDestroy);
}
static void reqWmPong(wl_client*, wl_resource*, uint32_t) {}
static const struct xdg_wm_base_interface wmImpl = {
    reqWmDestroy, reqCreatePoser, reqGetXsurf, reqWmPong,
};

// -------------------------------------------------------------- globals

static void bindComp(wl_client* cl, void* data, uint32_t ver,
                     uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_compositor_interface,
                                        ver > 4 ? 4 : ver, id);
    wl_resource_set_implementation(r, &compImpl, data, nullptr);
}
static void bindSeat(wl_client* cl, void* data, uint32_t ver,
                     uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_seat_interface,
                                        ver > 5 ? 5 : ver, id);
    wl_resource_set_implementation(r, &seatImpl, data, nullptr);
    wl_seat_send_capabilities(r, WL_SEAT_CAPABILITY_POINTER |
                                 WL_SEAT_CAPABILITY_KEYBOARD);
}
static void bindOut(wl_client* cl, void* data, uint32_t ver,
                    uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_output_interface,
                                        ver > 2 ? 2 : ver, id);
    wl_resource_set_implementation(r, &outImpl, data, nullptr);
    wl_output_send_geometry(r, 0, 0, 275, 155, 0, "hibiscus", "dash", 0);
    wl_output_send_mode(r, WL_OUTPUT_MODE_CURRENT, 1920, 1080, 60000);
    wl_output_send_scale(r, 1);
    wl_output_send_done(r);
}
static void bindSubcomp(wl_client* cl, void* data, uint32_t,
                        uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &wl_subcompositor_interface,
                                        1, id);
    wl_resource_set_implementation(r, &subcompImpl, data, nullptr);
}
static void bindDdm(wl_client* cl, void* data, uint32_t ver,
                    uint32_t id) {
    wl_resource* r = wl_resource_create(cl,
                                        &wl_data_device_manager_interface,
                                        ver > 3 ? 3 : ver, id);
    wl_resource_set_implementation(r, &ddmImpl, data, nullptr);
}
static void bindWm(wl_client* cl, void* data, uint32_t ver,
                   uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &xdg_wm_base_interface,
                                        ver > 3 ? 3 : ver, id);
    wl_resource_set_implementation(r, &wmImpl, data, nullptr);
}

// ------------------------------------------------- xdg_activation_v1
// GTK/GApplication "activate the running instance" and friends hand us
// a token then ask to raise a surface; we just mark its task active

struct ActTok {
    WlComp* c = nullptr;
    bool committed = false;
    wl_listener gone;
};

static ActTok* tokOf(wl_resource* res) {
    return (ActTok*)wl_resource_get_user_data(res);
}

static void actTokGone(wl_listener* lis, void*) {
    ActTok* t = wl_container_of(lis, t, gone);
    delete t;
}

static void reqTokSerial(wl_client*, wl_resource*, uint32_t,
                         wl_resource*) {}
static void reqTokAppId(wl_client*, wl_resource*, const char*) {}
static void reqTokSurface(wl_client*, wl_resource*, wl_resource*) {}
static void reqTokCommit(wl_client*, wl_resource* res) {
    ActTok* t = tokOf(res);
    if (t->committed) {
        wl_resource_post_error(res,
            XDG_ACTIVATION_TOKEN_V1_ERROR_ALREADY_USED,
            "activation token committed twice");
        return;
    }
    t->committed = true;
    char tok[32];
    snprintf(tok, sizeof(tok), "hibi%x-%x", (unsigned)nowMs(),
             (unsigned)t->c->in->actTokN++);
    t->c->in->actTokens.insert(tok);
    xdg_activation_token_v1_send_done(res, tok);
}
static void reqTokDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct xdg_activation_token_v1_interface actTokImpl = {
    reqTokSerial, reqTokAppId, reqTokSurface, reqTokCommit,
    reqTokDestroy,
};

static void reqActDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static void reqActToken(wl_client* cl, wl_resource* res, uint32_t id) {
    ActTok* t = new ActTok;
    t->c = compOf(res);
    wl_resource* tr = wl_resource_create(cl,
        &xdg_activation_token_v1_interface,
        wl_resource_get_version(res), id);
    wl_resource_set_implementation(tr, &actTokImpl, t, nullptr);
    t->gone.notify = actTokGone;
    wl_resource_add_destroy_listener(tr, &t->gone);
}
static void reqActActivate(wl_client*, wl_resource* res,
                           const char*, wl_resource* surfRes) {
    WlComp* c = compOf(res);
    WlSurf* s = surfRes ? surfOf(surfRes) : nullptr;
    if (!s || !s->task) return;
    // permissive on unknown/foreign tokens: activate only ever moves
    // focus to a window the client already owns
    c->setActive(s->task, true);
}
static const struct xdg_activation_v1_interface actImpl = {
    reqActDestroy, reqActToken, reqActActivate,
};

static void bindAct(wl_client* cl, void* data, uint32_t ver,
                    uint32_t id) {
    wl_resource* r = wl_resource_create(cl, &xdg_activation_v1_interface,
                                        ver > 1 ? 1 : ver, id);
    wl_resource_set_implementation(r, &actImpl, data, nullptr);
}

// -------------------------------------------------------------- WlComp

WlComp* WlComp::create() {
    WlComp* c = new WlComp;
    c->in = new WlCompIn;
    c->dpy = wl_display_create();
    if (!c->dpy) { delete c->in; delete c; return nullptr; }
    c->loop = wl_display_get_event_loop(c->dpy);
    const char* name = wl_display_add_socket_auto(c->dpy);
    if (!name) {
        wl_display_destroy(c->dpy);
        delete c->in; delete c;
        return nullptr;
    }
    c->sock = name;
    // libwayland serves wl_shm/pool/buffer internally; only the two
    // base formats are advertised, so every client lands on shm
    if (wl_display_init_shm(c->dpy) != 0) {
        wl_display_destroy(c->dpy);
        delete c->in; delete c;
        return nullptr;
    }
    wl_global_create(c->dpy, &wl_compositor_interface, 4, c, bindComp);
    wl_global_create(c->dpy, &wl_seat_interface, 5, c, bindSeat);
    wl_global_create(c->dpy, &wl_output_interface, 2, c, bindOut);
    wl_global_create(c->dpy, &wl_subcompositor_interface, 1, c,
                     bindSubcomp);
    wl_global_create(c->dpy, &wl_data_device_manager_interface, 3, c,
                     bindDdm);
    wl_global_create(c->dpy, &xdg_wm_base_interface, 3, c, bindWm);
    wl_global_create(c->dpy, &xdg_activation_v1_interface, 1, c,
                     bindAct);

    c->in->xctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (c->in->xctx) {
        c->in->keymap = xkb_keymap_new_from_names(c->in->xctx, nullptr,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (c->in->keymap) {
            c->in->kstate = xkb_state_new(c->in->keymap);
            char* s = xkb_keymap_get_as_string(c->in->keymap,
                XKB_KEYMAP_FORMAT_TEXT_V1);
            if (s) { c->in->kmStr = s; free(s); }
        }
    }
    LOGI("wlcomp on %s", c->sock.c_str());
    return c;
}

void WlComp::destroy() {
    if (dpy) {
        wl_display_destroy_clients(dpy);
        wl_display_destroy(dpy);
        dpy = nullptr;
    }
    for (WlTask* t : tasks) delete t;
    tasks.clear();
    if (in) {
        if (in->kstate) xkb_state_unref(in->kstate);
        if (in->keymap) xkb_keymap_unref(in->keymap);
        if (in->xctx) xkb_context_unref(in->xctx);
        delete in;
        in = nullptr;
    }
    delete this;
}

bool WlComp::taskGpuBind(WlTask* t, unsigned tex) {
    if (!t || !t->surf || !t->surf->buf || !t->surf->buf->eglImg)
        return false;
    return wlEglBindTex(tex, t->surf->buf->eglImg);
}

void WlComp::dispatch(int timeoutMs) {
    if (!dpy) return;
    int st;
    while (waitpid(-1, &st, WNOHANG) > 0) {}
    wl_event_loop_dispatch(loop, timeoutMs);
    wl_display_flush_clients(dpy);
}

void WlComp::flushEvents() {
    if (!dpy) return;
    wl_display_flush_clients(dpy);
    for (int fd : in->pendingFds) close(fd);
    in->pendingFds.clear();
}

void WlComp::flushFrames() {
    if (!dpy) return;
    const uint32_t t = nowMs();
    std::vector<PendFrame*> fs;
    fs.swap(in->pendingFrames);
    for (PendFrame* pf : fs) {
        wl_list_remove(&pf->gone.link);
        wl_callback_send_done(pf->res, t);
        wl_resource_destroy(pf->res);
        delete pf;
    }
}

void WlComp::flush() {
    flushFrames();
    flushEvents();
}

pid_t WlComp::spawn(const std::vector<std::string>& argv,
                    const std::vector<std::string>& extraEnv) {
    if (argv.empty()) return -1;
    const pid_t pid = fork();
    if (pid != 0) return pid;
    setsid();
    sigset_t set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, nullptr);
    setenv("WAYLAND_DISPLAY", sock.c_str(), 1);
    unsetenv("DISPLAY");
    setenv("GDK_BACKEND", "wayland", 0);
    setenv("QT_QPA_PLATFORM", "wayland", 0);
    setenv("MOZ_ENABLE_WAYLAND", "1", 0);
    setenv("SDL_VIDEODRIVER", "wayland", 0);
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    for (const std::string& e : extraEnv) {
        const size_t eq = e.find('=');
        if (eq != std::string::npos)
            setenv(e.substr(0, eq).c_str(), e.substr(eq + 1).c_str(), 1);
    }
    const int dn = open("/dev/null", O_RDWR);
    if (dn != -1) {
        dup2(dn, STDIN_FILENO);
        dup2(dn, STDOUT_FILENO);
        dup2(dn, STDERR_FILENO);
        if (dn > STDERR_FILENO) close(dn);
    }
    const std::string cmd = shellJoin(argv);
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
    _exit(127);
}

// deepest surface under a task-local point
static WlSurf* pickSurf(WlTask* t, float x, float y, float* sx,
                        float* sy) {
    if (!t || !t->surf) return nullptr;
    std::vector<WlSurf*> order;
    paintDfs(t->surf, &order);
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        WlSurf* s = *it;
        if (!s->buf || !s->mapped) continue;
        int ox, oy;
        surfOrigin(s, &ox, &oy);
        if (x >= ox && y >= oy && x < ox + s->buf->w &&
                y < oy + s->buf->h) {
            *sx = x - ox; *sy = y - oy;
            return s;
        }
    }
    return nullptr;
}

static void ptrTo(WlComp* c, WlSurf* s, uint32_t serial,
                  float x, float y) {
    wl_client* cl = wl_resource_get_client(s->res);
    for (wl_resource* p : c->in->seats[cl].pointers)
        wl_pointer_send_enter(p, serial, s->res,
                              wl_fixed_from_double(x),
                              wl_fixed_from_double(y));
}

static void ptrLeave(WlComp* c, WlSurf* s) {
    wl_client* cl = wl_resource_get_client(s->res);
    const uint32_t ser = c->nextSerial();
    for (wl_resource* p : c->in->seats[cl].pointers)
        wl_pointer_send_leave(p, ser, s->res);
}

void WlComp::pointerEvent(WlTask* t, float x, float y, int action) {
    if (grabTask) t = grabTask;
    float sx = 0, sy = 0;
    WlSurf* hit = in->ptrGrab ? in->ptrGrab
                              : pickSurf(t, x, y, &sx, &sy);
    if (hit && hit != pointerFocus && !in->ptrGrab) {
        if (pointerFocus) ptrLeave(this, pointerFocus);
        ptrTo(this, hit, nextSerial(), sx, sy);
        pointerFocus = hit;
    } else if (!hit && pointerFocus && !in->ptrGrab) {
        ptrLeave(this, pointerFocus);
        pointerFocus = nullptr;
    }
    WlSurf* tgt = in->ptrGrab ? in->ptrGrab : hit;
    if (!tgt) return;
    int ox, oy;
    surfOrigin(tgt, &ox, &oy);
    sx = x - ox; sy = y - oy;
    wl_client* cl = wl_resource_get_client(tgt->res);
    const uint32_t ms = nowMs();
    for (wl_resource* p : in->seats[cl].pointers) {
        if (action == 0) {
            wl_pointer_send_motion(p, ms, wl_fixed_from_double(sx),
                                   wl_fixed_from_double(sy));
            wl_pointer_send_button(p, nextSerial(), ms, BTN_LEFT,
                                   WL_POINTER_BUTTON_STATE_PRESSED);
        } else if (action == 1) {
            wl_pointer_send_button(p, nextSerial(), ms, BTN_LEFT,
                                   WL_POINTER_BUTTON_STATE_RELEASED);
            wl_pointer_send_motion(p, ms, wl_fixed_from_double(sx),
                                   wl_fixed_from_double(sy));
        } else {
            wl_pointer_send_motion(p, ms, wl_fixed_from_double(sx),
                                   wl_fixed_from_double(sy));
        }
        if (wl_resource_get_version(p) >= 5)
            wl_pointer_send_frame(p);
    }
    if (action == 0) in->ptrGrab = tgt;
    else if (action == 1) in->ptrGrab = nullptr;
}

static void kbdTo(WlComp* c, WlSurf* s, bool enter) {
    if (!s) return;
    wl_client* cl = wl_resource_get_client(s->res);
    const uint32_t ser = c->nextSerial();
    for (wl_resource* k : c->in->seats[cl].keyboards) {
        if (enter) {
            wl_array keys;
            wl_array_init(&keys);
            wl_keyboard_send_enter(k, ser, s->res, &keys);
            wl_array_release(&keys);
        } else {
            wl_keyboard_send_leave(k, ser, s->res);
        }
    }
}

void WlComp::configureTask(WlTask* t, int w, int h) {
    if (!t || !t->surf || !t->surf->roleRes) return;
    if (t->cfgW == w && t->cfgH == h) return;
    t->cfgW = w;
    t->cfgH = h;
    sendTopCfg(t->surf, w, h, active == t);
}

void WlComp::setActive(WlTask* t, bool on) {
    if (on) {
        if (active == t) return;
        if (active) setActive(active, false);
        active = t;
        sendTopCfg(t->surf, t->cfgW, t->cfgH, true);
        kbdFocus = t->surf;
        kbdTo(this, kbdFocus, true);
        return;
    }
    if (t && t->surf) sendTopCfg(t->surf, t->cfgW, t->cfgH, false);
    if (kbdFocus) {
        kbdTo(this, kbdFocus, false);
        kbdFocus = nullptr;
    }
    if (active == t) active = nullptr;
}

// the input grab parks the pointer on the grabbed task's toplevel: the
// synthetic cursor lands at buffer centre and every event aims there
// until the grab drops
void WlComp::setGrab(WlTask* t) {
    if (grabTask == t) return;
    if (pointerFocus) {
        ptrLeave(this, pointerFocus);
        pointerFocus = nullptr;
    }
    grabTask = t;
    if (!t || !t->surf) return;
    grabX = t->bufW * 0.5f;
    grabY = t->bufH * 0.5f;
    ptrTo(this, t->surf, nextSerial(), grabX, grabY);
    pointerFocus = t->surf;
    setActive(t, true);
}

void WlComp::grabMotion(float dx, float dy) {
    if (!grabTask || !grabTask->surf) return;
    grabX += dx;
    grabY += dy;
    if (grabX < 0.0f) grabX = 0.0f;
    if (grabY < 0.0f) grabY = 0.0f;
    if (grabX > grabTask->bufW) grabX = grabTask->bufW;
    if (grabY > grabTask->bufH) grabY = grabTask->bufH;
    wl_client* cl = wl_resource_get_client(grabTask->surf->res);
    const uint32_t ms = nowMs();
    for (wl_resource* p : in->seats[cl].pointers) {
        wl_pointer_send_motion(p, ms, wl_fixed_from_double(grabX),
                               wl_fixed_from_double(grabY));
        if (wl_resource_get_version(p) >= 5) wl_pointer_send_frame(p);
    }
}

void WlComp::grabButton(uint32_t evdevBtn, bool down) {
    if (!grabTask || !grabTask->surf) return;
    wl_client* cl = wl_resource_get_client(grabTask->surf->res);
    for (wl_resource* p : in->seats[cl].pointers) {
        wl_pointer_send_button(p, nextSerial(), nowMs(), evdevBtn,
            down ? WL_POINTER_BUTTON_STATE_PRESSED
                 : WL_POINTER_BUTTON_STATE_RELEASED);
        if (wl_resource_get_version(p) >= 5) wl_pointer_send_frame(p);
    }
}

void WlComp::grabAxis(float yoff) {
    if (!grabTask || !grabTask->surf) return;
    wl_client* cl = wl_resource_get_client(grabTask->surf->res);
    const uint32_t ms = nowMs();
    // wheel-up arrives positive from the host; wayland counts that as
    // negative scroll, one detent reading 15 surface units
    const wl_fixed_t v = wl_fixed_from_double(-yoff * 15.0);
    for (wl_resource* p : in->seats[cl].pointers) {
        if (wl_resource_get_version(p) >= 5)
            wl_pointer_send_axis_source(p, WL_POINTER_AXIS_SOURCE_WHEEL);
        wl_pointer_send_axis(p, ms, WL_POINTER_AXIS_VERTICAL_SCROLL, v);
        if (wl_resource_get_version(p) >= 5) wl_pointer_send_frame(p);
    }
}

void WlComp::keyEvent(uint32_t evdev, bool down) {
    if (in->kstate) {
        xkb_state_update_key(in->kstate, evdev + 8,
            down ? XKB_KEY_DOWN : XKB_KEY_UP);
        const uint32_t dep = xkb_state_serialize_mods(in->kstate,
            XKB_STATE_MODS_DEPRESSED);
        const uint32_t lat = xkb_state_serialize_mods(in->kstate,
            XKB_STATE_MODS_LATCHED);
        const uint32_t lck = xkb_state_serialize_mods(in->kstate,
            XKB_STATE_MODS_LOCKED);
        const uint32_t grp = xkb_state_serialize_layout(in->kstate,
            XKB_STATE_LAYOUT_EFFECTIVE);
        if (dep != in->depressed || lat != in->latched ||
                lck != in->locked || grp != in->group) {
            in->depressed = dep; in->latched = lat;
            in->locked = lck; in->group = grp;
            if (kbdFocus) {
                wl_client* cl = wl_resource_get_client(kbdFocus->res);
                for (wl_resource* k : in->seats[cl].keyboards)
                    wl_keyboard_send_modifiers(k, nextSerial(), dep,
                                               lat, lck, grp);
            }
        }
    }
    if (!kbdFocus) return;
    wl_client* cl = wl_resource_get_client(kbdFocus->res);
    const uint32_t ser = nextSerial();
    for (wl_resource* k : in->seats[cl].keyboards)
        wl_keyboard_send_key(k, ser, nowMs(), evdev,
            down ? WL_KEYBOARD_KEY_STATE_PRESSED
                 : WL_KEYBOARD_KEY_STATE_RELEASED);
}

void WlComp::requestClose(WlTask* t) {
    if (t && t->surf && t->surf->roleRes)
        xdg_toplevel_send_close(t->surf->roleRes);
}
