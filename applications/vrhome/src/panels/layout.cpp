#include "layout.h"

#include "../common/config.h"
#include "../math/head.h"
#include "../pill/pill.h"

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

float panelHW(const Panel& p) { return kPanelW * 0.5f * p.scale; }
float panelHH(const Panel& p) { return kPanelH * 0.5f * p.scale; }

float freeSlotYaw(const std::vector<Panel>& panels, float centre) {
    // a slot counts as taken when any panel sits within a window's angular
    // width of it, not only when it sits exactly on it: a floating window
    // parked between the slots blocks both rather than being stacked on
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
    int n = 0;
    for (auto& p : panels) offs[n++] = wrapPi(p.yaw - centre);
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
    for (auto& p : panels)
        if (!p.floating) return p.pitch;
    return 0.0f;
}

int evictIndex(const std::vector<Panel>& panels) {
    // a docked window goes first; only once every window floats does a
    // hand-placed one get reclaimed
    for (int i = 0; i < (int)panels.size(); ++i)
        if (!panels[i].floating) return i;
    return panels.empty() ? -1 : 0;
}

float dockSlotYaw(const std::vector<Panel>& panels, int self, float centre) {
    // nearest free slot to the window's current yaw; everything else -
    // docked or floating - blocks a slot it overlaps
    float best = 0.0f, bestD = 1e9f;
    bool found = false;
    for (int s = 0; s < kMaxPanels; ++s) {
        const float cand = centre + kSlotYaw[s];
        bool used = false;
        for (int i = 0; i < (int)panels.size(); ++i) {
            if (i == self) continue;
            if (fabsf(wrapPi(panels[i].yaw - cand)) < kPanelMinGap) {
                used = true;
                break;
            }
        }
        if (used) continue;
        const float d = fabsf(wrapPi(panels[self].yaw - cand));
        if (!found || d < bestD) { best = cand; bestD = d; found = true; }
    }
    return found ? best : freeSlotYaw(panels, centre);
}

void recenterSlots(std::vector<Panel>& panels, float centre, float pitch,
                   float prevCentre) {
    const float ep = pitch > kPitchMax ? kPitchMax
                     : pitch < -kPitchMax ? -kPitchMax : pitch;
    const float dyaw = wrapPi(centre - prevCentre);
    // windows keep their left-to-right order around the new centre but each
    // gets its own slot - snapping every panel to its nearest slot can
    // collapse two windows onto the same one
    const float asc[kMaxPanels] = {kSlotYaw[1], kSlotYaw[0], kSlotYaw[2]};
    int ord[kMaxPanels];
    int n = 0;
    for (int i = 0; i < (int)panels.size(); ++i) {
        Panel& p = panels[i];
        if (p.floating) {
            // carries with the dash: same yaw shift, own pitch kept
            p.yaw = wrapPi(p.yaw + dyaw);
            continue;
        }
        ord[n++] = i;
    }
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

float barFloatX(float winHW) {
    return barMinX(winHW) - kBarBtnGap - 2.0f * kBarBtnR;
}

// panel-coord point -> world offset from the bar's centre. The bar sits
// flush on the window's top edge: its bottom edge is v=1 exactly, nothing
// of it covers the app surface
static void barLocal(float u, float v, float hw, float hh,
                     float* x, float* y) {
    *x = u * hw;
    *y = v * hh - hh - kBarH * 0.5f;
}

bool onBar(float u, float v, float hw, float hh) {
    float x, y;
    barLocal(u, v, hw, hh, &x, &y);
    return fabsf(x) <= hw && fabsf(y) <= kBarH * 0.5f;
}

int barButtonAt(float u, float v, float hw, float hh) {
    float x, y;
    barLocal(u, v, hw, hh, &x, &y);
    // square hit area a touch bigger than the disc: gaze aim is coarse
    const float r = kBarBtnR + 0.008f;
    if (fabsf(x - barCloseX(hw)) <= r && fabsf(y) <= r)
        return ZONE_CLOSE;
    if (fabsf(x - barMinX(hw)) <= r && fabsf(y) <= r)
        return ZONE_MIN;
    if (fabsf(x - barFloatX(hw)) <= r && fabsf(y) <= r)
        return ZONE_FLOAT;
    return ZONE_LABEL;
}

bool onResizeGrip(float u, float v, float hw, float hh) {
    // the grip rides the window's bottom-right corner; the hit circle is
    // generous past the edge since the corner itself is a single point
    const float dx = u * hw - hw, dy = v * hh + hh;
    return dx * dx + dy * dy <= kResizeR * kResizeR;
}

float resizeScale(float grabScale, float grabR, float r) {
    if (grabR < 1e-4f) return grabScale;
    const float s = grabScale * (r / grabR);
    return s < kScaleMin ? kScaleMin : s > kScaleMax ? kScaleMax : s;
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

int panelIndex(const std::vector<Panel>& panels, const std::string& pkg) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].pkg == pkg) return i;
    return -1;
}

int minimizedIndex(const std::vector<Panel>& panels, const std::string& pkg) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].minimized && panels[i].pkg == pkg) return i;
    return -1;
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
    return rayQuad(c, r, up, origin, o, d, panelHW(p), panelHH(p),
                   u, v, t);
}

Pick pickPanelRay(const std::vector<Panel>& panels, const float origin[3],
                  const float o[3], const float d[3]) {
    Pick pick;
    float bestT = 1e9f;
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        if (p.minimized) continue;
        const float hw = panelHW(p), hh = panelHH(p);
        float u, v, t;
        if (!rayPanel(p, origin, o, d, &u, &v, &t)) continue;
        if (t >= bestT) continue;
        int zone = ZONE_NONE;
        // the bar's bottom edge is the window's top edge: the bar check
        // runs first so a ray landing on that shared edge picks chrome,
        // never a tap on the app surface under it
        if (onBar(u, v, hw, hh)) {
            zone = barButtonAt(u, v, hw, hh);
        } else if (onResizeGrip(u, v, hw, hh)) {
            zone = ZONE_RESIZE;
        } else if (p.floating && onMovePill(u, v, hw, hh)) {
            // floating windows carry the shared move pill under the quad
            zone = ZONE_PILL;
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
