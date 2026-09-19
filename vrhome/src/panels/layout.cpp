#include "layout.h"

#include "../common/config.h"
#include "../math/head.h"

#include <cmath>

void panelCenter(const Panel& p, const float origin[3], float out[3],
                 float right[3], float up[3]) {
    // a cylinder around the anchor, not a sphere: pitch raises the panel
    // without squeezing the ring's horizontal spread, so three windows
    // can't bunch at the zenith
    out[0] = origin[0] + sinf(p.yaw) * kPanelDist;
    out[1] = origin[1] + kPanelY + sinf(p.pitch) * kPanelDist;
    out[2] = origin[2] - cosf(p.yaw) * kPanelDist;
    right[0] = cosf(p.yaw); right[1] = 0; right[2] = sinf(p.yaw);
    // the plane's normal points back at the anchor; up = normal x right
    // tilts the top edge toward you as the ring rises, like a ceiling screen
    const float nx = origin[0] - out[0], ny = origin[1] - out[1],
                nz = origin[2] - out[2];
    const float l = sqrtf(nx*nx + ny*ny + nz*nz);
    const float n[3] = {nx/l, ny/l, nz/l};
    up[0] = n[1]*right[2] - n[2]*right[1];
    up[1] = n[2]*right[0] - n[0]*right[2];
    up[2] = n[0]*right[1] - n[1]*right[0];
}

float freeSlotYaw(const std::vector<Panel>& panels, float centre) {
    // a slot counts as taken when any panel sits within a window's angular
    // width of it, not only when it sits exactly on it: the centre is the
    // current gaze, so a panel spawned under an earlier gaze can hide
    // between the new slot positions and the next window lands on top of it
    for (int s = 0; s < kMaxPanels; ++s) {
        const float cand = centre + kSlotYaw[s];
        bool used = false;
        for (auto& p : panels)
            if (fabsf(wrapPi(p.yaw - cand)) < kPanelMinGap) {
                used = true;
                break;
            }
        if (!used) return cand;
    }
    // every slot blocked: drop into the middle of the widest free arc
    // rather than stacking windows
    float offs[kMaxPanels];
    const int n = (int)panels.size();
    for (int i = 0; i < n; ++i) offs[i] = wrapPi(panels[i].yaw - centre);
    for (int i = 1; i < n; ++i) {
        const float t = offs[i];
        int j = i - 1;
        while (j >= 0 && offs[j] > t) { offs[j + 1] = offs[j]; --j; }
        offs[j + 1] = t;
    }
    float bestOff = 0.0f, bestGap = -1.0f;
    for (int i = 0; i <= n; ++i) {
        const float lo = i == 0 ? -(float)M_PI : offs[i - 1];
        const float hi = i == n ? (float)M_PI : offs[i];
        if (hi - lo > bestGap) { bestGap = hi - lo; bestOff = (lo + hi) * 0.5f; }
    }
    return centre + bestOff;
}

float ringPitch(const std::vector<Panel>& panels) {
    return panels.empty() ? 0.0f : panels.front().pitch;
}

int evictIndex(const std::vector<Panel>& panels) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].pkg != kLibraryPkg) return i;
    return -1;
}

int libraryIndex(const std::vector<Panel>& panels) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].pkg == kLibraryPkg) return i;
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
    for (auto& p : panels) { p.grabYaw = p.yaw; p.grabPitch = p.pitch; }
}

void dragRing(std::vector<Panel>& panels, float dYaw, float dPitch) {
    for (auto& p : panels) {
        p.yaw = wrapPi(p.grabYaw + dYaw);
        const float np = p.grabPitch + dPitch;
        p.pitch = np > kPitchMax ? kPitchMax
                  : np < -kPitchMax ? -kPitchMax : np;
    }
}

int minimizedIndex(const std::vector<Panel>& panels, const std::string& pkg) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].minimized && panels[i].pkg == pkg) return i;
    return -1;
}

void recenterSlots(std::vector<Panel>& panels, float centre, float pitch) {
    const float ep = pitch > kPitchMax ? kPitchMax
                     : pitch < -kPitchMax ? -kPitchMax : pitch;
    // windows keep their left-to-right order around the new centre but each
    // gets its own slot - snapping every panel to its nearest slot can
    // collapse two windows onto the same one
    const float asc[kMaxPanels] = {kSlotYaw[1], kSlotYaw[0], kSlotYaw[2]};
    int ord[kMaxPanels];
    const int n = (int)panels.size();
    for (int i = 0; i < n; ++i) ord[i] = i;
    for (int i = 1; i < n; ++i) {
        const int t = ord[i];
        int j = i - 1;
        while (j >= 0 && wrapPi(panels[ord[j]].yaw - centre) >
                         wrapPi(panels[t].yaw - centre)) {
            ord[j + 1] = ord[j];
            --j;
        }
        ord[j + 1] = t;
    }
    // fewer panels than slots sit on the middle-most run, so a lone window
    // lands dead ahead instead of off to the side
    const int start = (kMaxPanels - n) / 2;
    for (int i = 0; i < n; ++i) {
        panels[ord[i]].yaw = centre + asc[start + i];
        panels[ord[i]].pitch = ep;
    }
}

bool rayPanel(const Panel& p, const float origin[3], const float o[3],
              const float d[3], float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    panelCenter(p, origin, c, r, up);
    // the plane's normal points at the anchor, tilted with pitch
    const float nx = origin[0] - c[0], ny = origin[1] - c[1],
                nz = origin[2] - c[2];
    const float nl = sqrtf(nx*nx + ny*ny + nz*nz);
    const float n[3] = {nx/nl, ny/nl, nz/nl};
    // normal points at the viewer, ray travels into the plane: d.n < 0
    const float dn = d[0]*n[0] + d[1]*n[1] + d[2]*n[2];
    if (dn > -1e-5f) return false;
    const float t0 = ((c[0]-o[0])*n[0] + (c[1]-o[1])*n[1] +
                      (c[2]-o[2])*n[2]) / dn;
    if (t0 <= 0) return false;
    const float px = o[0] + d[0]*t0 - c[0], py = o[1] + d[1]*t0 - c[1],
                pz = o[2] + d[2]*t0 - c[2];
    *u = (px*r[0] + pz*r[2]) / (kPanelW / 2);
    *v = (px*up[0] + py*up[1] + pz*up[2]) / (kPanelH / 2);
    if (t) *t = t0;
    return true;
}

Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head,
               const float origin[3], const float o[3]) {
    float d[3];
    gazeDir(head, d);
    Pick pick;
    float bestT = 1e9f;
    const int mid = middleIndex(panels);
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        if (p.minimized) continue;
        float u, v, t;
        if (!rayPanel(p, origin, o, d, &u, &v, &t)) continue;
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

bool dragPoint(const Panel& p, const Mat4& head, const float origin[3],
               const float o[3], float* px, float* py) {
    float d[3], u, v;
    gazeDir(head, d);
    if (!rayPanel(p, origin, o, d, &u, &v, nullptr)) return false;
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
