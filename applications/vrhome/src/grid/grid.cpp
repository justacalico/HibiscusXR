#include "grid.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../render/shape.h"
#include "../text/draw.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>

// pull the launcher app list when the java side's version moved; rebuild
// the cells and clamp the scroll so a shrinking list can't strand it
void syncGrid(HudEngine* e) {
    if (!e->bridge || !e->mAppsVer || !e->mApps) return;
    JNIEnv* env = threadEnv(e->vm);
    const int v = env->CallIntMethod(e->bridge, e->mAppsVer);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (v == e->grid.appsVer) return;
    e->grid.appsVer = v;
    e->grid.items.clear();
    jobjectArray arr =
        (jobjectArray)env->CallObjectMethod(e->bridge, e->mApps);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (arr) {
        const int n = env->GetArrayLength(arr);
        for (int i = 0; i < n; ++i) {
            jobject a = env->GetObjectArrayElement(arr, i);
            if (!a) continue;
            GridItem it;
            jstring jp = (jstring)env->GetObjectField(a, e->fAppPkg);
            jstring jl = (jstring)env->GetObjectField(a, e->fAppLabel);
            if (jp) {
                const char* c = env->GetStringUTFChars(jp, nullptr);
                it.pkg = c;
                env->ReleaseStringUTFChars(jp, c);
                env->DeleteLocalRef(jp);
            }
            if (jl) {
                const char* c = env->GetStringUTFChars(jl, nullptr);
                it.label = c;
                env->ReleaseStringUTFChars(jl, c);
                env->DeleteLocalRef(jl);
            }
            if (!it.pkg.empty()) e->grid.items.push_back(it);
            env->DeleteLocalRef(a);
        }
    }
    gridLayout(e->grid.items);
    for (auto& it : e->grid.items)
        it.vr = iconFor(e, it.pkg).vr;
    e->grid.scroll = gridClampScroll(e->grid.scroll,
                                   (int)e->grid.items.size());
}

void gridActivate(HudEngine* e, int idx) {
    if (idx < 0 || idx >= (int)e->grid.items.size()) return;
    queueLaunch(e->grid.items[idx].pkg.c_str());
    e->grid.shown = false;
}

void drawGrid(HudEngine* e, const Mat4& vp) {
    if (!e->grid.shown) return;
    float c[3], r[3], up[3];
    gridCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
               c, r, up);
    const float hw = kGridHW, hh = kGridHH;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // shadow + card, same recipe the windows wear
    const float shc[3] = {c[0] - up[0] * 0.02f, c[1] - up[1] * 0.02f,
                          c[2] - up[2] * 0.02f};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.30f};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.05f, hh + 0.05f,
              hw, hh, 0.05f, -1.0f, 0.05f, shCol);
    const float cardCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                              0.86f};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, 0.05f,
              0.0f, 0.003f, cardCol);

    // title band: label left, close disc on the right end
    const float hy = hh - kGridHeadH * 0.5f;
    {
        const float tcol[4] = {kPalSurface[0], kPalSurface[1],
                               kPalSurface[2], 0.60f};
        const float hc[3] = {c[0] + up[0]*hy, c[1] + up[1]*hy,
                             c[2] + up[2]*hy};
        shapeQuad(e, vp, hc, r, up, 0.006f, 0.0f, hw, kGridHeadH * 0.5f,
                  hw, kGridHeadH * 0.5f, 0.0f, 0.0f, 0.002f, tcol);
    }
    {
        const bool bhov = e->grid.zone == GZONE_CLOSE;
        const float bx = gridCloseX();
        const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                             c[1] + r[1]*bx + up[1]*hy,
                             c[2] + r[2]*bx + up[2]*hy};
        const float* bgp = bhov ? kPalDanger : kPalText;
        const float bg[4] = {bgp[0], bgp[1], bgp[2], bhov ? 0.32f : 0.13f};
        shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, kGridCloseR,
                  kGridCloseR, kGridCloseR, kGridCloseR, kGridCloseR,
                  0.0f, 0.002f, bg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2], 0.92f};
        const float il = kGridCloseR * 0.52f, it = 0.0026f;
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.785398f, il, it,
                  il, it, it, 0.0f, 0.0015f, icol);
        shapeQuad(e, vp, bc, r, up, 0.008f, -0.785398f, il, it,
                  il, it, it, 0.0f, 0.0015f, icol);
    }
    if (e->font.ok) {
        const float ts = 0.0016f;
        const char* title = "Library";
        float gt, gb, yo = hy;
        if (textBounds(e->font.set, title, ts, &gt, &gb))
            yo = hy - (gt + gb) * 0.5f;
        float to[3] = {c[0] + r[0]*(-hw + kGridSidePad) + up[0]*yo,
                       c[1] + r[1]*(-hw + kGridSidePad) + up[1]*yo,
                       c[2] + r[2]*(-hw + kGridSidePad) + up[2]*yo};
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
        drawTextPanel(e, title, to, r, up, ts, 0.0f);
        glUseProgram(e->shapeProg);
    }

    // cells: clip to under the title band - the icon shader fades out at
    // the band edges while a row scrolls through
    float blo, bhi;
    gridClipBand(&blo, &bhi);
    const float clipC = (blo + bhi) * 0.5f, clipH = (bhi - blo) * 0.5f;
    for (int i = 0; i < (int)e->grid.items.size(); ++i) {
        const GridItem& it = e->grid.items[i];
        const float iy = it.y + e->grid.scroll;
        if (iy + kGridCellH * 0.5f < blo - 0.02f ||
                iy - kGridCellH * 0.5f > bhi + 0.02f)
            continue;
        const bool hov = e->grid.hover == i;
        const float s = kGridIconHW * (hov ? 1.12f : 1.0f);
        const float ic[3] = {c[0] + r[0]*it.x + up[0]*iy,
                             c[1] + r[1]*it.x + up[1]*iy,
                             c[2] + r[2]*it.x + up[2]*iy};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;

        if (hov) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.10f};
            const float hx = s + 0.024f;
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, hx, hx, hx, hx,
                      hx * kIconRad, 0.0f, 0.002f, hl,
                      -1.0f, 0.0f, iy, clipC, clipH);
        }
        if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, s, icon->tex, 1.0f,
                        iy, clipC, clipH);
        } else {
            glUseProgram(e->shapeProg);
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str(),
                           iy, clipC, clipH);
            glUseProgram(e->shapeProg);
        }
        if (it.vr) {
            const float vc[4] = {kPalWarn[0], kPalWarn[1], kPalWarn[2],
                                 hov ? 0.95f : 0.65f};
            shapeQuad(e, vp, ic, r, up, 0.009f, 0.0f, s + 0.006f,
                      s + 0.006f, s + 0.006f, s + 0.006f, s + 0.006f,
                      0.0018f, 0.0015f, vc, -1.0f, 0.0f, iy, clipC, clipH);
        }
        // the label line sits under the icon; it only draws while it fits
        // the visible band whole, the icon fade carries the edge cases
        const float ly = iy - kGridIconHW - 0.038f;
        if (!it.label.empty() && e->font.ok &&
                ly + 0.018f < bhi && ly - 0.018f > blo) {
            const float ts = 0.0011f;
            const float lw = measureText(e, it.label.c_str(), ts) * 0.5f;
            const float maxW = (kGridHW * 2.0f - kGridSidePad * 2.0f) /
                               kGridCols * 0.48f;
            const float lw2 = lw > maxW ? maxW : lw;
            float lo3[3] = {c[0] + r[0]*(it.x - lw2) + up[0]*ly,
                            c[1] + r[1]*(it.x - lw2) + up[1]*ly,
                            c[2] + r[2]*(it.x - lw2) + up[2]*ly};
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
                          lw > maxW ? ts * maxW / lw : ts, 0.0f);
            glUseProgram(e->shapeProg);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
