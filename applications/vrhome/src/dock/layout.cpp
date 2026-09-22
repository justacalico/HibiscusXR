#include "layout.h"

#include "../common/config.h"
#include "../math/head.h"
#include "../panels/layout.h"

#include <cmath>

void dockCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]) {
    ringPoint(yaw, pitch, kDockDist, 0.0f, origin, c, r, up);
}

float dockPitchFor(float headPitch) {
    const float p = headPitch * kDockPitchScale - kDockPitchDrop;
    return p < kDockPitchMin ? kDockPitchMin
         : p > kDockPitchMax ? kDockPitchMax : p;
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
    DockItem q;
    q.kind = DK_QUICK;
    q.pkg = kQuickPanelPkg;
    q.sep = !out.empty();
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

float dockHandleDrop() {
    return kDockBarH * 0.5f + kHandleGap + kHandleT;
}

bool onDockHandle(float u, float v, float halfW) {
    const float x = u * halfW, y = v * (kDockBarH * 0.5f);
    return fabsf(x) <= kHandleW + kHandlePad &&
           fabsf(y + dockHandleDrop()) <= kHandleT + kHandlePad;
}

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

bool rayDock(float yaw, float pitch, const float origin[3],
             const float o[3], const float d[3], float halfW,
             float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    dockCenter(yaw, pitch, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, halfW, kDockBarH * 0.5f,
                   u, v, t);
}

DockPick pickDockRay(const std::vector<DockItem>& items, float halfW,
                     float yaw, float pitch, const float origin[3],
                     const float o[3], const float d[3]) {
    DockPick pk;
    if (items.empty() || halfW <= 0.0f) return pk;
    float u, v, t;
    if (!rayDock(yaw, pitch, origin, o, d, halfW, &u, &v, &t)) return pk;
    // the move handle hangs under the strip: it lives outside the bar box
    // so it checks before the in-bar bounds
    if (onDockHandle(u, v, halfW)) {
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
                  float yaw, float pitch, const Mat4& head,
                  const float origin[3], const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickDockRay(items, halfW, yaw, pitch, origin, o, d);
}

bool dockPinnable(const DockItem& it) {
    return it.kind != DK_QUICK;
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
