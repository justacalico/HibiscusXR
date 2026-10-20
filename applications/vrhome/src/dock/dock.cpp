#include "dock.h"

#include "layout.h"
#include "../anim/anim.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/palette.h"
#include "../panels/layout.h"
#include "../pill/pill.h"
#include "../render/chrome.h"
#include "../render/icons.h"
#include "../render/shape.h"
#include "../set/set.h"
#include "../text/draw.h"
#include "../text/utf8.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>
#include <ctime>

static long long nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static unsigned loadIconTex(HudEngine* e, const char* pkg) {
    IconPixels px;
    if (!e->backend || !e->backend->iconPixels(pkg, &px) ||
            px.w <= 0 || px.h <= 0) return 0;
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px.w, px.h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, px.rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

static void pullPins(HudEngine* e) {
    e->backend->takePins(&e->dockPins);
}

static void pullXr(HudEngine* e) {
    const int v = e->backend->vrVersion();
    if (v == e->dockXrVer) return;
    e->dockXrVer = v;
    e->backend->runningVr(&e->dockXr);
}

static void pullSys(HudEngine* e) {
    int v[3];
    e->backend->sysStatus(v);
    e->sysNet = v[0]; e->sysBatt = v[1]; e->sysChg = v[2];
}

DockIcon& iconFor(HudEngine* e, const std::string& pkg) {
    DockIcon& ic = e->dockIcons[pkg];
    if (!ic.tried) {
        ic.tried = true;
        ic.tex = loadIconTex(e, pkg.c_str());
    }
    if (ic.label.empty()) ic.label = e->backend->appLabel(pkg);
    if (!ic.vrTried) {
        ic.vrTried = true;
        ic.vr = e->backend->isVrApp(pkg);
    }
    return ic;
}

void syncDock(HudEngine* e) {
    if (e->backend) {
        pullPins(e);
        pullXr(e);
        pullSys(e);
    }
    std::time_t tt = std::time(nullptr);
    std::strftime(e->sysClock, sizeof(e->sysClock), "%H:%M",
                  std::localtime(&tt));
    e->dockSys.clockW = measureText(e, e->sysClock, kSysPx);
    e->dockSys.hasBatt = e->sysBatt >= 0;
    std::vector<DockItem> prevDock;
    prevDock.swap(e->dock);
    e->dock = buildDock(e->dockPins, e->panels, e->dockXr);
    carryHover(e->dock, prevDock);
    e->dockHW = dockLayout(e->dock, e->dockSys);
    std::vector<ShelfItem> prevShelf;
    prevShelf.swap(e->shelf);
    e->shelf = buildShelf(e->panels);
    carryShelfHover(e->shelf, prevShelf);
    e->shelfHW = shelfLayout(e->shelf);
    if (!e->backend) return;
    for (auto& it : e->dock) {
        if (it.pkg.empty()) {
            if (it.kind == DK_GRID && !e->uiLib.empty())
                it.label = e->uiLib;
            continue;
        }
        const DockIcon& ic = iconFor(e, it.pkg);
        it.label = ic.label;
        if (ic.vr) it.vr = true;
    }
    for (auto& it : e->shelf)
        it.label = iconFor(e, it.pkg).label;
}

static void focusTask(HudEngine* e, int taskId) {
    e->backend->focusTask(taskId);
}

void dockActivate(HudEngine* e, int idx) {
    if (!e->backend || idx < 0 || idx >= (int)e->dock.size()) return;
    const DockAction act = dockActivateAction(e->dock[idx], e->panels);
    switch (act.op) {
    case DOP_TOGGLE_GRID:
        e->grid.shown = !e->grid.shown;
        if (e->grid.shown) {
            e->quick.shown = false;
            if (e->set.shown) setClose(e);
        }
        return;
    case DOP_LAUNCH:
        queueLaunch(act.pkg.c_str());
        return;
    case DOP_FOCUS_XR:
        if (act.taskId >= 0) focusTask(e, act.taskId);
        e->backend->dismissMenu();
        if (e->set.shown) setClose(e);
        return;
    case DOP_FOCUS_PANEL:
        restorePanel(e->panels, act.panelIdx, e->dockYaw,
                     dashRingPitch(e->panels, e->dockPitch));
        if (act.taskId >= 0) focusTask(e, act.taskId);
        if (e->set.shown) setClose(e);
        return;
    default:
        return;
    }
}

void dockSysActivate(HudEngine* e) {
    // the status pill flips the quick-panel card - an overlay inside the
    // dash, not a windowed app. Only one card overlay is up at a time
    e->quick.shown = !e->quick.shown;
    if (e->quick.shown) {
        e->grid.shown = false;
        if (e->set.shown) setClose(e);
    }
}

void dockClose(HudEngine* e, int idx) {
    if (!e->backend || idx < 0 || idx >= (int)e->dock.size()) return;
    const DockAction act = dockCloseAction(e->dock[idx], e->panels);
    if (act.op == DOP_CLOSE_PANEL)
        e->backend->removeTasksOnDisplay(act.displayId);
    else if (act.op == DOP_CLOSE_TASK)
        e->backend->removeTask(act.taskId);
}

void shelfActivate(HudEngine* e, int idx) {
    if (idx < 0 || idx >= (int)e->shelf.size()) return;
    const DockAction act = shelfActivateAction(e->shelf[idx], e->panels);
    if (act.op != DOP_FOCUS_PANEL) return;
    restorePanel(e->panels, act.panelIdx, e->dockYaw,
                 dashRingPitch(e->panels, e->dockPitch));
    if (e->backend && act.taskId >= 0) focusTask(e, act.taskId);
    if (e->set.shown) setClose(e);
}

void dockPushPins(HudEngine* e) {
    if (e->backend) e->backend->setPins(e->dockPins);
}

void dockTogglePin(HudEngine* e, const char* pkg) {
    e->dockPins = pinToggle(e->dockPins, pkg);
    LOGI("dock pin toggle %s -> %zu pins", pkg, e->dockPins.size());
    dockPushPins(e);
}

void dockTick(HudEngine* e) {
    if (e->dockPress < 0) { e->dockPinP = 0.0f; return; }
    const DockItem* it = nullptr;
    if (e->dockPress < (int)e->dock.size() &&
            e->dock[e->dockPress].pkg == e->dockPressPkg)
        it = &e->dock[e->dockPress];
    if (!it || !dockPinnable(*it)) { e->dockPinP = 0.0f; return; }
    const float p = (float)(nowMs() - e->dockPressMs) / (float)kDockPinMs;
    e->dockPinP = p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p;
    if (p < 1.0f || e->dockPinDone) return;
    e->dockPinDone = true;
    dockTogglePin(e, it->pkg.c_str());
}

// ------------------------------------------------------------- draw

// fallback tile for an app with no icon: a coloured rounded square carrying
// the label's first letter
void drawLetterTile(HudEngine* e, const Mat4& vp, const float ic[3],
                    const float r[3], const float up[3], float s,
                    const char* label,
                    float clipY, float clipC, float clipH, float alpha) {
    static const float pal[][3] = {
        {0.20f, 0.40f, 0.72f}, {0.48f, 0.32f, 0.68f}, {0.16f, 0.52f, 0.56f},
        {0.66f, 0.30f, 0.34f}, {0.22f, 0.56f, 0.36f}, {0.72f, 0.52f, 0.22f},
    };
    unsigned h = 0;
    for (const char* q = label; *q; ++q) h = h * 31 + (unsigned char)*q;
    const float* pc = pal[h % 6];
    const float col[4] = {pc[0], pc[1], pc[2], alpha};
    shapeQuad(e, vp, ic, r, up, 0.008f, 0.0f, s, s, s, s, s * kIconRad,
              0.0f, 0.002f, col, -1.0f, 0.0f, clipY, clipC, clipH, 0.06f);
    // text has no clip hook: inside a scroll band the letter only draws
    // while the whole tile fits the band
    const bool clipped = clipH > 0.0f &&
                         (clipY + s > clipC + clipH ||
                          clipY - s < clipC - clipH);
    if (*label && e->font.ok && !clipped) {
        // the tile letter is the first codepoint, not the first byte -
        // a CJK label would otherwise draw a lone lead byte
        char ch[5] = {0};
        const char* q = label;
        nextCp(q);
        memcpy(ch, label, (size_t)(q - label));
        // glyph height lands a bit under the tile's: mPerPx is metres per
        // font pixel, not a fraction of the tile
        const float ts = s * 0.04f;
        const float tw = measureText(e, ch, ts) * 0.5f;
        float gt, gb;
        float yo = 0.0f;
        if (textBounds(e->font.set, ch, ts, &gt, &gb))
            yo = -(gt + gb) * 0.5f;
        float o[3] = {ic[0] - r[0]*tw + up[0]*yo - ic[0]*0.010f,
                      ic[1] - r[1]*tw + up[1]*yo - ic[1]*0.010f,
                      ic[2] - r[2]*tw + up[2]*yo - ic[2]*0.010f};
        glUseProgram(e->textProg);
        glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                           1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    kPalText[0], kPalText[1], kPalText[2]);
        glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->font.tex);
        drawTextPanel(e, ch, o, r, up, ts, 0.0f, alpha);
        glUseProgram(e->shapeProg);
    }
}

// textured app icon on a plane's frame: a rounded-crop quad fed through the
// icon shader; bitmaps upload top-row-first, so v=0 is the image's top
void drawIconTex(HudEngine* e, const Mat4& vp, const float ic[3],
                 const float r[3], const float up[3], float s,
                 unsigned tex, float alpha,
                 float clipY, float clipC, float clipH) {
    glUseProgram(e->iconProg);
    const GLint uMVP = glGetUniformLocation(e->iconProg, "uMVP");
    const GLint uTex = glGetUniformLocation(e->iconProg, "uTex");
    const GLint uHalf = glGetUniformLocation(e->iconProg, "uHalf");
    const GLint uRad = glGetUniformLocation(e->iconProg, "uRadius");
    const GLint uAl = glGetUniformLocation(e->iconProg, "uAlpha");
    const GLint aPos = glGetAttribLocation(e->iconProg, "aPos");
    const GLint aUV = glGetAttribLocation(e->iconProg, "aUV");
    const float q[4][5] = {
        {ic[0]-r[0]*s-up[0]*s, ic[1]-r[1]*s-up[1]*s,
         ic[2]-r[2]*s-up[2]*s, 0.0f, 1.0f},
        {ic[0]+r[0]*s-up[0]*s, ic[1]+r[1]*s-up[1]*s,
         ic[2]+r[2]*s-up[2]*s, 1.0f, 1.0f},
        {ic[0]+r[0]*s+up[0]*s, ic[1]+r[1]*s+up[1]*s,
         ic[2]+r[2]*s+up[2]*s, 1.0f, 0.0f},
        {ic[0]-r[0]*s+up[0]*s, ic[1]-r[1]*s+up[1]*s,
         ic[2]-r[2]*s+up[2]*s, 0.0f, 0.0f},
    };
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t)
        memcpy(verts + t*5, q[tris[t]], 20);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp.m);
    glUniform2f(uHalf, s, s);
    glUniform1f(uRad, s * kIconRad);
    glUniform1f(uAl, alpha);
    glUniform3f(glGetUniformLocation(e->iconProg, "uClip"),
                clipY, clipC, clipH);
    glUniform1i(uTex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts,
                 GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20,
                          (void*)0);
    glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 20,
                          (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    glUseProgram(e->shapeProg);
}

void drawDock(HudEngine* e, const Mat4& vp) {
    if (e->dock.empty() || e->dockHW <= 0.0f) return;
    // summon: the strip slides up from just under its rest position while
    // fading in; the same drop feeds the pick path so the aim matches
    const float sumT = progT(e->summonMs, e->frameMs, kDashMs);
    const float a = dashAlpha(sumT);
    const float drop = dashDrop(sumT);
    float c[3], r[3], up[3];
    dockCenterDrop(e->dockYaw, e->dockPitch, drop, e->ringPos, c, r, up);
    const float hw = e->dockHW, hh = kDockBarH * 0.5f;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUseProgram(e->shapeProg);
    // shadow under the strip: the box is the bar's own silhouette so only
    // the soft falloff reaches past it - a padded box reads as a dark slab
    const float shc[3] = {c[0] - up[0] * 0.024f, c[1] - up[1] * 0.024f,
                          c[2] - up[2] * 0.024f};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * a};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.06f, hh + 0.06f,
              hw, hh, hh, -1.0f, 0.06f, shCol);
    // the bar itself: frosted glass, top-lit, the scene bleeding through
    const float barCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                             0.55f * a};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, hh, 0.0f,
              0.003f, barCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.16f);
    // hairline rim so the glass edge reads crisp at every pitch
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                             0.14f * a};
    shapeQuad(e, vp, c, r, up, 0.005f, 0.0f, hw, hh, hw, hh, hh,
              0.0016f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.30f);
    // group separators
    const float sepCol[4] = {kPalText[0], kPalText[1],
                             kPalText[2], 0.10f * a};
    for (auto& it : e->dock) {
        if (!it.sep) continue;
        const float sx = it.x - kDockIconHW - (kDockGap + kDockSepW) * 0.5f;
        const float sc[3] = {c[0] + r[0] * sx, c[1] + r[1] * sx,
                             c[2] + r[2] * sx};
        shapeQuad(e, vp, sc, r, up, 0.006f, 0.0f, 0.0012f, hh * 0.55f,
                  0.0012f, hh * 0.55f, 0.0012f, 0.0f, 0.0015f, sepCol);
    }

    // status cluster on the left in two pills: clock + battery + wifi in
    // the first, the bell alone in the second, then the separator that
    // splits them off the app icons. Positions came out of dockLayout,
    // state out of the last bridge pull
    const DockStatus& st = e->dockSys;
    // the clock pill doubles as the quick-panel button: it brightens
    // under the gaze like the move handle does
    const float* pc = e->dockZone == DZONE_SYS ? kPalSurfaceHigh
                                             : kPalSurface;
    const float pillACol[4] = {pc[0], pc[1], pc[2], 0.55f * a};
    const float pillCol[4] = {kPalSurface[0], kPalSurface[1],
                              kPalSurface[2], 0.55f * a};
    {
        const float ax = (st.pillAL + st.pillAR) * 0.5f;
        const float aw = (st.pillAR - st.pillAL) * 0.5f;
        const float ac[3] = {c[0] + r[0] * ax, c[1] + r[1] * ax,
                             c[2] + r[2] * ax};
        shapeQuad(e, vp, ac, r, up, 0.006f, 0.0f, aw, kSysPillHH,
                  aw, kSysPillHH, kSysPillHH * 0.42f, 0.0f, 0.002f,
                  pillACol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.10f);
        const float bx = (st.pillBL + st.pillBR) * 0.5f;
        const float bw = (st.pillBR - st.pillBL) * 0.5f;
        const float bc[3] = {c[0] + r[0] * bx, c[1] + r[1] * bx,
                             c[2] + r[2] * bx};
        shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, bw, kSysPillHH,
                  bw, kSysPillHH, kSysPillHH * 0.42f, 0.0f, 0.002f, pillCol,
                  -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.10f);
    }
    if (!e->dock.empty()) {
        const float sc[3] = {c[0] + r[0] * st.sepX, c[1] + r[1] * st.sepX,
                             c[2] + r[2] * st.sepX};
        shapeQuad(e, vp, sc, r, up, 0.006f, 0.0f, 0.0012f, hh * 0.55f,
                  0.0012f, hh * 0.55f, 0.0012f, 0.0f, 0.0015f, sepCol);
    }

    // clock, vertically centred on the bar like the window labels
    if (e->font.ok) {
        float gt, gb, yo = 0.0f;
        if (textBounds(e->font.set, e->sysClock, kSysPx, &gt, &gb))
            yo = -(gt + gb) * 0.5f;
        float co[3] = {c[0] + r[0] * st.clockX + up[0] * yo,
                       c[1] + r[1] * st.clockX + up[1] * yo,
                       c[2] + r[2] * st.clockX + up[2] * yo};
        co[0] -= c[0] * 0.010f; co[1] -= c[1] * 0.010f; co[2] -= c[2] * 0.010f;
        glUseProgram(e->textProg);
        glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                           1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    kPalText[0], kPalText[1], kPalText[2]);
        glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->font.tex);
        drawTextPanel(e, e->sysClock, co, r, up, kSysPx, 0.0f, a);
        glUseProgram(e->shapeProg);
    }

    // the net slot: wifi unless a cable is patched in, dimmed while the
    // uplink reports down
    {
        const DockNetIcon ni = dockNetIcon(e->sysNet);
        const float wcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               (ni.lit ? 0.92f : 0.25f) * a};
        const float wc[3] = {c[0] + r[0]*st.wifiX, c[1] + r[1]*st.wifiX,
                             c[2] + r[2]*st.wifiX};
        drawGlyph(e, vp, ni.glyph == DNET_ETH ? GI_LAN : GI_WIFI,
                  wc, r, up, 0.032f, 0.008f, wcol);
    }

    // battery: outline body, level fill inside, tip nub on the right;
    // the fill runs the quick-panel's tint bands. No slot at all when the
    // device reports no battery
    if (st.hasBatt) {
        const float bw = 0.044f, bh = 0.024f, bt = 0.0030f;
        const float bc[3] = {c[0] + r[0]*st.battX, c[1] + r[1]*st.battX,
                             c[2] + r[2]*st.battX};
        const float ocol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.80f * a};
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.0f, bw * 0.5f + 0.006f,
                  bh * 0.5f + 0.006f, bw * 0.5f, bh * 0.5f, bh * 0.30f,
                  bt, 0.002f, ocol);
        float lvl = e->sysBatt / 100.0f;
        if (lvl < 0.0f) lvl = 0.0f; else if (lvl > 1.0f) lvl = 1.0f;
        const float fw = (bw - bt * 4.0f) * lvl, fhh = bh * 0.5f - bt * 2.0f;
        // the fill hugs the body's left inner edge
        const float fx = -(bw * 0.5f) + bt * 2.0f + fw * 0.5f;
        const float fc[3] = {bc[0] + r[0] * fx, bc[1] + r[1] * fx,
                             bc[2] + r[2] * fx};
        const float* fill = batteryTint(e->sysBatt, e->sysChg != 0);
        const float fcol[4] = {fill[0], fill[1], fill[2], 0.85f * a};
        if (fw > 0.001f)
            shapeQuad(e, vp, fc, r, up, 0.008f, 0.0f, fw * 0.5f, fhh,
                      fw * 0.5f, fhh, fhh * 0.3f, 0.0f, 0.0015f, fcol);
        const float tc[3] = {c[0] + r[0]*(st.battX + bw * 0.5f + 0.004f),
                             c[1] + r[1]*(st.battX + bw * 0.5f + 0.004f),
                             c[2] + r[2]*(st.battX + bw * 0.5f + 0.004f)};
        shapeQuad(e, vp, tc, r, up, 0.008f, 0.0f, 0.0025f, bh * 0.18f,
                  0.0025f, bh * 0.18f, 0.002f, 0.0f, 0.0012f, ocol);
    }

    // bell: dim while the shade is empty, a red shoulder dot while
    // notifications wait
    {
        const bool any = !e->notifsAll.empty();
        const float bcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               (any ? 0.90f : 0.30f) * a};
        const float bx = st.bellX;
        const float bc[3] = {c[0] + r[0]*bx, c[1] + r[1]*bx,
                             c[2] + r[2]*bx};
        drawGlyph(e, vp, GI_NOTIFICATIONS, bc, r, up, 0.024f, 0.008f,
                  bcol);
        if (any) {
            const float dc[3] = {c[0] + r[0]*(bx + 0.014f) + up[0]*0.018f,
                                 c[1] + r[1]*(bx + 0.014f) + up[1]*0.018f,
                                 c[2] + r[2]*(bx + 0.014f) + up[2]*0.018f};
            const float dcol[4] = {kPalDanger[0], kPalDanger[1],
                                   kPalDanger[2], 0.95f * a};
            shapeQuad(e, vp, dc, r, up, 0.009f, 0.0f, 0.007f, 0.007f,
                      0.006f, 0.006f, 0.006f, 0.0f, 0.0015f, dcol);
        }
    }

    for (int i = 0; i < (int)e->dock.size(); ++i) {
        const DockItem& it = e->dock[i];
        const bool hov = e->dockHover == i;
        // hs is the smoothed scale; hp is its 0..1 form for the extras
        // that fade with the icon instead of popping on the flag
        const float hp = hoverP(it.hs, kHoverScale);
        const float s = kDockIconHW * it.hs;
        const float ic[3] = {c[0] + r[0]*it.x + up[0]*kDockIconY,
                             c[1] + r[1]*it.x + up[1]*kDockIconY,
                             c[2] + r[2]*it.x + up[2]*kDockIconY};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;

        if (hp > 0.02f) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.16f * hp * a};
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, s + 0.018f,
                      s + 0.018f, s + 0.018f, s + 0.018f,
                      (s + 0.018f) * kIconRad, 0.0f, 0.002f, hl);
        }

        const float alpha = (it.minimized ? 0.45f : 1.0f) * a;
        if (it.kind == DK_GRID) {
            // the app-grid glyph: lit while the overlay is open
            const float* dc = e->grid.shown ? kPalAccent : kPalText;
            const float dcol[4] = {dc[0], dc[1], dc[2],
                                   (e->grid.shown ? 0.95f
                                                  : 0.72f + 0.23f * hp)
                                       * a};
            drawGlyph(e, vp, GI_APPS, ic, r, up, s * 0.72f, 0.008f, dcol);
        } else if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, s, icon->tex, alpha);
        } else {
            glUseProgram(e->shapeProg);
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str(),
                           0.0f, 0.0f, 0.0f, alpha);
            glUseProgram(e->shapeProg);
        }
        if (it.minimized) {
            const float dim[4] = {0.0f, 0.0f, 0.0f, 0.35f * a};
            shapeQuad(e, vp, ic, r, up, 0.010f, 0.0f, s, s, s, s,
                      s * kIconRad, 0.0f, 0.002f, dim);
        }

        // immersive marker: an amber ring around live/pinned XR items
        if (it.vr) {
            const float vc[4] = {kPalWarn[0], kPalWarn[1], kPalWarn[2],
                                 (0.65f + 0.30f * hp) * a};
            shapeQuad(e, vp, ic, r, up, 0.009f, 0.0f, s + 0.006f,
                      s + 0.006f, s + 0.006f, s + 0.006f, s + 0.006f,
                      0.0018f, 0.0015f, vc);
        }

        // running dot under the icon: clamped inside the bar so a
        // magnified icon can't push it past the bottom edge
        if (it.running) {
            const float doff = dockDotOff(s);
            const float dc[3] = {ic[0] - up[0] * doff,
                                 ic[1] - up[1] * doff,
                                 ic[2] - up[2] * doff};
            const float* dc2 = it.vr ? kPalWarn : kPalText;
            const float dcol[4] = {dc2[0], dc2[1], dc2[2],
                                   (it.minimized ? 0.4f : 0.85f) * a};
            shapeQuad(e, vp, dc, r, up, 0.008f, 0.0f, 0.007f, 0.007f,
                      0.007f, 0.007f, 0.007f, 0.0f, 0.0015f, dcol);
        }

        // close badge on a live immersive item: a disc off the icon's
        // top-right corner
        if (it.vr && it.running) {
            const float bo = s * 0.72f;
            const float bc[3] = {c[0] + r[0]*(it.x + bo) + up[0]*(kDockIconY + bo),
                                 c[1] + r[1]*(it.x + bo) + up[1]*(kDockIconY + bo),
                                 c[2] + r[2]*(it.x + bo) + up[2]*(kDockIconY + bo)};
            const bool bhov = hov && e->dockZone == DZONE_CLOSE;
            const float* bg2 = bhov ? kPalDanger : kPalSurfaceHigh;
            const float bcol[4] = {bg2[0], bg2[1], bg2[2], 0.92f * a};
            shapeQuad(e, vp, bc, r, up, 0.011f, 0.0f, kDockBadgeR,
                      kDockBadgeR, kDockBadgeR, kDockBadgeR, kDockBadgeR,
                      0.0f, 0.0015f, bcol);
            const float xcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                   0.95f * a};
            drawGlyph(e, vp, GI_CLOSE, bc, r, up, kDockBadgeR * 0.52f,
                      0.012f, xcol);
        }

        // pin-hold fill: an accent ring tightening around the icon
        if (e->dockPress == i && e->dockPinP > 0.0f && dockPinnable(it)) {
            const float pr[4] = {kPalAccent[0], kPalAccent[1], kPalAccent[2],
                                 (0.25f + 0.75f * e->dockPinP) * a};
            shapeQuad(e, vp, ic, r, up, 0.013f, 0.0f, s + 0.014f,
                      s + 0.014f, s + 0.014f, s + 0.014f, s + 0.014f,
                      0.0022f + 0.002f * e->dockPinP, 0.0015f, pr);
        }

        // label above the strip while the item is hovered
        if (hov && !it.label.empty() && e->font.ok) {
            const float ts = 0.0016f;
            const float tw = measureText(e, it.label.c_str(), ts) * 0.5f;
            float o[3] = {c[0] + r[0]*(it.x - tw) + up[0]*(hh + 0.05f),
                          c[1] + r[1]*(it.x - tw) + up[1]*(hh + 0.05f),
                          c[2] + r[2]*(it.x - tw) + up[2]*(hh + 0.05f)};
            o[0] -= c[0] * 0.010f;
            o[1] -= c[1] * 0.010f;
            o[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, it.label.c_str(), o, r, up, ts, 0.0f, a);
            glUseProgram(e->shapeProg);
        }
    }

    // move pill under the strip: holding it drags the whole dash, so it
    // brightens while gazed or held
    drawMovePill(e, vp, c, r, up, movePillDrop(kDockBarH * 0.5f),
                 e->dockZone == DZONE_HANDLE || e->moveHeld, a);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawShelf(HudEngine* e, const Mat4& vp) {
    if (e->shelf.empty() || e->shelfHW <= 0.0f) return;
    // the pill rides the strip's summon slide/fade with it
    const float sumT = progT(e->summonMs, e->frameMs, kDashMs);
    const float a = dashAlpha(sumT);
    const float drop = dashDrop(sumT);
    float c[3], r[3], up[3];
    shelfCenterDrop(e->dockYaw, e->dockPitch, drop, e->ringPos, c, r, up);
    const float hw = e->shelfHW;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUseProgram(e->shapeProg);
    // shadow + pill: the strip's recipe shrunk to a one-row tray
    const float shc[3] = {c[0] - up[0] * 0.016f, c[1] - up[1] * 0.016f,
                          c[2] - up[2] * 0.016f};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * a};
    shapeQuad(e, vp, shc, r, up, -0.024f, 0.0f, hw + 0.04f,
              kShelfHH + 0.04f, hw, kShelfHH, kShelfHH, -1.0f, 0.04f,
              shCol);
    const float pillCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                              0.62f * a};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, kShelfHH, hw, kShelfHH,
              kShelfHH, 0.0f, 0.003f, pillCol, -1.0f, 0.0f, 0.0f, 0.0f,
              0.0f, 0.07f);
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                             0.14f * a};
    shapeQuad(e, vp, c, r, up, 0.005f, 0.0f, hw, kShelfHH, hw, kShelfHH,
              kShelfHH, 0.0016f, 0.0012f, rimCol);

    for (int i = 0; i < (int)e->shelf.size(); ++i) {
        const ShelfItem& it = e->shelf[i];
        const bool hov = e->shelfHover == i;
        const float hp = hoverP(it.hs, kHoverScale);
        const float s = kShelfIconHW * it.hs;
        const float ic[3] = {c[0] + r[0]*it.x, c[1] + r[1]*it.x,
                             c[2] + r[2]*it.x};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;

        // the parked icon's alpha tracks its window's flight: it fades in
        // as the window shrinks onto the slot and back out on a restore,
        // so the icon only reads solid once the window has landed
        float ia = a;
        if (it.panelIdx >= 0 && it.panelIdx < (int)e->panels.size())
            ia *= e->panels[it.panelIdx].minT;

        if (hp > 0.02f) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.16f * hp * a};
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, s + 0.014f,
                      s + 0.014f, s + 0.014f, s + 0.014f,
                      (s + 0.014f) * kIconRad, 0.0f, 0.002f, hl);
        }
        if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, s, icon->tex, ia);
        } else {
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str(),
                           0.0f, 0.0f, 0.0f, ia);
            glUseProgram(e->shapeProg);
        }

        // label above the pill while the icon is hovered
        if (hov && !it.label.empty() && e->font.ok) {
            const float ts = 0.0016f;
            const float tw = measureText(e, it.label.c_str(), ts) * 0.5f;
            float o[3] = {c[0] + r[0]*(it.x - tw) +
                          up[0]*(kShelfHH + 0.04f),
                          c[1] + r[1]*(it.x - tw) +
                          up[1]*(kShelfHH + 0.04f),
                          c[2] + r[2]*(it.x - tw) +
                          up[2]*(kShelfHH + 0.04f)};
            o[0] -= c[0] * 0.010f;
            o[1] -= c[1] * 0.010f;
            o[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, it.label.c_str(), o, r, up, ts, 0.0f, a);
            glUseProgram(e->shapeProg);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
