#include "shade.h"

#include "item.h"
#include "layout.h"
#include "../anim/anim.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../hud/engine.h"
#include "../render/icons.h"
#include "../render/shape.h"
#include "../text/draw.h"
#include "../text/glyphs.h"

#include <GLES2/gl2.h>

// uiStrings fallback: what the card says before the first pull lands -
// same English the resources ship
static const char* kShadeFallback[UISHADE_COUNT] = {
    "Notifications", "Clear all", "No notifications",
};

static const char* shadeStr(HudEngine* e, int i) {
    return e->uiShade[i].empty() ? kShadeFallback[i] : e->uiShade[i].c_str();
}

// the clear-all label's text size, metres per font px like cardText's
static const float kClearTs = 0.0014f;

void syncShade(HudEngine* e) {
    if (!e->shade.shown && e->shade.openT <= 0.0f) return;
    const char* label = shadeStr(e, UISHADE_CLEAR);
    e->shade.clearHW = measureText(e, label, kClearTs) * 0.5f +
                       kShadeClearPadX;
    e->shade.scroll = shadeClampScroll(e->shade.scroll,
                                     (int)e->notifsAll.size());
}

void shadeDismissRow(HudEngine* e, int idx) {
    if (!e->backend || idx < 0 || idx >= (int)e->notifsAll.size()) return;
    const NotifItem& n = e->notifsAll[idx];
    if (!n.clearable) return;
    e->backend->dismissNotif(n.key);
    LOGI("shade dismiss %s", n.pkg.c_str());
}

void shadeClearAll(HudEngine* e) {
    if (!e->backend) return;
    for (const auto& n : e->notifsAll)
        if (n.clearable) e->backend->dismissNotif(n.key);
    LOGI("shade clear all");
}

// ------------------------------------------------------------- draw

// text on the card's plane at a card-local point, centred or left-
// anchored - the same shader juggling the quick card's cardText does
static void shadeText(HudEngine* e, const Mat4& vp, const float c[3],
                      const float r[3], const float up[3],
                      float x, float y, const char* s, float ts,
                      float alpha, bool centred, bool dim, float maxW) {
    if (!e->font.ok || !s || !*s) return;
    const std::string line = maxW > 0.0f
        ? clipText(e->font.set, s, ts, maxW) : s;
    float gt, gb, yo = y;
    if (textBounds(e->font.set, line.c_str(), ts, &gt, &gb))
        yo = y - (gt + gb) * 0.5f;
    float xo = x;
    if (centred) xo = x - measureText(e, line.c_str(), ts) * 0.5f;
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
    drawTextPanel(e, line.c_str(), o, r, up, ts, 0.0f, alpha);
    glUseProgram(e->shapeProg);
}

static void drawRow(HudEngine* e, const Mat4& vp, const float c[3],
                    const float r[3], const float up[3],
                    const NotifItem& it, int i, float gs, float ga,
                    float clipC, float clipH) {
    const float ry = shadeRowY(i, e->shade.scroll) * gs;
    const float rowHW = (kShadeHW - kShadePad) * gs;
    const float rowHH = kShadeRowH * 0.5f * gs;
    const float rc[3] = {c[0] + up[0]*ry, c[1] + up[1]*ry,
                         c[2] + up[2]*ry};
    const bool hov = e->shade.zone != SHZONE_NONE && e->shade.hover == i;

    // row card: a lighter sheet on the panel glass, a touch brighter
    // under the gaze; scroll edges fade through the clip band
    const float body[4] = {kPalSurface[0], kPalSurface[1], kPalSurface[2],
                           (hov ? 0.72f : 0.55f) * ga};
    shapeQuad(e, vp, rc, r, up, 0.006f, 0.0f, rowHW, rowHH, rowHW, rowHH,
              rowHH * 0.30f, 0.0f, 0.002f, body, -1.0f, 0.0f,
              ry, clipC, clipH, 0.06f);

    // app icon on the left; the dock's letter-tile fallback for pkgs
    // without one
    const float ix = (-kShadeHW + kShadePad + kShadeIconHW + 0.008f) * gs;
    const float ic[3] = {c[0] + r[0]*ix + up[0]*ry,
                         c[1] + r[1]*ix + up[1]*ry,
                         c[2] + r[2]*ix + up[2]*ry};
    const DockIcon* icon = nullptr;
    auto f = e->dockIcons.find(it.pkg);
    if (f != e->dockIcons.end()) icon = &f->second;
    const float is = kShadeIconHW * gs;
    if (icon && icon->tex) {
        drawIconTex(e, vp, ic, r, up, is, icon->tex, ga,
                    ry, clipC, clipH);
    } else {
        drawLetterTile(e, vp, ic, r, up, is,
                       icon && !icon->label.empty() ? icon->label.c_str()
                                                  : it.pkg.c_str(),
                       ry, clipC, clipH, ga);
        glUseProgram(e->shapeProg);
    }

    // title + body in the text column: only while the lines fit the band
    // whole, like the grid's labels - the row's own fade covers the edge
    const float tx = ix + (kShadeIconHW + 0.014f) * gs;
    const float textMax = (kShadeHW - kShadePad) * gs - tx -
                          kShadeBadgeR * 2.0f * gs;
    const float titleY = ry + 0.018f * gs, bodyY = ry - 0.026f * gs;
    const char* title = !it.title.empty() ? it.title.c_str()
        : (icon && !icon->label.empty() ? icon->label.c_str()
                                        : it.pkg.c_str());
    float blo, bhi;
    shadeClipBand(&blo, &bhi);
    blo *= gs; bhi *= gs;
    if (titleY + 0.016f < bhi && titleY - 0.016f > blo)
        shadeText(e, vp, c, r, up, tx, titleY, title, 0.0016f * gs,
                  ga, false, false, textMax);
    if (bodyY + 0.014f < bhi && bodyY - 0.014f > blo)
        shadeText(e, vp, c, r, up, tx, bodyY, it.text.c_str(),
                  0.0013f * gs, ga, false, true, textMax);

    // the dismiss badge on a clearable row, right edge
    if (it.clearable) {
        float bx, by;
        shadeBadgeAt(&bx, &by);
        const float bc[3] = {c[0] + r[0]*bx*gs + up[0]*(ry + by*gs),
                             c[1] + r[1]*bx*gs + up[1]*(ry + by*gs),
                             c[2] + r[2]*bx*gs + up[2]*(ry + by*gs)};
        const bool bhov = e->shade.zone == SHZONE_DISMISS &&
                          e->shade.hover == i;
        const float* bgp = bhov ? kPalDanger : kPalSurfaceHigh;
        const float bcol[4] = {bgp[0], bgp[1], bgp[2], 0.92f * ga};
        const float br = kShadeBadgeR * gs;
        shapeQuad(e, vp, bc, r, up, 0.011f, 0.0f, br, br, br, br, br,
                  0.0f, 0.0015f, bcol, -1.0f, 0.0f, ry, clipC, clipH);
        const float xcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.95f * ga};
        drawGlyph(e, vp, GI_CLOSE, bc, r, up, br * 0.52f, 0.012f, xcol);
    }
}

void drawShade(HudEngine* e, const Mat4& vp) {
    // openT chases `shown`, so the card is still drawn while it closes
    if (e->shade.openT <= 0.0f) return;
    const float qt = easeOutCubic(e->shade.openT);
    const float gs = kShadeScale0 + (1.0f - kShadeScale0) * qt;
    const float ga = qt;
    float c[3], r[3], up[3];
    shadeCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
                c, r, up);
    const float hw = kShadeHW * gs, hh = kShadeHH * gs;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // shadow + card, same glass recipe the other dash cards use
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

    // the rows scroll under the title band; the band draws over them so a
    // sliding row never bleeds into it
    float blo, bhi;
    shadeClipBand(&blo, &bhi);
    blo *= gs; bhi *= gs;
    const float clipC = (blo + bhi) * 0.5f, clipH = (bhi - blo) * 0.5f;
    const int n = (int)e->notifsAll.size();
    for (int i = 0; i < n; ++i) {
        const float ry = shadeRowY(i, e->shade.scroll) * gs;
        if (!shadeRowShown(ry, gs)) continue;
        drawRow(e, vp, c, r, up, e->notifsAll[i], i, gs, ga,
                clipC, clipH);
    }

    // title band: a sheet over the row tops, then title, clear-all pill
    // and close disc
    const float hy = shadeTitleY() * gs;
    const float hc[3] = {c[0] + up[0]*hy, c[1] + up[1]*hy, c[2] + up[2]*hy};
    const float hcol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                           0.86f * ga};
    shapeQuad(e, vp, hc, r, up, 0.008f, 0.0f, hw, kShadeHeadH * 0.5f * gs,
              hw, kShadeHeadH * 0.5f * gs, 0.030f * gs, 0.0f, 0.002f,
              hcol, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.10f);
    shadeText(e, vp, c, r, up, (-kShadeHW + kShadePad) * gs, hy,
              shadeStr(e, UISHADE_TITLE), 0.0016f * gs, ga, false, false,
              0.0f);
    {
        const bool chov = e->shade.zone == SHZONE_CLEAR;
        const bool can = shadeClearable(e->notifsAll);
        float px, py, phw, phh;
        shadeClearPill(e->shade.clearHW, &px, &py, &phw, &phh);
        const float pc[3] = {c[0] + r[0]*px*gs + up[0]*py*gs,
                             c[1] + r[1]*px*gs + up[1]*py*gs,
                             c[2] + r[2]*px*gs + up[2]*py*gs};
        const float bg[4] = {kPalText[0], kPalText[1], kPalText[2],
                             ((chov && can) ? 0.30f : 0.13f) * ga};
        shapeQuad(e, vp, pc, r, up, 0.010f, 0.0f, phw * gs, phh * gs,
                  phw * gs, phh * gs, phh * gs, 0.0f, 0.002f, bg);
        shadeText(e, vp, c, r, up, px * gs, py * gs,
                  shadeStr(e, UISHADE_CLEAR), kClearTs * gs,
                  (can ? ga : 0.45f * ga), true, !can, 0.0f);
    }
    {
        const bool bhov = e->shade.zone == SHZONE_CLOSE;
        const float bx = shadeCloseX() * gs;
        const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                             c[1] + r[1]*bx + up[1]*hy,
                             c[2] + r[2]*bx + up[2]*hy};
        const float* bgp = bhov ? kPalDanger : kPalText;
        const float bg[4] = {bgp[0], bgp[1], bgp[2],
                             (bhov ? 0.32f : 0.13f) * ga};
        shapeQuad(e, vp, bc, r, up, 0.010f, 0.0f, kShadeCloseR * gs,
                  kShadeCloseR * gs, kShadeCloseR * gs, kShadeCloseR * gs,
                  kShadeCloseR * gs, 0.0f, 0.002f, bg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.92f * ga};
        drawGlyph(e, vp, GI_CLOSE, bc, r, up, kShadeCloseR * 0.52f * gs,
                  0.008f, icol);
    }

    // empty shade: one dim line centred in the list band
    if (n == 0) {
        const float ey = (blo + bhi) * 0.5f;
        shadeText(e, vp, c, r, up, 0.0f, ey, shadeStr(e, UISHADE_EMPTY),
                  0.0015f * gs, ga, true, true, 0.0f);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
