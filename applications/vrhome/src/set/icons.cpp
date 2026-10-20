#include "icons.h"

#include "item.h"
#include "../hud/engine.h"
#include "../render/icons.h"
#include "../render/iprim.h"
#include "../render/shape.h"

#include <GLES2/gl2.h>

#include <cmath>

// the SetSec -> Material Symbols map; drawGlyph stamps the atlas cell,
// this file only picks which glyph each section shows

void setSecIcon(HudEngine* e, const Mat4& vp, int sec,
                const float ic[3], const float r[3], const float up[3],
                float s, const float col[4]) {
    int g = -1;
    switch (sec) {
    case SEC_WIFI:   g = GI_WIFI; break;
    case SEC_BT:     g = GI_BLUETOOTH; break;
    case SEC_CTRL:   g = GI_STADIA_CONTROLLER; break;
    case SEC_DISP:   g = GI_MONITOR; break;
    case SEC_ENV:    g = GI_LANDSCAPE; break;
    case SEC_SOUND:  g = GI_VOLUME_UP; break;
    case SEC_LANG:   g = GI_LANGUAGE; break;
    case SEC_TIME:   g = GI_SCHEDULE; break;
    case SEC_IME:    g = GI_KEYBOARD; break;
    case SEC_DEV:    g = GI_TERMINAL; break;
    case SEC_ABOUT:  g = GI_INFO; break;
    }
    if (g >= 0) drawGlyph(e, vp, g, ic, r, up, s, 0.008f, col);
}

void setGlyph(HudEngine* e, const Mat4& vp, int glyph,
              const float ic[3], const float r[3], const float up[3],
              float s, const float col[4]) {
    int g = -1;
    switch (glyph) {
    case SG_CHEVRON: g = GI_CHEVRON_RIGHT; break;
    case SG_LOCK:    g = GI_LOCK; break;
    case SG_CHECK:   g = GI_CHECK; break;
    case SG_TRASH:   g = GI_DELETE; break;
    case SG_REFRESH: g = GI_REFRESH; break;
    }
    if (g >= 0) drawGlyph(e, vp, g, ic, r, up, s, 0.008f, col);
}

void setBrandMark(HudEngine* e, const Mat4& vp, const float ic[3],
                  const float r[3], const float up[3], float s,
                  float alpha) {
    const float tile[4] = {0.086f, 0.047f, 0.137f, 0.95f * alpha};
    shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, s, s, s, s, 0.22f * s,
              0.0f, 0.0018f, tile);
    const float orb[4] = {0.37f, 0.91f, 1.0f, 0.35f * alpha};
    iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, 0.78f * s, 0.024f * s, orb);
    const float pet[4] = {1.0f, 0.31f, 0.55f, 0.95f * alpha};
    for (int i = 0; i < 5; ++i) {
        const float a = (90.0f + i * 72.0f) * 0.0174533f;
        iconSeg(e, vp, ic, r, up, cosf(a) * 0.10f * s, sinf(a) * 0.10f * s,
                cosf(a) * 0.58f * s, sinf(a) * 0.58f * s, 0.17f * s, pet);
    }
    const float thr[4] = {0.35f, 0.02f, 0.17f, 0.9f * alpha};
    iconDisc(e, vp, ic, r, up, 0.0f, 0.0f, 0.14f * s, 0.0f, thr);
    const float stm[4] = {0.94f, 0.79f, 0.47f, 0.95f * alpha};
    iconSeg(e, vp, ic, r, up, 0.12f * s, 0.20f * s, 0.40f * s, 0.44f * s,
            0.045f * s, stm);
    iconDisc(e, vp, ic, r, up, 0.44f * s, 0.48f * s, 0.085f * s, 0.0f,
             stm);
    const float halo[4] = {0.37f, 0.91f, 1.0f, 0.22f * alpha};
    const float pl[4] = {0.37f, 0.91f, 1.0f, 0.9f * alpha};
    iconDisc(e, vp, ic, r, up, 0.78f * s, 0.0f, 0.14f * s, 0.0f, halo);
    iconDisc(e, vp, ic, r, up, 0.78f * s, 0.0f, 0.06f * s, 0.0f, pl);
}
