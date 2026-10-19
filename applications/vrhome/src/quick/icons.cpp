#include "icons.h"

#include "../common/config.h"
#include "../common/palette.h"
#include "../hud/engine.h"
#include "../render/iprim.h"
#include "../render/shape.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>

// Tile glyphs drawn out of shapeQuad primitives, the same way the dock's
// wifi fan and battery are built. Everything is expressed inside a local
// square of half-size s centred on ic - strokes get their own quad rotated
// in-plane so diagonals stay straight on the card.

void quickIcon(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s,
               const float col[4], const float back[4]) {
    const float w = s * 0.11f;
    switch (id) {
    case QS_WIFI:
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 1.02f, w, 0.0f, col);
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.68f, w, 0.0f, col);
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.34f, w, 0.0f, col);
        iconDisc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.12f, 0.0f, col);
        break;
    case QS_BOUNDARY:
        iconCorners(e, vp, ic, r, up, s * 0.88f, w * 1.15f, col);
        break;
    case QS_BLUETOOTH: {
        // the rune: a vertical stroke carrying two right-pointing wedges
        iconSeg(e, vp, ic, r, up, 0.0f, -0.85f * s, 0.0f, 0.85f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, 0.82f * s, 0.42f * s, 0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.42f * s, 0.42f * s, 0.0f, 0.02f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.02f * s, 0.42f * s, -0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.42f * s, -0.42f * s, 0.0f, -0.82f * s, w, col);
        break;
    }
    case QS_SEETHROUGH:
        // eye: two arcs meeting at the corners, a pupil inside
        iconArc(e, vp, ic, r, up, 0.0f, -0.62f * s, s * 1.05f, w, 0.0f, col);
        iconArc(e, vp, ic, r, up, 0.0f, 0.62f * s, s * 1.05f, w, 3.141593f, col);
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.26f, 0.0f, col);
        break;
    case QS_MIC:
        iconBox(e, vp, ic, r, up, 0.0f, 0.18f * s, 0.24f * s, 0.46f * s,
            0.0f, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.30f * s, 0.0f, -0.60f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.28f * s, -0.60f * s, 0.28f * s,
            -0.60f * s, w, col);
        iconArc(e, vp, ic, r, up, 0.0f, 0.0f, 0.44f * s, w, 3.141593f, col);
        break;
    case QS_RESET_VIEW:
        // recenter: a ring with a gap top-right and an arrowhead on it
        iconArc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.72f, w, 2.2f, col);
        iconSeg(e, vp, ic, r, up, 0.30f * s, 0.85f * s, 0.78f * s,
            0.50f * s, w * 1.1f, col);
        iconSeg(e, vp, ic, r, up, 0.80f * s, 0.88f * s, 0.78f * s,
            0.50f * s, w * 1.1f, col);
        break;
    case QS_NIGHT:
        // crescent: a filled moon with a card-coloured bite
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.74f, 0.0f, col);
        iconDisc(e, vp, ic, r, up, 0.38f * s, 0.18f * s, s * 0.64f, 0.0f, back);
        break;
    case QS_DND:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.78f, w, col);
        iconSeg(e, vp, ic, r, up, -0.45f * s, 0.0f, 0.45f * s, 0.0f, w, col);
        break;
    case QS_AIRPLANE:
        // tilted 45 degrees so it reads as a plane climbing, not a plus
        iconSeg(e, vp, ic, r, up, -0.58f * s, -0.58f * s, 0.58f * s,
            0.58f * s, w * 1.2f, col);
        iconSeg(e, vp, ic, r, up, -0.44f * s, 0.58f * s, 0.58f * s,
            -0.44f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.66f * s, -0.21f * s, -0.21f * s,
            -0.66f * s, w, col);
        break;
    case QS_SAVER:
        // battery body with a mid fill and the tip nub
        iconBox(e, vp, ic, r, up, -0.06f * s, 0.0f, 0.66f * s, 0.40f * s,
            w, col);
        iconBox(e, vp, ic, r, up, -0.24f * s, 0.0f, 0.24f * s, 0.26f * s,
            0.0f, col);
        iconSeg(e, vp, ic, r, up, 0.72f * s, -0.16f * s, 0.72f * s, 0.16f * s,
            w, col);
        break;
    case QS_REPORT:
        iconSeg(e, vp, ic, r, up, -0.42f * s, -0.82f * s, -0.42f * s,
            0.82f * s, w, col);
        iconBox(e, vp, ic, r, up, 0.10f * s, 0.48f * s, 0.44f * s, 0.26f * s,
            0.0f, col);
        break;
    case QS_ABOUT:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.52f * s, s * 0.14f, 0.0f, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.70f * s, 0.0f, 0.22f * s,
            w * 1.4f, col);
        break;
    default:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.5f, w, col);
        break;
    }
}

void quickSliderIcon(HudEngine* e, const Mat4& vp, int slider,
                     const float ic[3], const float r[3],
                     const float up[3], float s, const float col[4]) {
    const float w = s * 0.12f;
    if (slider == QSLIDER_BRIGHTNESS) {
        // sun: a disc with rays
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.34f, 0.0f, col);
        for (int i = 0; i < 8; ++i) {
            const float a = i * 0.785398f;
            iconSeg(e, vp, ic, r, up, cosf(a) * 0.52f * s, sinf(a) * 0.52f * s,
                cosf(a) * 0.82f * s, sinf(a) * 0.82f * s, w * 0.7f, col);
        }
    } else {
        // speaker: body box, cone strokes, a pair of right-opening arcs
        iconBox(e, vp, ic, r, up, -0.44f * s, 0.0f, 0.16f * s, 0.22f * s,
            0.0f, col);
        iconSeg(e, vp, ic, r, up, -0.34f * s, 0.16f * s, 0.02f * s, 0.46f * s,
            w, col);
        iconSeg(e, vp, ic, r, up, -0.34f * s, -0.16f * s, 0.02f * s,
            -0.46f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.02f * s, -0.46f * s, 0.02f * s, 0.46f * s,
            w, col);
        iconArc(e, vp, ic, r, up, 0.06f * s, 0.0f, 0.34f * s, w * 0.8f,
            -1.570796f, col);
        iconArc(e, vp, ic, r, up, 0.06f * s, 0.0f, 0.62f * s, w * 0.8f,
            -1.570796f, col);
    }
}

void quickGearIcon(HudEngine* e, const Mat4& vp, const float ic[3],
                   const float r[3], const float up[3], float s,
                   const float col[4]) {
    // gear: a ring with teeth, the hub left open
    iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.58f, s * 0.13f, col);
    for (int i = 0; i < 8; ++i) {
        const float a = i * 0.785398f;
        iconSeg(e, vp, ic, r, up, cosf(a) * 0.58f * s, sinf(a) * 0.58f * s,
            cosf(a) * 0.88f * s, sinf(a) * 0.88f * s, s * 0.11f, col);
    }
}
