#include "kbd.h"

#include "../common/config.h"
#include "../panels/layout.h"

#include <cmath>

int kbdHostIndex(const std::vector<Panel>& panels, int hostDisp) {
    for (int i = 0; i < (int)panels.size(); ++i)
        if (panels[i].displayId == hostDisp && !panels[i].minimized)
            return i;
    for (int i = 0; i < (int)panels.size(); ++i)
        if (!panels[i].minimized) return i;
    return -1;
}

void kbdCenter(const Panel& p, const float origin[3], float offYaw,
               float offY, float c[3], float r[3], float up[3]) {
    float pc[3];
    panelCenter(p, origin, pc, r, up);
    // the quad is its own window: it keeps the host's yaw and up, drops
    // under the host's bottom edge, then pulls toward the viewer until it
    // sits at kKbdDist - closer than the dock, so the strip can never
    // cover the lower rows
    float n[3] = {origin[0] - pc[0], origin[1] - pc[1], origin[2] - pc[2]};
    const float nl = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    const float inv = nl > 1e-6f ? 1.0f / nl : 0.0f;
    const float pull = nl > kKbdDist ? nl - kKbdDist : 0.0f;
    const float drop = kPanelH * 0.5f + kKbdGap + kKbdHH;
    for (int i = 0; i < 3; ++i)
        c[i] = pc[i] - up[i] * drop + n[i] * inv * pull;
    // the pill's placement: swinging the frame around the viewer's vertical
    // axis moves the quad like a ring drag moves its host, and offY slides
    // it along its own up
    if (offYaw != 0.0f) {
        const float cs = cosf(offYaw), sn = sinf(offYaw);
        float* v3[3] = {c, r, up};
        for (int i = 0; i < 3; ++i) {
            const float x = v3[i][0] * cs - v3[i][2] * sn;
            v3[i][2] = v3[i][0] * sn + v3[i][2] * cs;
            v3[i][0] = x;
        }
    }
    c[0] += up[0] * offY; c[1] += up[1] * offY; c[2] += up[2] * offY;
}

void kbdFreeCenter(float yaw, const float origin[3], float offYaw,
                   float offY, float c[3], float r[3], float up[3]) {
    Panel ghost;
    ghost.yaw = yaw;
    ghost.pitch = 0.0f;
    kbdCenter(ghost, origin, offYaw, offY, c, r, up);
}

void kbdFrame(const std::vector<Panel>& panels, int hostDisp, bool free,
              float freeYaw, float offYaw, float offY,
              const float origin[3], float c[3], float r[3], float up[3]) {
    if (!free) {
        const int hi = kbdHostIndex(panels, hostDisp);
        if (hi >= 0) {
            kbdCenter(panels[hi], origin, offYaw, offY, c, r, up);
            return;
        }
    }
    kbdFreeCenter(freeYaw, origin, offYaw, offY, c, r, up);
}



void kbdHitPx(float u, float v, float* x, float* y) {
    *x = (u * 0.5f + 0.5f) * kKbdW;
    *y = (0.5f - v * 0.5f) * kKbdH;
}
