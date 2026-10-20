#include "grid.h"

#include "layout.h"
#include "../anim/anim.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../render/icons.h"
#include "../render/shape.h"
#include "../text/draw.h"

#include <GLES2/gl2.h>
#include <cmath>
#include <cstring>

void syncGrid(HudEngine* e) {
    if (!e->backend) return;
    const int v = e->backend->appsVersion();
    if (v == e->grid.appsVer) return;
    e->grid.appsVer = v;
    e->backend->launcherApps(&e->grid.items);
    gridLayout(e->grid.items);
    for (auto& it : e->grid.items) it.vr = iconFor(e, it.pkg).vr;
    e->grid.scroll = gridClampScroll(e->grid.scroll, (int)e->grid.items.size());
}

void gridActivate(HudEngine* e, int idx) {
    if (idx < 0 || idx >= (int)e->grid.items.size()) return;
    queueLaunch(e->grid.items[idx].pkg.c_str());
    e->grid.shown = false;
}

void drawGrid(HudEngine* e, const Mat4& vp) {
    // openT chases `shown`, so the card is still drawn while it closes
    if (e->grid.openT <= 0.0f) return;
    const float gt = easeOutCubic(e->grid.openT);
    const float gs = kGridScale0 + (1.0f - kGridScale0) * gt;
    const float ga = gt;
    float c[3], r[3], up[3];
    gridCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
               c, r, up);
    const float hw = kGridHW * gs, hh = kGridHH * gs;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // shadow + card: frosted glass, lit from its top edge
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

    // title band: label left, close disc on the right end
    const float hy = hh - kGridHeadH * 0.5f * gs;
    {
        const bool bhov = e->grid.zone == GZONE_CLOSE;
        const float bx = gridCloseX() * gs;
        const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                             c[1] + r[1]*bx + up[1]*hy,
                             c[2] + r[2]*bx + up[2]*hy};
        const float* bgp = bhov ? kPalDanger : kPalText;
        const float bg[4] = {bgp[0], bgp[1], bgp[2],
                             (bhov ? 0.32f : 0.13f) * ga};
        shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, kGridCloseR * gs,
                  kGridCloseR * gs, kGridCloseR * gs, kGridCloseR * gs,
                  kGridCloseR * gs, 0.0f, 0.002f, bg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.92f * ga};
        drawGlyph(e, vp, GI_CLOSE, bc, r, up, kGridCloseR * 0.52f * gs,
                  0.008f, icol);
    }
    if (e->font.ok) {
        const float ts = 0.0016f * gs;
        const char* title =
            e->uiLib.empty() ? "Library" : e->uiLib.c_str();
        float gt, gb, yo = hy;
        if (textBounds(e->font.set, title, ts, &gt, &gb))
            yo = hy - (gt + gb) * 0.5f;
        float to[3] = {c[0] + r[0]*(-hw + kGridSidePad * gs) + up[0]*yo,
                       c[1] + r[1]*(-hw + kGridSidePad * gs) + up[1]*yo,
                       c[2] + r[2]*(-hw + kGridSidePad * gs) + up[2]*yo};
        to[0] -= c[0] * 0.010f; to[1] -= c[1] * 0.010f;
        to[2] -= c[2] * 0.010f;
        glUseProgram(e->textProg);
        glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                           1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    kPalText[0], kPalText[1], kPalText[2]);
        glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->font.tex);
        drawTextPanel(e, title, to, r, up, ts, 0.0f, ga);
        glUseProgram(e->shapeProg);
    }

    // cells: clip to under the title band - the icon shader fades out at
    // the band edges while a row scrolls through
    float blo, bhi;
    gridClipBand(&blo, &bhi);
    blo *= gs; bhi *= gs;
    const float clipC = (blo + bhi) * 0.5f, clipH = (bhi - blo) * 0.5f;
    for (int i = 0; i < (int)e->grid.items.size(); ++i) {
        const GridItem& it = e->grid.items[i];
        const float iy = (it.y + e->grid.scroll) * gs;
        if (!gridCellShown(iy, gs)) continue;
        const float hp = hoverP(it.hs, kGridHoverScale);
        const float s = kGridIconHW * gs * it.hs;
        const float ic[3] = {c[0] + r[0]*it.x*gs + up[0]*iy,
                             c[1] + r[1]*it.x*gs + up[1]*iy,
                             c[2] + r[2]*it.x*gs + up[2]*iy};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;

        if (hp > 0.02f) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.10f * hp * ga};
            // hugs the icon: any bigger and the lift clips into the
            // label drawn under the cell
            const float hx = s + 0.008f;
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, hx, hx, hx, hx,
                      hx * kIconRad, 0.0f, 0.002f, hl,
                      -1.0f, 0.0f, iy, clipC, clipH);
        }
        if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, s, icon->tex, ga,
                        iy, clipC, clipH);
        } else {
            glUseProgram(e->shapeProg);
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str(),
                           iy, clipC, clipH, ga);
            glUseProgram(e->shapeProg);
        }
        if (it.vr) {
            const float vc[4] = {kPalWarn[0], kPalWarn[1], kPalWarn[2],
                                 (0.65f + 0.30f * hp) * ga};
            shapeQuad(e, vp, ic, r, up, 0.009f, 0.0f, s + 0.006f,
                      s + 0.006f, s + 0.006f, s + 0.006f, s + 0.006f,
                      0.0018f, 0.0015f, vc, -1.0f, 0.0f, iy, clipC, clipH);
        }
        // the label line sits under the icon; it only draws while it fits
        // the visible band whole, the icon fade carries the edge cases
        const float ly = iy - (kGridIconHW + 0.038f) * gs;
        if (!it.label.empty() && e->font.ok &&
                ly + 0.018f < bhi && ly - 0.018f > blo) {
            const float ts = 0.0011f;
            const float lw = measureText(e, it.label.c_str(), ts) * 0.5f;
            const float maxW = (kGridHW * 2.0f - kGridSidePad * 2.0f) /
                               kGridCols * 0.48f;
            const float lw2 = lw > maxW ? maxW : lw;
            float lo3[3] = {c[0] + r[0]*(it.x*gs - lw2) + up[0]*ly,
                            c[1] + r[1]*(it.x*gs - lw2) + up[1]*ly,
                            c[2] + r[2]*(it.x*gs - lw2) + up[2]*ly};
            lo3[0] -= c[0] * 0.010f; lo3[1] -= c[1] * 0.010f;
            lo3[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, it.label.c_str(), lo3, r, up,
                          lw > maxW ? ts * maxW / lw : ts, 0.0f, ga);
            glUseProgram(e->shapeProg);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
