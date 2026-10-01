#include "layout.h"

#include "../anim/anim.h"
#include "../common/config.h"
#include "../math/head.h"
#include "../panels/layout.h"
#include "../pill/pill.h"

#include <cmath>

void dockCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]) {
    ringPoint(yaw, pitch, kDockDist, 0.0f, origin, c, r, up);
}

void dockCenterDrop(float yaw, float pitch, float drop,
                    const float origin[3], float c[3], float r[3],
                    float up[3]) {
    dockCenter(yaw, pitch, origin, c, r, up);
    for (int i = 0; i < 3; ++i) c[i] -= up[i] * drop;
}

float dockPitchFor(float headPitch) {
    const float p = headPitch * kDockPitchScale - kDockPitchDrop;
    return p < kDockPitchMin ? kDockPitchMin
         : p > kDockPitchMax ? kDockPitchMax : p;
}

float ringPitchFor(float dockPitch) {
    const float p = dockPitch + kRingLift;
    return p > kPitchMax ? kPitchMax : p < -kPitchMax ? -kPitchMax : p;
}

float dashRingPitch(const std::vector<Panel>& panels, float dockPitch) {
    for (auto& p : panels)
        if (!p.floating) return p.pitch;
    return ringPitchFor(dockPitch);
}

float dockDragPitch(float grabPitch, float ringGrabPitch, float ringPitch) {
    return grabPitch + (ringPitch - ringGrabPitch);
}

static int findPanel(const std::vector<Panel>& panels,
                     const std::string& pkg) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].pkg == pkg) return i;
    return -1;
}

static int findXr(const std::vector<XrTask>& xr, const std::string& pkg) {
    for (int i = 0; i < (int)xr.size(); ++i)
        if (xr[i].pkg == pkg) return i;
    return -1;
}

static bool inPins(const std::vector<std::string>& pins,
                   const std::string& pkg) {
    for (auto& p : pins)
        if (p == pkg) return true;
    return false;
}

std::vector<DockItem> buildDock(const std::vector<std::string>& pins,
                                const std::vector<Panel>& panels,
                                const std::vector<XrTask>& xr) {
    std::vector<DockItem> out;
    for (auto& pkg : pins) {
        // the quick-panel app has its own permanent slot on the end; a pin
        // for it would draw the same icon twice
        if (pkg == kQuickPanelPkg) continue;
        DockItem it;
        it.kind = DK_PIN;
        it.pkg = pkg;
        const int pi = findPanel(panels, pkg);
        const int xi = findXr(xr, pkg);
        if (pi >= 0) {
            it.running = true;
            it.panelIdx = pi;
            it.taskId = panels[pi].taskId;
            it.minimized = panels[pi].minimized;
        }
        if (xi >= 0) {
            it.running = true;
            it.taskId = xr[xi].taskId;
            it.vr = true;
        }
        out.push_back(it);
    }
    // live tasks that aren't pinned fill the running section; a pkg shows
    // once - a running pin carries the dot instead of a duplicate icon,
    // and the quick-panel app lights its permanent slot instead
    bool runSep = false;
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        if (p.pkg.empty() || inPins(pins, p.pkg) ||
                p.pkg == kQuickPanelPkg) continue;
        DockItem it;
        it.kind = DK_RUN;
        it.pkg = p.pkg;
        it.running = true;
        it.panelIdx = i;
        it.taskId = p.taskId;
        it.minimized = p.minimized;
        it.sep = !runSep && !out.empty();
        runSep = true;
        out.push_back(it);
    }
    for (auto& t : xr) {
        if (t.pkg.empty() || inPins(pins, t.pkg)) continue;
        DockItem it;
        it.kind = DK_RUN;
        it.pkg = t.pkg;
        it.running = true;
        it.taskId = t.taskId;
        it.vr = true;
        it.sep = !runSep && !out.empty();
        runSep = true;
        out.push_back(it);
    }
    // the app-grid button: the library lives inside the dash, its icon is a
    // permanent slot beside quick settings. It carries no pkg so it never
    // resolves through the icon cache
    DockItem g;
    g.kind = DK_GRID;
    g.label = "Library";
    g.sep = !out.empty();
    out.push_back(g);

    DockItem q;
    q.kind = DK_QUICK;
    q.pkg = kQuickPanelPkg;
    q.sep = false;
    // a running quick-panel task rides its own button rather than adding a
    // second icon to the strip
    const int qi = findPanel(panels, kQuickPanelPkg);
    if (qi >= 0) {
        q.running = true;
        q.panelIdx = qi;
        q.taskId = panels[qi].taskId;
        q.minimized = panels[qi].minimized;
    }
    out.push_back(q);
    return out;
}

float dockLayout(std::vector<DockItem>& items, DockStatus& st) {
    // status cluster on the left in two pills: clock + battery + wifi in
    // the first, the bell alone in the second, then a separator gap
    // before the app icons like the group seps use
    const float pillAW = kSysPillPad * 2.0f + st.clockW + kSysIconW * 2.0f +
                         kSysGap * 2.0f;
    const float pillBW = kSysPillPad * 2.0f + kSysIconW;
    const float clusterW = pillAW + kSysPillGap + pillBW;
    const float lead = items.empty() ? clusterW
                                     : clusterW + kDockGap + kDockSepW;
    float total = 2.0f * kDockPad + lead;
    for (auto& it : items)
        total += kDockIconW + (it.sep ? kDockSepW : 0.0f);
    if (!items.empty()) total += (items.size() - 1) * kDockGap;
    const float halfW = total * 0.5f;
    float x = -halfW + kDockPad;
    st.pillAL = x;
    st.clockX = x + kSysPillPad;
    x = st.clockX + st.clockW + kSysGap;
    st.battX = x + kSysIconW * 0.5f;
    x += kSysIconW + kSysGap;
    st.wifiX = x + kSysIconW * 0.5f;
    x += kSysIconW + kSysPillPad;
    st.pillAR = x;
    x += kSysPillGap;
    st.pillBL = x;
    st.bellX = x + kSysPillPad + kSysIconW * 0.5f;
    x += kSysPillPad + kSysIconW + kSysPillPad;
    st.pillBR = x;
    if (!items.empty()) {
        st.sepX = x + (kDockGap + kDockSepW) * 0.5f;
        x += kDockGap + kDockSepW + kDockIconHW;
    }
    for (auto& it : items) {
        if (it.sep) x += kDockSepW;
        it.x = x;
        x += kDockIconW + kDockGap;
    }
    return halfW;
}

// the close badge hangs off a live immersive icon's top-right corner
static void badgeAt(float itemX, float* bx, float* by) {
    *bx = itemX + kDockIconHW * 0.72f;
    *by = kDockIconY + kDockIconHW * 0.72f;
}

// the move handle hangs under the strip: the shared pill, sized off the
// bar's own half-height

int dockItemAt(const std::vector<DockItem>& items, float halfW,
               float u, float v, int* zone) {
    *zone = DZONE_NONE;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return -1;
    const float x = u * halfW, y = v * (kDockBarH * 0.5f);
    for (int i = 0; i < (int)items.size(); ++i) {
        const DockItem& it = items[i];
        if (it.vr && it.running) {
            float bx, by;
            badgeAt(it.x, &bx, &by);
            const float dx = x - bx, dy = y - by;
            const float r = kDockBadgeR + 0.012f;
            if (dx * dx + dy * dy <= r * r) {
                *zone = DZONE_CLOSE;
                return i;
            }
        }
        const float slack = kDockIconHW + 0.012f;
        if (fabsf(x - it.x) <= slack && fabsf(y - kDockIconY) <= slack) {
            *zone = DZONE_ICON;
            return i;
        }
    }
    return -1;
}

bool rayDock(float yaw, float pitch, float drop, const float origin[3],
             const float o[3], const float d[3], float halfW,
             float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    dockCenterDrop(yaw, pitch, drop, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, halfW, kDockBarH * 0.5f,
                   u, v, t);
}

DockPick pickDockRay(const std::vector<DockItem>& items, float halfW,
                     float yaw, float pitch, float drop,
                     const float origin[3], const float o[3],
                     const float d[3]) {
    DockPick pk;
    if (items.empty() || halfW <= 0.0f) return pk;
    float u, v, t;
    if (!rayDock(yaw, pitch, drop, origin, o, d, halfW, &u, &v, &t))
        return pk;
    // the move handle hangs under the strip: it lives outside the bar box
    // so it checks before the in-bar bounds
    if (onMovePill(u, v, halfW, kDockBarH * 0.5f)) {
        pk.bar = true;
        pk.u = u; pk.v = v; pk.t = t;
        pk.zone = DZONE_HANDLE;
        return pk;
    }
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return pk;
    pk.bar = true;
    pk.u = u; pk.v = v; pk.t = t;
    pk.idx = dockItemAt(items, halfW, u, v, &pk.zone);
    return pk;
}

DockPick pickDock(const std::vector<DockItem>& items, float halfW,
                  float yaw, float pitch, float drop, const Mat4& head,
                  const float origin[3], const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickDockRay(items, halfW, yaw, pitch, drop, origin, o, d);
}

bool dockPinnable(const DockItem& it) {
    return it.kind == DK_PIN || it.kind == DK_RUN;
}

std::vector<std::string> pinToggle(const std::vector<std::string>& pins,
                                   const std::string& pkg) {
    std::vector<std::string> out;
    bool dropped = false;
    for (auto& p : pins) {
        if (p == pkg) { dropped = true; continue; }
        out.push_back(p);
    }
    if (!dropped) out.push_back(pkg);
    return out;
}

// ------------------------------------------------------------- shelf

std::vector<ShelfItem> buildShelf(const std::vector<Panel>& panels) {
    std::vector<ShelfItem> out;
    for (int i = 0; i < (int)panels.size(); ++i) {
        // minimized parks; a restore keeps the slot until the flight lands
        if (!panels[i].minimized && panels[i].minT <= 0.0f) continue;
        ShelfItem it;
        it.panelIdx = i;
        it.pkg = panels[i].pkg;
        out.push_back(it);
    }
    return out;
}

bool shelfXFor(const std::vector<ShelfItem>& items, int panelIdx,
               float* x) {
    for (auto& it : items)
        if (it.panelIdx == panelIdx) { *x = it.x; return true; }
    return false;
}

void carryHover(std::vector<DockItem>& items,
                const std::vector<DockItem>& prev) {
    for (auto& it : items)
        for (auto& p : prev)
            if (p.kind == it.kind && p.pkg == it.pkg) {
                it.hs = p.hs;
                break;
            }
}

void carryShelfHover(std::vector<ShelfItem>& items,
                     const std::vector<ShelfItem>& prev) {
    for (auto& it : items)
        for (auto& p : prev)
            if (p.pkg == it.pkg) { it.hs = p.hs; break; }
}

void tickDockHover(std::vector<DockItem>& items, int hover, float dtMs) {
    for (int i = 0; i < (int)items.size(); ++i)
        items[i].hs = dampMs(items[i].hs,
                             i == hover ? kHoverScale : 1.0f,
                             dtMs, kHoverTauMs);
}

void tickShelfHover(std::vector<ShelfItem>& items, int hover, float dtMs) {
    for (int i = 0; i < (int)items.size(); ++i)
        items[i].hs = dampMs(items[i].hs,
                             i == hover ? kHoverScale : 1.0f,
                             dtMs, kHoverTauMs);
}

float shelfLayout(std::vector<ShelfItem>& items) {
    if (items.empty()) return 0.0f;
    const float inner = items.size() * kShelfIconHW * 2.0f +
                        (items.size() - 1) * kShelfGap;
    const float halfW = inner * 0.5f + kShelfPad;
    float x = -halfW + kShelfPad + kShelfIconHW;
    for (auto& it : items) {
        it.x = x;
        x += kShelfIconHW * 2.0f + kShelfGap;
    }
    return halfW;
}

float shelfLift() {
    return kDockBarH * 0.5f + kShelfGapY + kShelfHH;
}

float shelfTop() {
    return shelfLift() + kShelfHH;
}

void shelfCenter(float yaw, float pitch, const float origin[3],
                 float c[3], float r[3], float up[3]) {
    dockCenter(yaw, pitch, origin, c, r, up);
    for (int i = 0; i < 3; ++i) c[i] += up[i] * shelfLift();
}

void shelfCenterDrop(float yaw, float pitch, float drop,
                     const float origin[3], float c[3], float r[3],
                     float up[3]) {
    shelfCenter(yaw, pitch, origin, c, r, up);
    for (int i = 0; i < 3; ++i) c[i] -= up[i] * drop;
}

int shelfItemAt(const std::vector<ShelfItem>& items, float halfW,
                float u, float v) {
    if (halfW <= 0.0f || fabsf(u) > 1.0f || fabsf(v) > 1.0f) return -1;
    const float x = u * halfW;
    // slack stays under half the icon gap so a point between two icons
    // still lands on the pill body instead of stealing a neighbour's hit
    const float slack = kShelfIconHW + 0.008f;
    for (int i = 0; i < (int)items.size(); ++i)
        if (fabsf(x - items[i].x) <= slack) return i;
    return -1;
}

bool rayShelf(float yaw, float pitch, float drop, const float origin[3],
              const float o[3], const float d[3], float halfW,
              float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    shelfCenterDrop(yaw, pitch, drop, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, halfW, kShelfHH, u, v, t);
}

ShelfPick pickShelfRay(const std::vector<ShelfItem>& items, float halfW,
                       float yaw, float pitch, float drop,
                       const float origin[3], const float o[3],
                       const float d[3]) {
    ShelfPick pk;
    if (items.empty() || halfW <= 0.0f) return pk;
    float u, v, t;
    if (!rayShelf(yaw, pitch, drop, origin, o, d, halfW, &u, &v, &t))
        return pk;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return pk;
    pk.hit = true;
    pk.t = t;
    pk.idx = shelfItemAt(items, halfW, u, v);
    return pk;
}

ShelfPick pickShelf(const std::vector<ShelfItem>& items, float halfW,
                    float yaw, float pitch, float drop, const Mat4& head,
                    const float origin[3], const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickShelfRay(items, halfW, yaw, pitch, drop, origin, o, d);
}

static bool panelLinkOk(int panelIdx, const char* pkg,
                        const std::vector<Panel>& panels) {
    return panelIdx >= 0 && panelIdx < (int)panels.size() &&
           panels[panelIdx].pkg == pkg;
}

DockAction dockActivateAction(const DockItem& it,
                              const std::vector<Panel>& panels) {
    DockAction a;
    if (it.kind == DK_GRID) {
        a.op = DOP_TOGGLE_GRID;
    } else if (it.kind == DK_QUICK) {
        // a live quick panel gets focused like any other running item
        if (panelLinkOk(it.panelIdx, it.pkg.c_str(), panels)) {
            a.op = DOP_FOCUS_PANEL;
            a.panelIdx = it.panelIdx;
            a.taskId = panels[it.panelIdx].taskId;
        } else {
            a.op = DOP_LAUNCH;
            a.pkg = kQuickPanelPkg;
        }
    } else if (it.vr && it.running) {
        a.op = DOP_FOCUS_XR;
        a.taskId = it.taskId;
    } else if (it.panelIdx >= 0 && it.panelIdx < (int)panels.size()) {
        if (panels[it.panelIdx].pkg == it.pkg) {
            a.op = DOP_FOCUS_PANEL;
            a.panelIdx = it.panelIdx;
            a.taskId = panels[it.panelIdx].taskId;
        } else {
            a.op = DOP_LAUNCH;
            a.pkg = it.pkg;
        }
    } else {
        a.op = DOP_LAUNCH;
        a.pkg = it.pkg;
    }
    return a;
}

DockAction dockCloseAction(const DockItem& it,
                           const std::vector<Panel>& panels) {
    DockAction a;
    if (it.panelIdx >= 0 && it.panelIdx < (int)panels.size()) {
        a.op = DOP_CLOSE_PANEL;
        a.panelIdx = it.panelIdx;
        a.displayId = panels[it.panelIdx].displayId;
    } else if (it.taskId >= 0) {
        a.op = DOP_CLOSE_TASK;
        a.taskId = it.taskId;
    }
    return a;
}

DockAction shelfActivateAction(const ShelfItem& it,
                               const std::vector<Panel>& panels) {
    DockAction a;
    if (it.panelIdx < 0 || it.panelIdx >= (int)panels.size()) return a;
    const Panel& p = panels[it.panelIdx];
    if (!p.minimized || p.pkg != it.pkg) return a;
    a.op = DOP_FOCUS_PANEL;
    a.panelIdx = it.panelIdx;
    a.taskId = p.taskId;
    return a;
}
