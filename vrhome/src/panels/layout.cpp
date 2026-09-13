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

float pillHalfWidth(float textW, float winHW) {
    const float cap = winHW - kBarInset;
    float w = textW * 0.5f + kPillPadX;
    // a pill is wider than it is tall: floor at 2:1 so a missing label
    // still leaves a readable lozenge instead of a dot
    if (w < kBarH) w = kBarH;
    return w < cap ? w : cap;
}

float pillTextLimit(float winHW) {
    return (winHW - kBarInset - kPillPadX) * 2.0f;
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

Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head) {
    float d[3];
    gazeDir(head, d);
    Pick pick;
    float bestT = 1e9f;
    for (int i = 0; i < (int)panels.size(); ++i) {
        const Panel& p = panels[i];
        float c[3], r[3];
        panelCenter(p, c, r);
        // plane normal toward origin
        float n[3] = {-c[0], 0, -c[2]};
        const float nl = sqrtf(n[0]*n[0] + n[2]*n[2]);
        n[0] /= nl; n[2] /= nl;
        // normal points at the viewer, ray travels into the plane: d.n < 0
        const float dn = d[0]*n[0] + d[2]*n[2];
        if (dn > -1e-5f) continue;
        const float t = (c[0]*n[0] + c[1]*n[1] + c[2]*n[2]) / dn;
        if (t <= 0 || t >= bestT) continue;
        const float px = d[0]*t - c[0], py = d[1]*t - c[1], pz = d[2]*t - c[2];
        const float u = (px*r[0] + pz*r[2]) / (kPanelW / 2);
        const float v = py / (kPanelH / 2);
        if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) continue;
        bestT = t;
        pick.idx = i;
        pick.u = u; pick.v = v;
    }
    return pick;
}
