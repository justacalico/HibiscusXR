#include "chrome.h"

#include "../hud/engine.h"
#include "../anim/anim.h"
#include "../common/config.h"
#include "../common/palette.h"
#include "../dock/layout.h"
#include "../grid/layout.h"
#include "../kbd/kbd.h"
#include "../panels/layout.h"
#include "../pill/pill.h"
#include "shape.h"
#include "../text/draw.h"

#include <cmath>
#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// the panel's animated transform this frame: the spawn-in scale/fade and
// the park flight fold into a centre offset toward the shelf slot, a size
// multiplier and a master alpha. False once the window is fully parked -
// minimized panels draw nothing the rest of the time
static bool panelVis(HudEngine* e, int i, const float shc[3],
                     const float shr[3], float c[3], float r[3],
                     float up[3], float* sc, float* a) {
    Panel& p = e->panels[i];
    if (p.minT >= 1.0f) return false;
    panelCenter(p, e->ringPos, c, r, up);
    const float st = progT(p.bornMs, e->frameMs, kSpawnMs);
    *sc = spawnScale(st);
    *a = spawnAlpha(st);
    if (p.minT > 0.0f) {
        const float mt = minEase(p.minT);
        float tx = 0.0f;
        shelfXFor(e->shelf, i, &tx);
        const float ic[3] = {shc[0] + shr[0]*tx, shc[1] + shr[1]*tx,
                             shc[2] + shr[2]*tx};
        lerp3(c, ic, mt, c);
        *sc *= minScale(mt);
        *a *= minAlpha(mt);
    }
    return true;
}

void drawPanels(HudEngine* e, const Mat4& viewProj) {
    if (e->panels.empty()) return;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // the park flight's target frame: the shelf pill's anchor, shared by
    // every window in transit
    float shc[3], shr[3], shu[3];
    shelfCenter(e->dockYaw, e->dockPitch, e->ringPos, shc, shr, shu);

    // shadows first: they sit behind the panels and must not cover a
    // neighbouring window
    glUseProgram(e->shapeProg);
    for (int i = 0; i < (int)e->panels.size(); ++i) {
        Panel& p = e->panels[i];
        float c[3], r[3], up[3], sc, a;
        if (!panelVis(e, i, shc, shr, c, r, up, &sc, &a)) continue;
        const float hw = panelHW(p) * sc, hh = panelHH(p) * sc;
        // chrome hangs below: the pill under the window adds to the
        // silhouette's bottom, so the shadow centre dips halfway to it
        const float drp = pillDrop(hh);
        const float shw = hw + 0.10f * sc, shh = hh + drp + 0.10f * sc;
        const float sc2[3] = {c[0] - up[0] * drp * 0.5f,
                              c[1] - up[1] * drp * 0.5f,
                              c[2] - up[2] * drp * 0.5f};
        const float col[4] = {0.0f, 0.0f, 0.0f, 0.36f * a};
        shapeQuad(e, viewProj, sc2, r, up, -0.03f * sc, 0.0f, shw, shh,
                  shw - 0.10f * sc, shh - 0.10f * sc, 0.10f * sc, -1.0f,
                  0.10f * sc, col);
    }

    for (int i = 0; i < (int)e->panels.size(); ++i) {
        Panel& p = e->panels[i];
        float c[3], r[3], up[3], sc, a;
        if (!panelVis(e, i, shc, shr, c, r, up, &sc, &a)) continue;
        const float hw = panelHW(p) * sc, hh = panelHH(p) * sc;
        const bool hov = (e->hover == i);
        const int nbtns = 3;
        const float phw = pillBarHW(hw);
        const float drp = pillDrop(hh);

        // the label is measured first so it can shrink to fit the pill's
        // text region left of the button strip - the fit math runs on the
        // rest size so the font doesn't pump mid-transition
        float s = 0.0014f, bold = 0.0f, w = 0.0f;
        if (!p.label.empty() && e->font.ok) {
            const float lim = pillTextLimit(pillBarHW(panelHW(p)), nbtns);
            bold = 0.8f * s;
            w = measureText(e, p.label.c_str(), s) + bold;
            if (w > lim) {
                s *= lim / w;
                bold = 0.8f * s;
                w = measureText(e, p.label.c_str(), s) + bold;
            }
        }

        // under-window pill: a capsule hanging off the bottom edge with a
        // small gap, carrying the label on the left and the buttons on the
        // right - the Quest window bar, just moved under the surface
        const float pc[3] = {c[0] - up[0] * drp, c[1] - up[1] * drp,
                             c[2] - up[2] * drp};
        const float* bcp = hov ? kPalSurfaceHigh : kPalPanel;
        const float pillCol[4] = {bcp[0], bcp[1], bcp[2],
                                  (hov ? 0.95f : 0.88f) * a};
        glUseProgram(e->shapeProg);
        shapeQuad(e, viewProj, pc, r, up, 0.004f, 0.0f, phw,
                  kPillH * 0.5f * sc, phw, kPillH * 0.5f * sc,
                  kPillH * 0.5f * sc, 0.0f, 0.002f, pillCol, 0.0f);

        // float + minimize + close discs on the pill's right end; glyphs
        // are small capsules, the close pair rotated into an x, the float
        // one four diagonal ticks pointing out - the window leaves the grid
        {
            const float icon[4] = {kPalText[0], kPalText[1], kPalText[2],
                                   0.92f * a};
            const float il = kPillBtnR * 0.55f * sc, it = 0.0028f * sc;
            const float bxs[3] = {pillFloatX(phw), pillMinX(phw),
                                  pillCloseX(phw)};
            const int zones[3] = {ZONE_FLOAT, ZONE_MIN, ZONE_CLOSE};
            for (int b = 0; b < 3; ++b) {
                const float bx = bxs[b];
                const bool bhov = hov && e->hoverZone == zones[b];
                const float bc[3] = {pc[0] + r[0]*bx, pc[1] + r[1]*bx,
                                     pc[2] + r[2]*bx};
                const float* bgp = bhov && zones[b] == ZONE_CLOSE
                                   ? kPalDanger
                                   : p.floating && zones[b] == ZONE_FLOAT
                                     ? kPalAccent : kPalText;
                const float bg[4] = {bgp[0], bgp[1], bgp[2],
                                     (bhov ? 0.32f : 0.13f) * a};
                shapeQuad(e, viewProj, bc, r, up, 0.006f, 0.0f,
                          kPillBtnR * sc, kPillBtnR * sc, kPillBtnR * sc,
                          kPillBtnR * sc, kPillBtnR * sc, 0.0f, 0.002f, bg);
                if (zones[b] == ZONE_FLOAT) {
                    // two overlapping windows, lit while floating: a
                    // stroked square behind top-left, a pane-filled one
                    // in front bottom-right - reads as a free window
                    // where plain corner ticks read as fullscreen
                    const float* gcp = p.floating ? kPalAccent : kPalText;
                    const float gc[4] = {gcp[0], gcp[1], gcp[2], 0.92f * a};
                    const float wo = kPillBtnR * 0.15f * sc;
                    const float ws = kPillBtnR * 0.32f * sc;
                    const float wr = ws * 0.28f;
                    const float wt = it * 0.8f;
                    const float back[3] = {bc[0] - r[0]*wo + up[0]*wo,
                                           bc[1] - r[1]*wo + up[1]*wo,
                                           bc[2] - r[2]*wo + up[2]*wo};
                    const float fore[3] = {bc[0] + r[0]*wo - up[0]*wo,
                                           bc[1] + r[1]*wo - up[1]*wo,
                                           bc[2] + r[2]*wo - up[2]*wo};
                    shapeQuad(e, viewProj, back, r, up, 0.008f, 0.0f,
                              ws, ws, ws, ws, wr, wt, 0.0015f, gc);
                    const float pane[4] = {bcp[0], bcp[1], bcp[2],
                                           0.95f * a};
                    shapeQuad(e, viewProj, fore, r, up, 0.009f, 0.0f,
                              ws, ws, ws, ws, wr, 0.0f, 0.0015f, pane);
                    shapeQuad(e, viewProj, fore, r, up, 0.010f, 0.0f,
                              ws, ws, ws, ws, wr, wt, 0.0015f, gc);
                } else if (zones[b] == ZONE_MIN) {
                    shapeQuad(e, viewProj, bc, r, up, 0.008f, 0.0f, il, it,
                              il, it, it, 0.0f, 0.0015f, icon);
                } else {
                    shapeQuad(e, viewProj, bc, r, up, 0.008f, 0.785398f, il,
                              it, il, it, it, 0.0f, 0.0015f, icon);
                    shapeQuad(e, viewProj, bc, r, up, 0.008f, -0.785398f, il,
                              it, il, it, it, 0.0f, 0.0015f, icon);
                }
            }
        }

        // the app surface itself: all four corners rounded now that no
        // chrome is bound to any edge
        glUseProgram(e->floatProg);
        glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
        glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), hw, hh);
        glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"),
                    kCornerR * sc);
        glUniform1f(glGetUniformLocation(e->floatProg, "uRadiusB"),
                    kCornerR * sc);
        glUniform1f(glGetUniformLocation(e->floatProg, "uAlpha"), a);
        const GLint uMVP = glGetUniformLocation(e->floatProg, "uMVP");
        const GLint uST  = glGetUniformLocation(e->floatProg, "uST");
        const GLint aPos = glGetAttribLocation(e->floatProg, "aPos");
        const GLint aUV  = glGetAttribLocation(e->floatProg, "aUV");
        const float q[4][5] = {
            {c[0]-r[0]*hw-up[0]*hh, c[1]-r[1]*hw-up[1]*hh,
             c[2]-r[2]*hw-up[2]*hh, 0.0f, 0.0f},
            {c[0]+r[0]*hw-up[0]*hh, c[1]+r[1]*hw-up[1]*hh,
             c[2]+r[2]*hw-up[2]*hh, 1.0f, 0.0f},
            {c[0]+r[0]*hw+up[0]*hh, c[1]+r[1]*hw+up[1]*hh,
             c[2]+r[2]*hw+up[2]*hh, 1.0f, 1.0f},
            {c[0]-r[0]*hw+up[0]*hh, c[1]-r[1]*hw+up[1]*hh,
             c[2]-r[2]*hw+up[2]*hh, 0.0f, 1.0f},
        };
        const int tris[6] = {0,1,2, 0,2,3};
        float verts[30];
        for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
        glUniformMatrix4fv(uST, 1, GL_FALSE, p.stMat);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, p.tex);
        glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
        glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
        glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
        glEnableVertexAttribArray(aPos);
        glEnableVertexAttribArray(aUV);
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthMask(GL_FALSE);

        // hairline border around the window alone - the pill floats free
        // underneath and needs no outline of its own
        glUseProgram(e->shapeProg);
        const float bdCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                (hov ? 0.55f : 0.14f) * a};
        shapeQuad(e, viewProj, c, r, up, 0.006f, 0.0f, hw + 0.006f,
                  hh + 0.006f, hw + 0.006f, hh + 0.006f,
                  kCornerR * sc + 0.006f, 0.0016f, 0.0012f, bdCol);

        // the resize grip on the window's bottom-right corner: two short
        // diagonal ticks tucked inside the corner, lighting while held
        {
            const bool ghov = hov && e->hoverZone == ZONE_RESIZE;
            const bool gheld = e->pressZone == ZONE_RESIZE &&
                               e->pressDisp == p.displayId;
            const float* gp = ghov || gheld ? kPalAccent : kPalText;
            const float gcol[4] = {gp[0], gp[1], gp[2],
                                   (ghov || gheld ? 0.9f : 0.45f) * a};
            for (int g = 0; g < 2; ++g) {
                const float off = (0.030f - g * 0.016f) * sc;
                const float gc[3] = {
                    c[0] + r[0]*(hw - off) + up[0]*(-hh + off),
                    c[1] + r[1]*(hw - off) + up[1]*(-hh + off),
                    c[2] + r[2]*(hw - off) + up[2]*(-hh + off)};
                const float gl = 0.011f * sc;
                shapeQuad(e, viewProj, gc, r, up, 0.008f, 0.785398f,
                          gl, 0.0022f, gl, 0.0022f, 0.0022f, 0.0f,
                          0.0015f, gcol);
            }
        }

        // the pill body already doubles as a floating window's move
        // handle, so no separate drag line is drawn anymore

        // app label left-aligned inside the pill, bold, shrunk to fit if
        // the name is long. Centering uses the real glyph bounds, not the
        // font's nominal ascent, so descenders don't push it off-centre
        if (!p.label.empty() && e->font.ok) {
            float boff = -drp;
            float gt, gb;
            if (textBounds(e->font.set, p.label.c_str(), s, &gt, &gb))
                boff = -drp - (gt + gb) * 0.5f;
            const float tx = -(phw - kPillPadX * sc);
            float to[3] = {c[0] + r[0] * tx + up[0] * boff,
                           c[1] + r[1] * tx + up[1] * boff,
                           c[2] + r[2] * tx + up[2] * boff};
            to[0] -= c[0] * 0.010f; to[1] -= c[1] * 0.010f;
            to[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, viewProj.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, p.label.c_str(), to, r, up, s * sc,
                          bold * sc, a);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// the floating keyboard: a textured quad under its host panel, rounded on
// all four corners - unlike the windows it owns no chrome bar, the IME's
// own window is a hairline on the app display and all the keys live in the
// texture
void drawKbd(HudEngine* e, const Mat4& viewProj) {
    if (!e->kbd.shown || !e->kbd.st) return;
    float c[3], r[3], up[3];
    kbdFrame(e->panels, e->kbd.hostDisp, e->kbd.only,
             e->kbd.only ? e->kbd.yaw : e->dockYaw,
             e->kbd.offYaw, e->kbd.offY, e->ringPos, c, r, up);
    const float hw = kKbdHW, hh = kKbdHH;
    const bool hov = e->kbd.hover;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // shadow, same recipe the windows use
    glUseProgram(e->shapeProg);
    const float scol[4] = {0.0f, 0.0f, 0.0f, 0.36f};
    shapeQuad(e, viewProj, c, r, up, -0.03f, 0.0f, hw + 0.10f, hh + 0.10f,
              hw, hh, 0.10f, -1.0f, 0.10f, scol);

    // the IME's surface, rounded on all corners
    glUseProgram(e->floatProg);
    glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
    glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), hw, hh);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"), kCornerR);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadiusB"), kCornerR);
    // the panel loop leaves its transition fade on the program - the quad
    // doesn't animate, so it draws opaque
    glUniform1f(glGetUniformLocation(e->floatProg, "uAlpha"), 1.0f);
    const GLint uMVP = glGetUniformLocation(e->floatProg, "uMVP");
    const GLint uST  = glGetUniformLocation(e->floatProg, "uST");
    const GLint aPos = glGetAttribLocation(e->floatProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->floatProg, "aUV");
    const float q[4][5] = {
        {c[0]-r[0]*hw-up[0]*hh, c[1]-r[1]*hw-up[1]*hh,
         c[2]-r[2]*hw-up[2]*hh, 0.0f, 0.0f},
        {c[0]+r[0]*hw-up[0]*hh, c[1]+r[1]*hw-up[1]*hh,
         c[2]+r[2]*hw-up[2]*hh, 1.0f, 0.0f},
        {c[0]+r[0]*hw+up[0]*hh, c[1]+r[1]*hw+up[1]*hh,
         c[2]+r[2]*hw+up[2]*hh, 1.0f, 1.0f},
        {c[0]-r[0]*hw+up[0]*hh, c[1]-r[1]*hw+up[1]*hh,
         c[2]-r[2]*hw+up[2]*hh, 0.0f, 1.0f},
    };
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glUniformMatrix4fv(uST, 1, GL_FALSE, e->kbd.stMat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->kbd.tex);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDepthMask(GL_TRUE);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDepthMask(GL_FALSE);

    // hairline border, brightened under the aim like the window frames
    glUseProgram(e->shapeProg);
    const float bcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           hov ? 0.55f : 0.14f};
    shapeQuad(e, viewProj, c, r, up, 0.006f, 0.0f, hw + 0.006f,
              hh + 0.006f, hw + 0.006f, hh + 0.006f, kCornerR + 0.006f,
              0.0016f, 0.0012f, bcol);

    // the move pill under the quad: holding it drags the keyboard alone
    drawMovePill(e, viewProj, c, r, up, movePillDrop(kKbdHH),
                 e->kbd.zone == KZONE_HANDLE || e->kbd.moveHeld);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawMovePill(HudEngine* e, const Mat4& vp, const float c[3],
                  const float r[3], const float up[3], float drop,
                  bool hot, float alpha) {
    const float hc[3] = {c[0] - up[0] * drop, c[1] - up[1] * drop,
                         c[2] - up[2] * drop};
    const float hcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           (hot ? 0.95f : 0.55f) * alpha};
    shapeQuad(e, vp, hc, r, up, 0.006f, 0.0f, kHandleW, kHandleT,
              kHandleW, kHandleT, kHandleT, 0.0f, 0.0015f, hcol);
}

// summon-key hold feedback: a flat overlay ring whose arc fills while the
// button is held. The quad is built in clip space so it never touches the
// head pose - it stays glued to the screen centre no matter how the user
// moves during the hold
void drawHoldRing(HudEngine* e) {
    if (e->holdP <= 0.0f) return;
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(e->holdProg);
    const GLint uMVP  = glGetUniformLocation(e->holdProg, "uMVP");
    const GLint uProg = glGetUniformLocation(e->holdProg, "uProg");
    const GLint uCol  = glGetUniformLocation(e->holdProg, "uColor");
    const GLint aPos  = glGetAttribLocation(e->holdProg, "aPos");
    const GLint aUV   = glGetAttribLocation(e->holdProg, "aUV");
    GLint vpBox[4];
    glGetIntegerv(GL_VIEWPORT, vpBox);
    const float aspect = vpBox[3] > 0 ? (float)vpBox[2] / (float)vpBox[3] : 1.0f;
    const float s = kHoldSize, sx = s / aspect;
    const float q[4][5] = {
        {-sx, -s, 0.0f, -1.0f, -1.0f}, { sx, -s, 0.0f,  1.0f, -1.0f},
        { sx,  s, 0.0f,  1.0f,  1.0f}, {-sx,  s, 0.0f, -1.0f,  1.0f},
    };
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
    const Mat4 mvp = identity();
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, mvp.m);
    glUniform1f(uProg, e->holdP);
    const float col[4] = {kPalAccent[0], kPalAccent[1], kPalAccent[2],
                          0.95f};
    glUniform4fv(uCol, 1, col);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void drawCursor(HudEngine* e, const Mat4& viewProj) {
    float c[3], r[3], up[3], pos[3];
    if (e->dockHover >= 0 || e->dockZone == DZONE_HANDLE) {
        // on the dock the cursor sits on the strip's own plane - a handle
        // hit has no item index but still carries u,v, so it lands on the
        // line under the bar. The summon slide counts: the dot rides the
        // strip while it settles
        dockCenterDrop(e->dockYaw, e->dockPitch,
                       dashDrop(progT(e->summonMs, e->frameMs, kDashMs)),
                       e->ringPos, c, r, up);
        const float hw = e->dockHW, hh = kDockBarH * 0.5f;
        pos[0] = c[0] + r[0]*e->dockU*hw + up[0]*e->dockV*hh;
        pos[1] = c[1] + r[1]*e->dockU*hw + up[1]*e->dockV*hh;
        pos[2] = c[2] + r[2]*e->dockU*hw + up[2]*e->dockV*hh;
    } else if (e->grid.zone != GZONE_NONE) {
        // on the app-grid card the dot lands on the card's own plane
        gridCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
                   c, r, up);
        pos[0] = c[0] + r[0]*e->grid.u*kGridHW + up[0]*e->grid.v*kGridHH;
        pos[1] = c[1] + r[1]*e->grid.u*kGridHW + up[1]*e->grid.v*kGridHH;
        pos[2] = c[2] + r[2]*e->grid.u*kGridHW + up[2]*e->grid.v*kGridHH;
    } else if (e->kbd.hover) {
        // keys and the pill share the quad's plane, so u/v (even past the
        // quad's edges, where the pill sits) drop the dot right on the hit
        kbdFrame(e->panels, e->kbd.hostDisp, e->kbd.only,
                 e->kbd.only ? e->kbd.yaw : e->dockYaw,
                 e->kbd.offYaw, e->kbd.offY, e->ringPos, c, r, up);
        pos[0] = c[0] + r[0]*e->kbd.u*kKbdHW + up[0]*e->kbd.v*kKbdHH;
        pos[1] = c[1] + r[1]*e->kbd.u*kKbdHW + up[1]*e->kbd.v*kKbdHH;
        pos[2] = c[2] + r[2]*e->kbd.u*kKbdHW + up[2]*e->kbd.v*kKbdHH;
    } else if (e->hover >= 0 && e->hover < (int)e->panels.size()) {
        const Panel& p = e->panels[e->hover];
        panelCenter(p, e->ringPos, c, r, up);
        const float u = e->hitU, v = e->hitV;
        const float hw = panelHW(p), hh = panelHH(p);
        pos[0] = c[0] + r[0]*u*hw + up[0]*v*hh;
        pos[1] = c[1] + r[1]*u*hw + up[1]*v*hh;
        pos[2] = c[2] + r[2]*u*hw + up[2]*v*hh;
    } else {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(e->shapeProg);
    // a background-shade stroke under each text-shade piece rims the cursor,
    // so it stays readable over a surface its own colour either theme way
    const float edgeCol[4] = {kPalBackground[0], kPalBackground[1],
                              kPalBackground[2], 0.90f};
    const float ringCol[4] = {kPalText[0], kPalText[1], kPalText[2], 0.85f};
    const float dotCol[4]  = {kPalText[0], kPalText[1], kPalText[2], 0.90f};
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.0195f, 0.0195f, 0.014f,
              0.014f, 0.014f, 0.0042f, 0.001f, edgeCol);
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.014f, 0.014f, 0.014f,
              0.014f, 0.014f, 0.0016f, 0.001f, ringCol);
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.009f, 0.009f, 0.0075f,
              0.0075f, 0.0075f, 0.0f, 0.001f, edgeCol);
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.005f, 0.005f, 0.005f,
              0.005f, 0.005f, 0.0f, 0.001f, dotCol);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
