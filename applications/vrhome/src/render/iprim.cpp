#include "iprim.h"

#include "../hud/engine.h"
#include "shape.h"

#include <GLES2/gl2.h>

#include <cmath>

void iconSeg(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x0, float y0, float x1, float y1, float w,
             const float col[4]) {
    const float mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
    const float dx = x1 - x0, dy = y1 - y0;
    const float len = sqrtf(dx * dx + dy * dy) * 0.5f;
    const float sc[3] = {ic[0] + r[0] * mx + up[0] * my,
                         ic[1] + r[1] * mx + up[1] * my,
                         ic[2] + r[2] * mx + up[2] * my};
    shapeQuad(e, vp, sc, r, up, 0.008f, atan2f(dy, dx), len + w, w,
              len, w, w, 0.0f, 0.0015f, col);
}

void iconDisc(HudEngine* e, const Mat4& vp, const float ic[3],
              const float r[3], const float up[3],
              float x, float y, float rad, float border,
              const float col[4]) {
    const float dc[3] = {ic[0] + r[0] * x + up[0] * y,
                         ic[1] + r[1] * x + up[1] * y,
                         ic[2] + r[2] * x + up[2] * y};
    shapeQuad(e, vp, dc, r, up, 0.008f, 0.0f, rad + border + 0.004f,
              rad + border + 0.004f, rad, rad, rad, border, 0.0018f, col);
}

void iconArc(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x, float y, float rad, float border, float rot,
             const float col[4]) {
    const float ac[3] = {ic[0] + r[0] * x + up[0] * y,
                         ic[1] + r[1] * x + up[1] * y,
                         ic[2] + r[2] * x + up[2] * y};
    shapeQuad(e, vp, ac, r, up, 0.008f, rot, rad + border, rad + border,
              rad, rad, rad, border, 0.0018f, col, -1.0f, 1.15f);
}

void iconBox(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x, float y, float hw, float hh, float border,
             const float col[4]) {
    const float bc[3] = {ic[0] + r[0] * x + up[0] * y,
                         ic[1] + r[1] * x + up[1] * y,
                         ic[2] + r[2] * x + up[2] * y};
    shapeQuad(e, vp, bc, r, up, 0.008f, 0.0f, hw + border + 0.003f,
              hh + border + 0.003f, hw, hh, hh > hw ? hw : hh, border,
              0.0016f, col);
}

void iconCorners(HudEngine* e, const Mat4& vp, const float ic[3],
                 const float r[3], const float up[3], float s,
                 float w, const float col[4]) {
    for (int q = 0; q < 4; ++q) {
        const float sx = (q & 1) ? 1.0f : -1.0f;
        const float sy = (q & 2) ? 1.0f : -1.0f;
        iconSeg(e, vp, ic, r, up, sx * (s - 0.30f * s), sy * s,
                sx * s, sy * s, w, col);
        iconSeg(e, vp, ic, r, up, sx * s, sy * (s - 0.30f * s),
                sx * s, sy * s, w, col);
    }
}
