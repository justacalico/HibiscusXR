#include "sysmsg.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../render/shape.h"
#include "../text/draw.h"
#include "../text/glyphs.h"

#include <GLES2/gl2.h>
#include <cstring>
#include <string>
#include <vector>

void syncSysMsgs(HudEngine* e) {
    if (!e->backend) return;
    const int v = e->backend->sysMsgVersion();
    if (v == e->sysMsgVer) return;
    e->sysMsgVer = v;
    e->backend->sysMsgs(&e->sysMsgs);
    for (auto& m : e->sysMsgs)
        if (!m.pkg.empty()) iconFor(e, m.pkg);
}

void sysMsgBtnClick(HudEngine* e, int btn) {
    if (!e->backend || e->sysMsgs.empty()) return;
    e->backend->sysMsgClick(e->sysMsgs.front().id, btn);
    LOGI("sysmsg click btn %d", btn);
}

void sysMsgDismiss(HudEngine* e) {
    if (!e->backend || e->sysMsgs.empty()) return;
    e->backend->sysMsgDismiss(e->sysMsgs.front().id);
    LOGI("sysmsg dismiss");
}

// one text line on the card plane, clipped to a width - same recipe the
// notification cards use
static void msgText(HudEngine* e, const Mat4& vp, const char* utf8,
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

void drawSysMsg(HudEngine* e, const Mat4& vp, float yaw, float pitch,
                float lift) {
    if (e->sysMsgs.empty()) return;
    const SysMsgItem& it = e->sysMsgs.front();
    float cc[3], r[3], up[3];
    sysMsgCenter(yaw, pitch, lift, e->dashDist, e->ringPos, cc, r, up);
    const float hw = kSysMsgW * 0.5f;
    const float hh = kSysMsgH * 0.5f;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // drop shadow behind the card, then the body - same recipe as the
    // notification cards, tuned a touch darker since this one is modal
    const float shc[3] = {cc[0] - up[0] * 0.016f, cc[1] - up[1] * 0.016f,
                          cc[2] - up[2] * 0.016f};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.38f};
    shapeQuad(e, vp, shc, r, up, -0.012f, 0.0f, hw + 0.04f, hh + 0.04f,
              hw, hh, 0.034f, -1.0f, 0.04f, shCol);
    const float body[4] = {kPalSurface[0], kPalSurface[1], kPalSurface[2],
                           0.80f};
    shapeQuad(e, vp, cc, r, up, 0.004f, 0.0f, hw, hh, hw, hh,
              0.034f, 0.0f, 0.0025f, body, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.14f);
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2], 0.16f};
    shapeQuad(e, vp, cc, r, up, 0.005f, 0.0f, hw, hh, hw, hh,
              0.034f, 0.0015f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f, 0.0f,
              0.0f, 0.30f);
    // red accent down the card's left edge: this is an alert, not a post
    const float ax = -hw + 0.006f;
    const float ac[3] = {cc[0] + r[0] * ax, cc[1] + r[1] * ax,
                         cc[2] + r[2] * ax};
    const float acol[4] = {kPalDanger[0], kPalDanger[1], kPalDanger[2],
                           0.92f};
    shapeQuad(e, vp, ac, r, up, 0.008f, 0.0f, 0.003f, hh - 0.020f,
              0.003f, hh - 0.020f, 0.003f, 0.0f, 0.0015f, acol);

    // app icon top-left; a system-level crash has no pkg and gets the
    // letter tile's exclamation mark
    const float ix = -hw + kSysMsgPad + kSysMsgIconHW;
    const float iy = hh - kSysMsgPad - kSysMsgIconHW;
    const float ic[3] = {cc[0] + r[0] * ix + up[0] * iy,
                         cc[1] + r[1] * ix + up[1] * iy,
                         cc[2] + r[2] * ix + up[2] * iy};
    const DockIcon* icon = nullptr;
    if (!it.pkg.empty()) {
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;
    }
    if (icon && icon->tex) {
        drawIconTex(e, vp, ic, r, up, kSysMsgIconHW, icon->tex, 1.0f);
    } else {
        const char* lb = icon && !icon->label.empty()
                         ? icon->label.c_str()
                         : (it.pkg.empty() ? "!" : it.pkg.c_str());
        drawLetterTile(e, vp, ic, r, up, kSysMsgIconHW, lb);
        glUseProgram(e->shapeProg);
    }

    // title beside the icon, body lines under it - each clipped to the
    // text column, which ends at the card's right padding
    const float tx = ix + kSysMsgIconHW + 0.014f;
    const float textMax = hw - kSysMsgPad - tx;
    float to[3] = {cc[0] + r[0]*tx + up[0]*(iy + 0.010f),
                   cc[1] + r[1]*tx + up[1]*(iy + 0.010f),
                   cc[2] + r[2]*tx + up[2]*(iy + 0.010f)};
    to[0] -= to[0] * 0.010f; to[1] -= to[1] * 0.010f;
    to[2] -= to[2] * 0.010f;
    msgText(e, vp, it.title.c_str(), to, r, up, 0.0019f, textMax,
            kPalText[0], kPalText[1], kPalText[2]);

    // body text: up to kSysMsgMaxLines lines split on newlines, starting
    // under the icon row
    const float lx = -hw + kSysMsgPad + 0.010f;
    const float bodyMax = hw - kSysMsgPad - lx;
    float ly = iy - kSysMsgIconHW - 0.040f;
    size_t pos = 0;
    for (int ln = 0; ln < kSysMsgMaxLines && pos <= it.text.size(); ++ln) {
        const size_t nl = it.text.find('\n', pos);
        const std::string line = it.text.substr(pos,
                nl == std::string::npos ? nl : nl - pos);
        float bo[3] = {cc[0] + r[0]*lx + up[0]*ly,
                       cc[1] + r[1]*lx + up[1]*ly,
                       cc[2] + r[2]*lx + up[2]*ly};
        bo[0] -= bo[0] * 0.010f; bo[1] -= bo[1] * 0.010f;
        bo[2] -= bo[2] * 0.010f;
        msgText(e, vp, line.c_str(), bo, r, up, 0.0015f, bodyMax,
                kPalTextDim[0], kPalTextDim[1], kPalTextDim[2]);
        if (nl == std::string::npos) break;
        pos = nl + 1;
        ly -= 0.034f;
    }

    // button pills along the bottom row, labels centred inside
    const int nb = (int)it.buttons.size();
    const float by = sysMsgBtnY();
    for (int i = 0; i < nb; ++i) {
        const float bx = sysMsgBtnX(i, nb);
        const float bhw = sysMsgBtnHW(nb);
        const float bc[3] = {cc[0] + r[0]*bx + up[0]*by,
                             cc[1] + r[1]*bx + up[1]*by,
                             cc[2] + r[2]*bx + up[2]*by};
        const bool bhov = e->sysMsgHover >= 0 &&
                e->sysMsgZone == MZONE_BTN && e->sysMsgBtn == i;
        const float* bp = bhov ? kPalAccent : kPalSurfaceHigh;
        const float bcol[4] = {bp[0], bp[1], bp[2], 0.96f};
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.0f, bhw, kSysMsgBtnHH,
                  bhw, kSysMsgBtnHH, kSysMsgBtnHH, 0.0f, 0.002f, bcol);
        if (!it.buttons[i].empty() && e->font.ok) {
            const float ts = 0.0015f;
            // clip first so the centre offset matches what's drawn
            const std::string lab = clipText(e->font.set,
                    it.buttons[i].c_str(), ts, 2.0f * bhw - 0.016f);
            const float tw = measureText(e, lab.c_str(), ts) * 0.5f;
            float gt, gb;
            float yo = -kSysMsgBtnHH * 0.32f;
            if (textBounds(e->font.set, lab.c_str(), ts, &gt, &gb))
                yo = -(gt + gb) * 0.5f;
            float o[3] = {bc[0] - r[0]*tw + up[0]*yo,
                          bc[1] - r[1]*tw + up[1]*yo,
                          bc[2] - r[2]*tw + up[2]*yo};
            o[0] -= o[0] * 0.008f; o[1] -= o[1] * 0.008f;
            o[2] -= o[2] * 0.008f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, lab.c_str(), o, r, up, ts, 0.0f);
            glUseProgram(e->shapeProg);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
