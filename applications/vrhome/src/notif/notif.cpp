#include "notif.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../render/icons.h"
#include "../render/shape.h"
#include "../text/draw.h"
#include "../text/glyphs.h"

#include <GLES2/gl2.h>
#include <cmath>
#include <cstring>

void syncNotifs(HudEngine* e) {
    if (!e->backend) return;
    const int v = e->backend->notifVersion();
    if (v == e->notifVer) return;
    e->notifVer = v;
    std::vector<NotifItem> raw;
    e->backend->notifs(&raw);
    e->notifsAll = buildNotifs(raw);
    for (auto& n : e->notifsAll) iconFor(e, n.pkg);
}

void notifDismiss(HudEngine* e, int idx) {
    if (!e->backend || idx < 0 || idx >= (int)e->notifs.size()) return;
    const NotifItem& n = e->notifs[idx];
    if (!n.clearable) return;
    e->backend->dismissNotif(n.key);
    LOGI("notif dismiss %s", n.pkg.c_str());
}

// one text line on the card plane, clipped to the text column
static void cardText(HudEngine* e, const Mat4& vp, const char* utf8,
                     const float o[3], const float r[3], const float up[3],
                     float mPerPx, float maxW, float cr, float cg, float cb) {
    if (!*utf8 || !e->font.ok) return;
    const std::string line = clipText(e->font.set, utf8, mPerPx, maxW);
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                       1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"), cr, cg, cb);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    drawTextPanel(e, line.c_str(), o, r, up, mPerPx, 0.0f);
    glUseProgram(e->shapeProg);
}

void drawNotifStack(HudEngine* e, const Mat4& vp, float yaw, float pitch,
                    float lift) {
    const int n = (int)e->notifs.size();
    if (n <= 0) return;
    float c[3], r[3], up[3];
    notifCenter(yaw, pitch, lift, e->dashDist, e->ringPos, c, r, up);
    const float hw = kNotifCardW * 0.5f;
    const float hh = kNotifCardH * 0.5f;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    for (int i = 0; i < n; ++i) {
        const NotifItem& it = e->notifs[i];
        const float yc = notifCardY(i, n);
        const float cc[3] = {c[0] + up[0] * yc, c[1] + up[1] * yc,
                             c[2] + up[2] * yc};
        const bool hov = e->notifHover == i;

        // drop shadow behind the card: box is the card silhouette so only
        // the falloff reaches past it
        const float shc[3] = {cc[0] - up[0] * 0.014f, cc[1] - up[1] * 0.014f,
                              cc[2] - up[2] * 0.014f};
        const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f};
        shapeQuad(e, vp, shc, r, up, -0.012f, 0.0f, hw + 0.04f, hh + 0.04f,
                  hw, hh, 0.032f, -1.0f, 0.04f, shCol);
        // card body; a hovered card brightens a touch
        const float body[4] = {kPalSurface[0], kPalSurface[1],
                               kPalSurface[2], hov ? 0.72f : 0.60f};
        shapeQuad(e, vp, cc, r, up, 0.004f, 0.0f, hw, hh, hw, hh,
                  0.032f, 0.0f, 0.0025f, body, -1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.14f);
        const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 hov ? 0.22f : 0.14f};
        shapeQuad(e, vp, cc, r, up, 0.005f, 0.0f, hw, hh, hw, hh,
                  0.032f, 0.0015f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.30f);
        if (hov) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.08f};
            shapeQuad(e, vp, cc, r, up, 0.006f, 0.0f, hw, hh, hw, hh,
                      0.032f, 0.0f, 0.002f, hl);
        }

        // app icon on the left; the dock's letter-tile fallback for pkgs
        // without one
        const float ix = -hw + kNotifPad + kNotifIconHW;
        const float ic[3] = {cc[0] + r[0] * ix, cc[1] + r[1] * ix,
                             cc[2] + r[2] * ix};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;
        if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, kNotifIconHW, icon->tex, 1.0f);
        } else {
            // no icon: a letter tile derived from the app label
            const char* lb = icon && !icon->label.empty()
                             ? icon->label.c_str() : it.pkg.c_str();
            drawLetterTile(e, vp, ic, r, up, kNotifIconHW, lb);
            glUseProgram(e->shapeProg);
        }

        // title + body in the text column; title falls back to the app
        // label so a bare notification still says who posted it
        const float tx = ix + kNotifIconHW + 0.014f;
        const float textMax = hw - kNotifPad - kNotifBadgeR * 2.0f - tx;
        const char* title = !it.title.empty() ? it.title.c_str()
            : (icon && !icon->label.empty() ? icon->label.c_str()
                                            : it.pkg.c_str());
        float to[3] = {cc[0] + r[0]*tx + up[0]*0.012f,
                       cc[1] + r[1]*tx + up[1]*0.012f,
                       cc[2] + r[2]*tx + up[2]*0.012f};
        to[0] -= cc[0] * 0.010f; to[1] -= cc[1] * 0.010f;
        to[2] -= cc[2] * 0.010f;
        cardText(e, vp, title, to, r, up, 0.0018f, textMax,
                 kPalText[0], kPalText[1], kPalText[2]);
        float bo[3] = {cc[0] + r[0]*tx - up[0]*0.026f,
                       cc[1] + r[1]*tx - up[1]*0.026f,
                       cc[2] + r[2]*tx - up[2]*0.026f};
        bo[0] -= cc[0] * 0.010f; bo[1] -= cc[1] * 0.010f;
        bo[2] -= cc[2] * 0.010f;
        cardText(e, vp, it.text.c_str(), bo, r, up, 0.0014f, textMax,
                 kPalTextDim[0], kPalTextDim[1], kPalTextDim[2]);

        // dismiss badge on a clearable card, top-right
        if (it.clearable) {
            float bx, by;
            notifBadgeAt(&bx, &by);
            const float bc[3] = {cc[0] + r[0]*bx + up[0]*by,
                                 cc[1] + r[1]*bx + up[1]*by,
                                 cc[2] + r[2]*bx + up[2]*by};
            const bool bhov = hov && e->notifZone == NZONE_CLOSE;
            const float* bgp = bhov ? kPalDanger : kPalSurfaceHigh;
            const float bcol[4] = {bgp[0], bgp[1], bgp[2], 0.92f};
            shapeQuad(e, vp, bc, r, up, 0.011f, 0.0f, kNotifBadgeR,
                      kNotifBadgeR, kNotifBadgeR, kNotifBadgeR,
                      kNotifBadgeR, 0.0f, 0.0015f, bcol);
            const float xcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                   0.95f};
            drawGlyph(e, vp, GI_CLOSE, bc, r, up, kNotifBadgeR * 0.52f,
                      0.012f, xcol);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
