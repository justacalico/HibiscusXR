#include "layout.h"

#include "../common/config.h"
#include "../math/head.h"

#include <cmath>

void ringPoint(float yaw, float pitch, float dist, float y0,
               const float origin[3], float out[3], float right[3],
               float up[3]) {
    // a cylinder around the anchor, not a sphere: pitch raises the quad
    // without squeezing the ring's horizontal spread, so three windows
    // can't bunch at the zenith
    out[0] = origin[0] + sinf(yaw) * dist;
    out[1] = origin[1] + y0 + sinf(pitch) * dist;
    out[2] = origin[2] - cosf(yaw) * dist;
    right[0] = cosf(yaw); right[1] = 0; right[2] = sinf(yaw);
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

void panelCenter(const Panel& p, const float origin[3], float out[3],
                 float right[3], float up[3]) {
    ringPoint(p.yaw, p.pitch, kPanelDist, kPanelY, origin, out, right, up);
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

float barBtnsW(int n) {
    return n <= 0 ? 0.0f
         : kBarBtnPad + n * 2.0f * kBarBtnR + (n - 1) * kBarBtnGap;
}

float barTextLimit(float winHW, int btns) {
    return 2.0f * (winHW - kBarPadX - barBtnsW(btns));
}

float barCloseX(float winHW) {
    return winHW - kBarBtnPad - kBarBtnR;
}

float barMinX(float winHW) {
    return barCloseX(winHW) - kBarBtnGap - 2.0f * kBarBtnR;
}

// panel-coord point -> world offset from the bar's centre. The bar sits
// flush on the window's top edge: its bottom edge is v=1 exactly, nothing
// of it covers the app surface
static void barLocal(float u, float v, float* x, float* y) {
    *x = u * (kPanelW * 0.5f);
    *y = v * (kPanelH * 0.5f) - kPanelH * 0.5f - kBarH * 0.5f;
}

bool onBar(float u, float v) {
    float x, y;
    barLocal(u, v, &x, &y);
    return fabsf(x) <= kPanelW * 0.5f && fabsf(y) <= kBarH * 0.5f;
}

int barButtonAt(float u, float v) {
    float x, y;
    barLocal(u, v, &x, &y);
    // square hit area a touch bigger than the disc: gaze aim is coarse
    const float r = kBarBtnR + 0.008f;
    const float winHW = kPanelW * 0.5f;
    if (fabsf(x - barCloseX(winHW)) <= r && fabsf(y) <= r)
        return ZONE_CLOSE;
    if (fabsf(x - barMinX(winHW)) <= r && fabsf(y) <= r)
        return ZONE_MIN;
    return ZONE_LABEL;
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

bool rayQuad(const float c[3], const float r[3], const float up[3],
             const float viewer[3], const float o[3], const float d[3],
             float hw, float hh, float* u, float* v, float* t) {
    // the plane's normal points at the viewer, tilted with pitch
    const float nx = viewer[0] - c[0], ny = viewer[1] - c[1],
                nz = viewer[2] - c[2];
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
    *u = (px*r[0] + py*r[1] + pz*r[2]) / hw;
    *v = (px*up[0] + py*up[1] + pz*up[2]) / hh;
    if (t) *t = t0;
    return true;
}

bool rayPanel(const Panel& p, const float origin[3], const float o[3],
              const float d[3], float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    panelCenter(p, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, kPanelW / 2, kPanelH / 2,
                   u, v, t);
}

Pick pickPanelRay(const std::vector<Panel>& panels, const float origin[3],
                  const float o[3], const float d[3]) {
    Pick pick;
    float bestT = 1e9f;
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        if (p.minimized) continue;
        float u, v, t;
        if (!rayPanel(p, origin, o, d, &u, &v, &t)) continue;
        if (t >= bestT) continue;
        int zone = ZONE_NONE;
        // the bar's bottom edge is the window's top edge: the bar check
        // runs first so a ray landing on that shared edge picks chrome,
        // never a tap on the app surface under it
        if (onBar(u, v)) {
            const int z = barButtonAt(u, v);
            // the library closes but never minimizes: where the min disc
            // would sit reads as plain bar on its strip
            zone = (p.pkg == kLibraryPkg && z == ZONE_MIN)
                   ? ZONE_LABEL : z;
        } else if (fabsf(u) <= 1.0f && fabsf(v) <= 1.0f) {
            zone = ZONE_WINDOW;
        }
        if (zone == ZONE_NONE) continue;
        bestT = t;
        pick.idx = i;
        pick.u = u; pick.v = v; pick.zone = zone;
        pick.t = t;
    }
    return pick;
}

Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head,
               const float origin[3], const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickPanelRay(panels, origin, o, d);
}

bool dragPointRay(const Panel& p, const float origin[3], const float o[3],
                  const float d[3], float* px, float* py) {
    float u, v;
    if (!rayPanel(p, origin, o, d, &u, &v, nullptr)) return false;
    // a held drag follows the aim even past the window's edge
    u = u < -1.0f ? -1.0f : u > 1.0f ? 1.0f : u;
    v = v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v;
    *px = (u * 0.5f + 0.5f) * kVdW;
    *py = (0.5f - v * 0.5f) * kVdH;
    return true;
}

bool dragPoint(const Panel& p, const Mat4& head, const float origin[3],
               const float o[3], float* px, float* py) {
    float d[3];
    gazeDir(head, d);
    return dragPointRay(p, origin, o, d, px, py);
}

float dragBoost(float anchor, float p, float max) {
    const float b = anchor + (p - anchor) * kDragGain;
    return b < 0.0f ? 0.0f : b > max ? max : b;
}
