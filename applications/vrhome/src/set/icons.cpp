#include "icons.h"

#include "item.h"
#include "../hud/engine.h"
#include "../render/iprim.h"

#include <GLES2/gl2.h>

#include <cmath>

// Section and row glyphs out of the shared icon primitives, the same
// visual recipe as the quick panel's tile icons.

void setSecIcon(HudEngine* e, const Mat4& vp, int sec,
                const float ic[3], const float r[3], const float up[3],
                float s, const float col[4]) {
    const float w = s * 0.11f;
    switch (sec) {
    case SEC_WIFI:
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 1.02f, w, 0.0f, col);
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.68f, w, 0.0f, col);
        iconArc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.34f, w, 0.0f, col);
        iconDisc(e, vp, ic, r, up, 0.0f, -0.42f * s, s * 0.12f, 0.0f, col);
        break;
    case SEC_BT:
        iconSeg(e, vp, ic, r, up, 0.0f, -0.85f * s, 0.0f, 0.85f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, 0.82f * s, 0.42f * s, 0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.42f * s, 0.42f * s, 0.0f, 0.02f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.02f * s, 0.42f * s, -0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.42f * s, -0.42f * s, 0.0f, -0.82f * s, w, col);
        break;
    case SEC_CTRL:
        // gamepad: a wide body with a stick dot and a button pair
        iconBox(e, vp, ic, r, up, 0.0f, -0.05f * s, 0.78f * s, 0.42f * s,
                w, col);
        iconDisc(e, vp, ic, r, up, -0.42f * s, -0.05f * s, 0.13f * s,
                 0.0f, col);
        iconSeg(e, vp, ic, r, up, 0.34f * s, 0.02f * s, 0.52f * s,
                0.02f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.43f * s, -0.07f * s, 0.43f * s,
                0.11f * s, w, col);
        break;
    case SEC_DISP:
        // monitor: screen box over a stem
        iconBox(e, vp, ic, r, up, 0.0f, 0.12f * s, 0.72f * s, 0.48f * s,
                w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.40f * s, 0.0f, -0.62f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.30f * s, -0.70f * s, 0.30f * s,
                -0.70f * s, w, col);
        break;
    case SEC_ENV:
        // landscape: frame with a horizon and a sun
        iconBox(e, vp, ic, r, up, 0.0f, 0.0f, 0.72f * s, 0.52f * s, w, col);
        iconDisc(e, vp, ic, r, up, -0.34f * s, 0.22f * s, 0.10f * s,
                 0.0f, col);
        iconSeg(e, vp, ic, r, up, -0.55f * s, -0.30f * s, -0.10f * s,
                0.0f, w, col);
        iconSeg(e, vp, ic, r, up, -0.10f * s, 0.0f, 0.30f * s, -0.30f * s,
                w, col);
        iconSeg(e, vp, ic, r, up, 0.30f * s, -0.30f * s, 0.55f * s,
                -0.30f * s, w, col);
        break;
    case SEC_SOUND:
        // speaker + one wave
        iconSeg(e, vp, ic, r, up, -0.70f * s, -0.16f * s, -0.70f * s,
                0.16f * s, w * 1.4f, col);
        iconSeg(e, vp, ic, r, up, -0.70f * s, 0.16f * s, -0.30f * s,
                0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.30f * s, 0.42f * s, -0.30f * s,
                -0.42f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.30f * s, -0.42f * s, -0.70f * s,
                -0.16f * s, w, col);
        iconArc(e, vp, ic, r, up, -0.05f * s, 0.0f, s * 0.42f, w,
                3.141593f * 0.5f, col);
        break;
    case SEC_LANG:
        // globe: ring, equator and a meridian
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.62f, w, col);
        iconSeg(e, vp, ic, r, up, -0.62f * s, 0.0f, 0.62f * s, 0.0f, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, -0.62f * s, 0.0f, 0.62f * s, w, col);
        break;
    case SEC_TIME:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.62f, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, 0.0f, 0.0f, 0.36f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.0f, 0.0f, 0.26f * s, -0.20f * s, w, col);
        break;
    case SEC_IME:
        // keyboard: box with two key rows
        iconBox(e, vp, ic, r, up, 0.0f, 0.0f, 0.72f * s, 0.44f * s, w, col);
        for (int i = 0; i < 3; ++i)
            iconSeg(e, vp, ic, r, up, (-0.42f + i * 0.30f) * s, 0.14f * s,
                    (-0.20f + i * 0.30f) * s, 0.14f * s, w, col);
        for (int i = 0; i < 3; ++i)
            iconSeg(e, vp, ic, r, up, (-0.42f + i * 0.30f) * s, -0.10f * s,
                    (-0.20f + i * 0.30f) * s, -0.10f * s, w, col);
        iconSeg(e, vp, ic, r, up, -0.30f * s, -0.30f * s, 0.30f * s,
                -0.30f * s, w, col);
        break;
    case SEC_DEV:
        // terminal prompt
        iconSeg(e, vp, ic, r, up, -0.55f * s, 0.40f * s, -0.10f * s, 0.0f,
                w * 1.3f, col);
        iconSeg(e, vp, ic, r, up, -0.10f * s, 0.0f, -0.55f * s, -0.40f * s,
                w * 1.3f, col);
        iconSeg(e, vp, ic, r, up, 0.10f * s, -0.45f * s, 0.60f * s,
                -0.45f * s, w * 1.3f, col);
        break;
    case SEC_ABOUT:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.62f, w, col);
        iconDisc(e, vp, ic, r, up, 0.0f, 0.24f * s, s * 0.07f, 0.0f, col);
        iconSeg(e, vp, ic, r, up, 0.0f, 0.02f * s, 0.0f, -0.38f * s,
                w * 1.4f, col);
        break;
    default:
        iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.3f, w, col);
        break;
    }
}

void setGlyph(HudEngine* e, const Mat4& vp, int glyph,
              const float ic[3], const float r[3], const float up[3],
              float s, const float col[4]) {
    const float w = s * 0.14f;
    switch (glyph) {
    case SG_CHEVRON:
        iconSeg(e, vp, ic, r, up, -0.30f * s, 0.60f * s, 0.30f * s, 0.0f,
                w, col);
        iconSeg(e, vp, ic, r, up, 0.30f * s, 0.0f, -0.30f * s, -0.60f * s,
                w, col);
        break;
    case SG_LOCK:
        iconBox(e, vp, ic, r, up, 0.0f, -0.18f * s, 0.42f * s, 0.34f * s,
                w * 0.8f, col);
        iconArc(e, vp, ic, r, up, 0.0f, 0.10f * s, s * 0.30f, w,
                3.141593f, col);
        break;
    case SG_CHECK:
        iconSeg(e, vp, ic, r, up, -0.55f * s, 0.0f, -0.12f * s, -0.40f * s,
                w * 1.2f, col);
        iconSeg(e, vp, ic, r, up, -0.12f * s, -0.40f * s, 0.58f * s,
                0.42f * s, w * 1.2f, col);
        break;
    case SG_TRASH:
        iconSeg(e, vp, ic, r, up, -0.42f * s, 0.48f * s, 0.42f * s,
                0.48f * s, w, col);
        iconBox(e, vp, ic, r, up, 0.0f, -0.18f * s, 0.34f * s, 0.40f * s,
                w * 0.8f, col);
        iconSeg(e, vp, ic, r, up, -0.12f * s, 0.58f * s, 0.12f * s,
                0.58f * s, w, col);
        break;
    case SG_REFRESH:
        iconArc(e, vp, ic, r, up, 0.0f, 0.0f, s * 0.56f, w,
                3.141593f * 0.4f, col);
        iconSeg(e, vp, ic, r, up, 0.30f * s, 0.62f * s, 0.56f * s,
                0.62f * s, w, col);
        iconSeg(e, vp, ic, r, up, 0.56f * s, 0.62f * s, 0.56f * s,
                0.36f * s, w, col);
        break;
    case SG_BATT:
        iconBox(e, vp, ic, r, up, -0.05f * s, 0.0f, 0.55f * s, 0.30f * s,
                w * 0.8f, col);
        iconBox(e, vp, ic, r, up, 0.62f * s, 0.0f, 0.08f * s, 0.16f * s,
                0.0f, col);
        break;
    default:
        break;
    }
}
