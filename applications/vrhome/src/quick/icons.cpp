#include "icons.h"

#include "../common/config.h"
#include "../common/palette.h"
#include "../hud/engine.h"
#include "../render/shape.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>

// Tile glyphs drawn out of shapeQuad primitives, the same way the dock's
// wifi fan and battery are built. Everything is expressed inside a local
// square of half-size s centred on ic - strokes get their own quad rotated
// in-plane so diagonals stay straight on the card.

// a straight stroke from (x0,y0) to (x1,y1) in icon-local metres, w its
// half-width
static void seg(HudEngine* e, const Mat4& vp, const float ic[3],
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

// a disc (border 0) or ring (border>0) centred on an icon-local offset
static void disc(HudEngine* e, const Mat4& vp, const float ic[3],
                 const float r[3], const float up[3],
                 float x, float y, float rad, float border,
                 const float col[4]) {
    const float dc[3] = {ic[0] + r[0] * x + up[0] * y,
                         ic[1] + r[1] * x + up[1] * y,
                         ic[2] + r[2] * x + up[2] * y};
    shapeQuad(e, vp, dc, r, up, 0.008f, 0.0f, rad + border + 0.004f,
              rad + border + 0.004f, rad, rad, rad, border, 0.0018f, col);
}

// a wedge of a ring opening upward from an icon-local centre; rot steers
// the opening (0 = up)
static void arc(HudEngine* e, const Mat4& vp, const float ic[3],
                const float r[3], const float up[3],
                float x, float y, float rad, float border, float rot,
                const float col[4]) {
    const float ac[3] = {ic[0] + r[0] * x + up[0] * y,
                         ic[1] + r[1] * x + up[1] * y,
                         ic[2] + r[2] * x + up[2] * y};
    shapeQuad(e, vp, ac, r, up, 0.008f, rot, rad + border, rad + border,
              rad, rad, rad, border, 0.0018f, col, -1.0f, 1.15f);
}

// an outlined rounded box in icon-local metres
static void box(HudEngine* e, const Mat4& vp, const float ic[3],
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

// four corner brackets, the crop_free/focus frame
static void corners(HudEngine* e, const Mat4& vp, const float ic[3],
                    const float r[3], const float up[3], float s,
                    float w, const float col[4]) {
    for (int q = 0; q < 4; ++q) {
        const float sx = (q & 1) ? 1.0f : -1.0f;
        const float sy = (q & 2) ? 1.0f : -1.0f;
        seg(e, vp, ic, r, up, sx * (s - 0.30f * s), sy * s, sx * s, sy * s,
            w, col);
        seg(e, vp, ic, r, up, sx * s, sy * (s - 0.30f * s), sx * s, sy * s,
            w, col);
    }
}

void quickIcon(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s,
               const float col[4], const float back[4]) {
    const float w = s * 0.11f;
    switch (id) {
    case QS_WIFI:
        arc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 1.02f, w, 0.0f, col);
        arc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.68f, w, 0.0f, col);
        arc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.34f, w, 0.0f, col);
        disc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.12f, 0.0f, col);
        break;
    case QS_BOUNDARY:
        corners(e, vp, ic, r, up, s * 0.88f, w * 1.15f, col);
        break;
    case QS_BLUETOOTH: {
        // the rune: a vertical stroke carrying two right-pointing wedges
        seg(e, vp, ic, r, up, 0.0f, -0.85f * s, 0.0f, 0.85f * s, w, col);
        seg(e, vp, ic, r, up, 0.0f, 0.82f * s, 0.42f * s, 0.42f * s, w, col);
        seg(e, vp, ic, r, up, 0.42f * s, 0.42f * s, 0.0f, 0.02f * s, w, col);
        seg(e, vp, ic, r, up, 0.0f, -0.02f * s, 0.42f * s, -0.42f * s, w, col);
        seg(e, vp, ic, r, up, 0.42f * s, -0.42f * s, 0.0f, -0.82f * s, w, col);
        break;
    }
    case QS_SEETHROUGH:
        // eye: two arcs meeting at the corners, a pupil inside
        arc(e, vp, ic, r, up, 0.0f, -0.62f * s, s * 1.05f, w, 0.0f, col);
        arc(e, vp, ic, r, up, 0.0f, 0.62f * s, s * 1.05f, w, 3.141593f, col);
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.26f, 0.0f, col);
        break;
    case QS_MIC:
        box(e, vp, ic, r, up, 0.0f, 0.18f * s, 0.24f * s, 0.46f * s,
            0.0f, col);
        seg(e, vp, ic, r, up, 0.0f, -0.30f * s, 0.0f, -0.60f * s, w, col);
        seg(e, vp, ic, r, up, -0.28f * s, -0.60f * s, 0.28f * s,
            -0.60f * s, w, col);
        arc(e, vp, ic, r, up, 0.0f, 0.0f, 0.44f * s, w, 3.141593f, col);
        break;
    case QS_RESET_VIEW:
        // recenter: a ring with a gap top-right and an arrowhead on it
        arc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.72f, w, 2.2f, col);
        seg(e, vp, ic, r, up, 0.30f * s, 0.85f * s, 0.78f * s,
            0.50f * s, w * 1.1f, col);
        seg(e, vp, ic, r, up, 0.80f * s, 0.88f * s, 0.78f * s,
            0.50f * s, w * 1.1f, col);
        break;
    case QS_NIGHT:
        // crescent: a filled moon with a card-coloured bite
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.74f, 0.0f, col);
        disc(e, vp, ic, r, up, 0.38f * s, 0.18f * s, s * 0.64f, 0.0f, back);
        break;
    case QS_DND:
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.78f, w, col);
        seg(e, vp, ic, r, up, -0.45f * s, 0.0f, 0.45f * s, 0.0f, w, col);
        break;
    case QS_AIRPLANE:
        // tilted 45 degrees so it reads as a plane climbing, not a plus
        seg(e, vp, ic, r, up, -0.58f * s, -0.58f * s, 0.58f * s,
            0.58f * s, w * 1.2f, col);
        seg(e, vp, ic, r, up, -0.44f * s, 0.58f * s, 0.58f * s,
            -0.44f * s, w, col);
        seg(e, vp, ic, r, up, -0.66f * s, -0.21f * s, -0.21f * s,
            -0.66f * s, w, col);
        break;
    case QS_SAVER:
        // battery body with a mid fill and the tip nub
        box(e, vp, ic, r, up, -0.06f * s, 0.0f, 0.66f * s, 0.40f * s,
            w, col);
        box(e, vp, ic, r, up, -0.24f * s, 0.0f, 0.24f * s, 0.26f * s,
            0.0f, col);
        seg(e, vp, ic, r, up, 0.72f * s, -0.16f * s, 0.72f * s, 0.16f * s,
            w, col);
        break;
    case QS_CENTER:
        corners(e, vp, ic, r, up, s * 0.78f, w, col);
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.16f, 0.0f, col);
        break;
    case QS_REPORT:
        seg(e, vp, ic, r, up, -0.42f * s, -0.82f * s, -0.42f * s,
            0.82f * s, w, col);
        box(e, vp, ic, r, up, 0.10f * s, 0.48f * s, 0.44f * s, 0.26f * s,
            0.0f, col);
        break;
    case QS_ABOUT:
        disc(e, vp, ic, r, up, 0.0f, 0.52f * s, s * 0.14f, 0.0f, col);
        seg(e, vp, ic, r, up, 0.0f, -0.70f * s, 0.0f, 0.22f * s,
            w * 1.4f, col);
        break;
    default:
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.5f, w, col);
        break;
    }
}

void quickSliderIcon(HudEngine* e, const Mat4& vp, int slider,
                     const float ic[3], const float r[3],
                     const float up[3], float s, const float col[4]) {
    const float w = s * 0.12f;
    if (slider == QSLIDER_BRIGHTNESS) {
        // sun: a disc with rays
        disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.34f, 0.0f, col);
        for (int i = 0; i < 8; ++i) {
            const float a = i * 0.785398f;
            seg(e, vp, ic, r, up, cosf(a) * 0.52f * s, sinf(a) * 0.52f * s,
                cosf(a) * 0.82f * s, sinf(a) * 0.82f * s, w * 0.7f, col);
        }
    } else {
        // speaker: body box, cone strokes, a pair of right-opening arcs
        box(e, vp, ic, r, up, -0.44f * s, 0.0f, 0.16f * s, 0.22f * s,
            0.0f, col);
        seg(e, vp, ic, r, up, -0.34f * s, 0.16f * s, 0.02f * s, 0.46f * s,
            w, col);
        seg(e, vp, ic, r, up, -0.34f * s, -0.16f * s, 0.02f * s,
            -0.46f * s, w, col);
        seg(e, vp, ic, r, up, 0.02f * s, -0.46f * s, 0.02f * s, 0.46f * s,
            w, col);
        arc(e, vp, ic, r, up, 0.06f * s, 0.0f, 0.34f * s, w * 0.8f,
            -1.570796f, col);
        arc(e, vp, ic, r, up, 0.06f * s, 0.0f, 0.62f * s, w * 0.8f,
            -1.570796f, col);
    }
}

void quickGearIcon(HudEngine* e, const Mat4& vp, const float ic[3],
                   const float r[3], const float up[3], float s,
                   const float col[4]) {
    // gear: a ring with teeth, the hub left open
    disc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.58f, s * 0.13f, col);
    for (int i = 0; i < 8; ++i) {
        const float a = i * 0.785398f;
        seg(e, vp, ic, r, up, cosf(a) * 0.58f * s, sinf(a) * 0.58f * s,
            cosf(a) * 0.88f * s, sinf(a) * 0.88f * s, s * 0.11f, col);
    }
}
