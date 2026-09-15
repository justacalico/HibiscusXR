#include "layout.h"

#include "../common/config.h"
#include "../math/head.h"

#include <cmath>

void panelCenter(const Panel& p, float out[3], float right[3]) {
    out[0] = sinf(p.yaw) * kPanelDist;
    out[1] = kPanelY;
    out[2] = -cosf(p.yaw) * kPanelDist;
    // fwd = (-sin,0,cos) toward origin; right = cross(up, fwd) so the panel's
    // right edge lands on the viewer's right
    right[0] = cosf(p.yaw); right[1] = 0; right[2] = sinf(p.yaw);
}

float freeSlotYaw(const std::vector<Panel>& panels, float centre) {
    bool used[kMaxPanels] = {};
    for (auto& p : panels) {
        for (int s = 0; s < kMaxPanels; ++s) {
            const float d = wrapPi(p.yaw - (centre + kSlotYaw[s]));
            if (fabsf(d) < 0.05f) used[s] = true;
        }
    }
    for (int s = 0; s < kMaxPanels; ++s)
        if (!used[s]) return centre + kSlotYaw[s];
    return centre;
}

int evictIndex(const std::vector<Panel>& panels) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].pkg != kLibraryPkg) return i;
    return -1;
}

float pillHalfWidth(float textW, float winHW, bool btns) {
    const float cap = winHW - kBarInset;
    float w = textW * 0.5f + kPillPadX + (btns ? kPillBtnW : 0.0f);
    // a pill is wider than it is tall: floor at 2:1 so a missing label
    // still leaves a readable lozenge instead of a dot
    if (w < kBarH) w = kBarH;
    return w < cap ? w : cap;
}

float pillTextLimit(float winHW, bool btns) {
    return (winHW - kBarInset - kPillPadX - (btns ? kPillBtnW : 0.0f)) * 2.0f;
}

float pillCloseX(float pillHW) {
    return pillHW - kPillBtnPad - kPillBtnR;
}

float pillMinX(float pillHW) {
    return pillCloseX(pillHW) - kPillBtnGap - 2.0f * kPillBtnR;
}

// panel-coord point -> world offset from the pill's centre, which hangs
// centred under the window's bottom edge
static void pillLocal(float u, float v, float* x, float* y) {
    *x = u * (kPanelW * 0.5f);
    *y = v * (kPanelH * 0.5f) + kPanelH * 0.5f + kBarGap + kBarH * 0.5f;
}

bool onPill(float u, float v, float pillHW) {
    float x, y;
    pillLocal(u, v, &x, &y);
    return fabsf(x) <= pillHW && fabsf(y) <= kBarH * 0.5f;
}

int pillButtonAt(float u, float v, float pillHW) {
    float x, y;
    pillLocal(u, v, &x, &y);
    // square hit area a touch bigger than the disc: gaze aim is coarse
    const float r = kPillBtnR + 0.008f;
    if (fabsf(x - pillCloseX(pillHW)) <= r && fabsf(y) <= r)
        return ZONE_CLOSE;
    if (fabsf(x - pillMinX(pillHW)) <= r && fabsf(y) <= r)
        return ZONE_MIN;
    return ZONE_LABEL;
}

float handleDrop() {
    return kPanelH * 0.5f + kBarGap + kBarH + kHandleGap + kHandleT;
}

// panel-coord point -> world offset from the handle's centre, which hangs
// centred under the pill
static void handleLocal(float u, float v, float* x, float* y) {
    *x = u * (kPanelW * 0.5f);
    *y = v * (kPanelH * 0.5f) + handleDrop();
}

bool onHandle(float u, float v) {
    float x, y;
    handleLocal(u, v, &x, &y);
    return fabsf(x) <= kHandleW + kHandlePad &&
           fabsf(y) <= kHandleT + kHandlePad;
}

int middleIndex(const std::vector<Panel>& panels) {
    int best = -1;
    float bestSum = 1e9f;
    for (int i = 0; i < (int)panels.size(); ++i) {
        if (panels[i].minimized) continue;
        float sum = 0.0f;
        for (int j = 0; j < (int)panels.size(); ++j) {
            if (panels[j].minimized) continue;
            sum += fabsf(wrapPi(panels[i].yaw - panels[j].yaw));
        }
        if (sum < bestSum) { bestSum = sum; best = i; }
    }
    return best;
}

void grabRing(std::vector<Panel>& panels) {
    for (auto& p : panels) p.grabYaw = p.yaw;
}

void dragRing(std::vector<Panel>& panels, float delta) {
    for (auto& p : panels) p.yaw = wrapPi(p.grabYaw + delta);
}

int minimizedIndex(const std::vector<Panel>& panels, const std::string& pkg) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].minimized && panels[i].pkg == pkg) return i;
    return -1;
}

void recenterSlots(std::vector<Panel>& panels, float centre) {
    for (auto& p : panels) {
        // keep the panel's slot offset, re-centre the ring on current gaze
        const float off = wrapPi(p.yaw - centre);
        // find nearest slot offset and snap to it around the new centre
        float best = 1e9f; int bs = 0;
        for (int s = 0; s < kMaxPanels; ++s) {
            const float d = fabsf(off - kSlotYaw[s]);
            if (d < best) { best = d; bs = s; }
        }
        p.yaw = centre + kSlotYaw[bs];
    }
}

bool rayPanel(const Panel& p, const float d[3], float* u, float* v,
              float* t) {
    float c[3], r[3];
    panelCenter(p, c, r);
    // plane normal toward origin
    float n[3] = {-c[0], 0, -c[2]};
    const float nl = sqrtf(n[0]*n[0] + n[2]*n[2]);
    n[0] /= nl; n[2] /= nl;
    // normal points at the viewer, ray travels into the plane: d.n < 0
    const float dn = d[0]*n[0] + d[2]*n[2];
    if (dn > -1e-5f) return false;
    const float t0 = (c[0]*n[0] + c[1]*n[1] + c[2]*n[2]) / dn;
    if (t0 <= 0) return false;
    const float px = d[0]*t0 - c[0], py = d[1]*t0 - c[1], pz = d[2]*t0 - c[2];
    *u = (px*r[0] + pz*r[2]) / (kPanelW / 2);
    *v = py / (kPanelH / 2);
    if (t) *t = t0;
    return true;
}

Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head) {
    float d[3];
    gazeDir(head, d);
    Pick pick;
    float bestT = 1e9f;
    const int mid = middleIndex(panels);
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        if (p.minimized) continue;
        float u, v, t;
        if (!rayPanel(p, d, &u, &v, &t)) continue;
        if (t >= bestT) continue;
        int zone = ZONE_NONE;
        if (fabsf(u) <= 1.0f && fabsf(v) <= 1.0f) {
            zone = ZONE_WINDOW;
        } else {
            // pillHW is filled in by the renderer once the label is
            // measured; before that the empty-pill floor still hits
            const float phw = p.pillHW > 0.0f ? p.pillHW
                    : pillHalfWidth(0.0f, kPanelW * 0.5f,
                                    p.pkg != kLibraryPkg);
            if (onPill(u, v, phw))
                zone = p.pkg == kLibraryPkg ? ZONE_LABEL
                                            : pillButtonAt(u, v, phw);
            else if (i == mid && onHandle(u, v))
                zone = ZONE_HANDLE;
        }
        if (zone == ZONE_NONE) continue;
        bestT = t;
        pick.idx = i;
        pick.u = u; pick.v = v; pick.zone = zone;
    }
    return pick;
}

bool dragPoint(const Panel& p, const Mat4& head, float* px, float* py) {
    float d[3], u, v;
    gazeDir(head, d);
    if (!rayPanel(p, d, &u, &v, nullptr)) return false;
    // a held drag follows the gaze even past the window's edge
    u = u < -1.0f ? -1.0f : u > 1.0f ? 1.0f : u;
    v = v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v;
    *px = (u * 0.5f + 0.5f) * kVdW;
    *py = (0.5f - v * 0.5f) * kVdH;
    return true;
}

float dragBoost(float anchor, float p, float max) {
    const float b = anchor + (p - anchor) * kDragGain;
    return b < 0.0f ? 0.0f : b > max ? max : b;
}
