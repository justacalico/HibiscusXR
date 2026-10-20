#include "quick.h"

#include "icons.h"
#include "item.h"
#include "layout.h"
#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/palette.h"
#include "../dock/layout.h"
#include "../hud/engine.h"
#include "../render/shape.h"
#include "../set/set.h"
#include "../set/item.h"
#include "../text/draw.h"
#include "../text/glyphs.h"

#include <GLES2/gl2.h>

#include <cstdio>
#include <cstring>

void syncQuick(HudEngine* e) {
    if (!e->backend) return;
    if (!e->quick.shown && e->quick.openT <= 0.0f) return;
    e->backend->quickStatus(&e->quick.st);
}

void quickActivate(HudEngine* e, int idx) {
    if (idx < 0 || idx >= QS_COUNT) return;
    const int id = quickSpec(idx).id;
    switch (quickActivateOp(id)) {
    case QOP_TOGGLE:
        e->quick.st.toggles ^= (1u << id);
        if (e->backend)
            e->backend->quickToggle(id, quickToggleOn(e->quick.st, id));
        return;
    case QOP_RECENTER:
        wantRecenter();
        return;
    case QOP_SETTINGS:
        setOpen(e, SEC_ABOUT);
        return;
    default:
        return;
    }
}

void quickGear(HudEngine* e) {
    setOpen(e, SEC_WIFI);
}

void quickSliderTo(HudEngine* e, int slider, float u) {
    if (slider < 0 || slider >= QSLIDER_COUNT) return;
    const float v = quickSliderValue(u);
    if (slider == QSLIDER_BRIGHTNESS) e->quick.st.brightness = v;
    else e->quick.st.volume = v;
    if (e->backend) e->backend->quickSetSlider(slider, v);
}

// ------------------------------------------------------------- draw

// uiStrings fallback: what the card says before the first pull lands -
// same English the resources ship
static const char* kQsFallback[UIQS_COUNT] = {
    "Quick settings", "On", "Off", "Not connected",
    "Wi-Fi", "Boundary", "Bluetooth", "Seethrough",
    "Microphone", "Reset view", "Night mode", "Do Not Disturb",
    "Airplane mode", "Battery saver", "Report problem",
    "About device",
};

static const char* qsStr(HudEngine* e, int i) {
    return e->uiQs[i].empty() ? kQsFallback[i] : e->uiQs[i].c_str();
}

// text centred on (x,y) in card-local metres, or left-anchored when
// centred is false; the same shader juggling the grid's labels use
static void cardText(HudEngine* e, const Mat4& vp, const float c[3],
                     const float r[3], const float up[3],
                     float x, float y, const char* s, float ts,
                     float alpha, bool centred, bool dim) {
    if (!e->font.ok || !s || !*s) return;
    float gt, gb, yo = y;
    if (textBounds(e->font.set, s, ts, &gt, &gb)) yo = y - (gt + gb) * 0.5f;
    float xo = x;
    if (centred) xo = x - measureText(e, s, ts) * 0.5f;
    float o[3] = {c[0] + r[0]*xo + up[0]*yo,
                  c[1] + r[1]*xo + up[1]*yo,
                  c[2] + r[2]*xo + up[2]*yo};
    o[0] -= c[0] * 0.010f; o[1] -= c[1] * 0.010f; o[2] -= c[2] * 0.010f;
    const float* tc = dim ? kPalTextDim : kPalText;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                       1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                tc[0], tc[1], tc[2]);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    drawTextPanel(e, s, o, r, up, ts, 0.0f, alpha);
    glUseProgram(e->shapeProg);
}

static void drawSlider(HudEngine* e, const Mat4& vp, const float c[3],
                       const float r[3], const float up[3],
                       int slider, float gs, float ga) {
    const float y = quickSliderY(slider) * gs;
    const float v = slider == QSLIDER_BRIGHTNESS ? e->quick.st.brightness
                                                 : e->quick.st.volume;
    const bool hot = e->quick.slideHeld == slider ||
                     (e->quick.zone == QZONE_SLIDER &&
                      e->quick.slider == slider);
    const float ic[3] = {c[0] + r[0]*(-kQuickHW + kQuickPad +
                                      kQuickIconS) * gs + up[0]*y,
                         c[1] + r[1]*(-kQuickHW + kQuickPad +
                                      kQuickIconS) * gs + up[1]*y,
                         c[2] + r[2]*(-kQuickHW + kQuickPad +
                                      kQuickIconS) * gs + up[2]*y};
    const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           (hot ? 1.0f : 0.80f) * ga};
    quickSliderIcon(e, vp, slider, ic, r, up, kQuickIconS * gs, icol);
    float tl, tr;
    quickSliderTrack(&tl, &tr);
    const float tc[3] = {c[0] + r[0]*(tl + tr) * 0.5f * gs + up[0]*y,
                         c[1] + r[1]*(tl + tr) * 0.5f * gs + up[1]*y,
                         c[2] + r[2]*(tl + tr) * 0.5f * gs + up[2]*y};
    const float trackHW = (tr - tl) * 0.5f * gs;
    const float tcol[4] = {kPalSurfaceHigh[0], kPalSurfaceHigh[1],
                           kPalSurfaceHigh[2], 0.65f * ga};
    shapeQuad(e, vp, tc, r, up, 0.006f, 0.0f, trackHW,
              kQuickTrackHH * gs, trackHW, kQuickTrackHH * gs,
              kQuickTrackHH * gs, 0.0f, 0.0018f, tcol);
    const float fw = (tr - tl) * v;
    if (fw > 0.001f) {
        const float fc[3] = {c[0] + r[0]*(tl + fw * 0.5f) * gs + up[0]*y,
                             c[1] + r[1]*(tl + fw * 0.5f) * gs + up[1]*y,
                             c[2] + r[2]*(tl + fw * 0.5f) * gs + up[2]*y};
        const float fcol[4] = {kPalAccent[0], kPalAccent[1], kPalAccent[2],
                               0.90f * ga};
        const float fhw = fw * 0.5f * gs;
        shapeQuad(e, vp, fc, r, up, 0.007f, 0.0f, fhw,
                  kQuickTrackHH * gs, fhw, kQuickTrackHH * gs,
                  kQuickTrackHH * gs, 0.0f, 0.0018f, fcol);
    }
    const float kx = (tl + (tr - tl) * v) * gs;
    const float kc[3] = {c[0] + r[0]*kx + up[0]*y,
                         c[1] + r[1]*kx + up[1]*y,
                         c[2] + r[2]*kx + up[2]*y};
    const float kcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           (hot ? 1.0f : 0.92f) * ga};
    const float kr = (hot ? kQuickKnobR * 1.15f : kQuickKnobR) * gs;
    shapeQuad(e, vp, kc, r, up, 0.008f, 0.0f, kr, kr, kr, kr, kr,
              0.0f, 0.0018f, kcol);
}

static void drawTile(HudEngine* e, const Mat4& vp, const float c[3],
                     const float r[3], const float up[3],
                     int i, float gs, float ga) {
    const QsSpec& s = quickSpec(i);
    float tx, ty, hw, hh;
    quickTileRect(i, &tx, &ty, &hw, &hh);
    const bool on = s.toggle && quickToggleOn(e->quick.st, s.id);
    const float hs = e->quick.hs[i];
    const float hp = hoverP(hs, kQuickHoverScale);
    const float tcx = tx * gs, tcy = ty * gs;
    const float thw = hw * gs * hs, thh = hh * gs * hs;
    const float tc[3] = {c[0] + r[0]*tcx + up[0]*tcy,
                         c[1] + r[1]*tcx + up[1]*tcy,
                         c[2] + r[2]*tcx + up[2]*tcy};
    float bgA;
    const float* bg;
    if (!s.implemented) { bg = kPalSurface; bgA = 0.22f; }
    else if (on)      { bg = kPalAccent;  bgA = 0.88f + 0.12f * hp; }
    else              { bg = kPalSurface; bgA = 0.55f + 0.20f * hp; }
    const float bgc[4] = {bg[0], bg[1], bg[2], bgA * ga};
    shapeQuad(e, vp, tc, r, up, 0.006f, 0.0f, thw, thh, thw, thh,
              thh * 0.34f, 0.0f, 0.002f, bgc, -1.0f, 0.0f, 0.0f, 0.0f,
              0.0f, 0.06f);
    const float dead = s.implemented ? 1.0f : 0.45f;
    const float* icp = on ? kPalText : kPalTextDim;
    const float icol[4] = {icp[0], icp[1], icp[2],
                           (0.92f + 0.08f * hp) * dead * ga};
    const float* bcp = on ? kPalAccent : kPalSurface;
    const float bgc2[4] = {bcp[0], bcp[1], bcp[2], dead * ga};
    const float is = (s.large ? 0.031f : 0.026f) * gs;
    // icon sits in the tile's upper half for large tiles, above the label
    // for small ones
    const float iconY = s.large ? tcy + thh * 0.38f : tcy + thh * 0.30f;
    const float ic[3] = {c[0] + r[0]*tcx + up[0]*iconY,
                         c[1] + r[1]*tcx + up[1]*iconY,
                         c[2] + r[2]*tcx + up[2]*iconY};
    quickIcon(e, vp, s.id, ic, r, up, is, icol, bgc2);
    float labelY, subY;
    quickTileTextY(s.large, &labelY, &subY);
    const char* label = qsStr(e, UIQS_LABEL0 + s.id);
    cardText(e, vp, c, r, up, tcx, tcy + labelY * thh,
             label, 0.0010f * gs, dead * ga, true, !on);
    if (s.large) {
        const QsSub sub = quickSubtitle(e->quick.st, s.id);
        const char* st_ = sub.kind == QSUB_ON ? qsStr(e, UIQS_ON)
                      : sub.kind == QSUB_OFF ? qsStr(e, UIQS_OFF)
                      : sub.kind == QSUB_NOTCONN ? qsStr(e, UIQS_NOTCONN)
                      : sub.text;
        cardText(e, vp, c, r, up, tcx, tcy + subY * thh, st_,
                 0.00085f * gs, dead * ga, true, true);
    }
}

void drawQuick(HudEngine* e, const Mat4& vp) {
    // openT chases `shown`, so the card is still drawn while it closes
    if (e->quick.openT <= 0.0f) return;
    const float qt = easeOutCubic(e->quick.openT);
    const float gs = kQuickScale0 + (1.0f - kQuickScale0) * qt;
    const float ga = qt;
    float c[3], r[3], up[3];
    quickCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
                c, r, up);
    const float hw = kQuickHW * gs, hh = kQuickHH * gs;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // shadow + card, same glass recipe the grid card uses
    const float shc[3] = {c[0] - up[0] * 0.024f * gs,
                          c[1] - up[1] * 0.024f * gs,
                          c[2] - up[2] * 0.024f * gs};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * ga};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.06f, hh + 0.06f,
              hw, hh, 0.06f, -1.0f, 0.06f, shCol);
    const float cardCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                              0.66f * ga};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0f, 0.003f, cardCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.14f);
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                             0.14f * ga};
    shapeQuad(e, vp, c, r, up, 0.005f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0016f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.30f);

    // title band
    const float hy = quickTitleY() * gs;
    // gear disc + glyph, close disc + x
    {
        const bool ghov = e->quick.zone == QZONE_GEAR;
        const float gx = quickGearX() * gs;
        const float gc[3] = {c[0] + r[0]*gx + up[0]*hy,
                             c[1] + r[1]*gx + up[1]*hy,
                             c[2] + r[2]*gx + up[2]*hy};
        const float gbg[4] = {kPalText[0], kPalText[1], kPalText[2],
                              (ghov ? 0.30f : 0.13f) * ga};
        shapeQuad(e, vp, gc, r, up, 0.006f, 0.0f, kQuickGearR * gs,
                  kQuickGearR * gs, kQuickGearR * gs, kQuickGearR * gs,
                  kQuickGearR * gs, 0.0f, 0.002f, gbg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.92f * ga};
        quickGearIcon(e, vp, gc, r, up, kQuickGearR * 0.72f * gs, icol);
    }
    {
        const bool bhov = e->quick.zone == QZONE_CLOSE;
        const float bx = quickCloseX() * gs;
        const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                             c[1] + r[1]*bx + up[1]*hy,
                             c[2] + r[2]*bx + up[2]*hy};
        const float* bgp = bhov ? kPalDanger : kPalText;
        const float bg[4] = {bgp[0], bgp[1], bgp[2],
                             (bhov ? 0.32f : 0.13f) * ga};
        shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, kQuickCloseR * gs,
                  kQuickCloseR * gs, kQuickCloseR * gs, kQuickCloseR * gs,
                  kQuickCloseR * gs, 0.0f, 0.002f, bg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.92f * ga};
        const float il = kQuickCloseR * 0.52f * gs, it = 0.0026f;
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.785398f, il, it,
                  il, it, it, 0.0f, 0.0015f, icol);
        shapeQuad(e, vp, bc, r, up, 0.008f, -0.785398f, il, it,
                  il, it, it, 0.0f, 0.0015f, icol);
    }
    // title left, battery % beside the gear - dropped entirely on a
    // device with no battery
    cardText(e, vp, c, r, up, (-kQuickHW + kQuickPad) * gs, hy,
             qsStr(e, UIQS_TITLE), 0.0016f * gs, ga, false, false);
    if (e->quick.st.batt >= 0) {
        char batt[8];
        snprintf(batt, sizeof(batt), "%d%%", e->quick.st.batt);
        const float bw2 = measureText(e, batt, 0.0012f * gs);
        cardText(e, vp, c, r, up,
                 (quickGearX() - kQuickGearR - 0.016f) * gs - bw2, hy,
                 batt, 0.0012f * gs, ga, false, true);
    }

    for (int i = 0; i < QSLIDER_COUNT; ++i)
        drawSlider(e, vp, c, r, up, i, gs, ga);
    for (int i = 0; i < QS_COUNT; ++i)
        drawTile(e, vp, c, r, up, i, gs, ga);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
