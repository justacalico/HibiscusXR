#include "chrome.h"

#include "../hud/engine.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../text/draw.h"

#include <cmath>
#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// one rounded quad on a panel's plane centred at c, spanned by r and up.
// toward>0 shifts it toward the viewer so layered chrome never z-fights the
// surface under it. ang rotates the quad inside the panel's plane; the shape
// stays defined in the quad's own frame, so a rotated capsule reads as a
// rotated capsule
static void shapeQuad(HudEngine* e, const Mat4& vp, const float c[3],
                      const float r[3], const float up[3],
                      float toward, float ang,
                      float qw, float qh, float bw, float bh,
                      float radius, float border, float soft,
                      const float col[4]) {
    const GLint uMVP    = glGetUniformLocation(e->shapeProg, "uMVP");
    const GLint uQuad   = glGetUniformLocation(e->shapeProg, "uQuad");
    const GLint uBox    = glGetUniformLocation(e->shapeProg, "uBox");
    const GLint uRadius = glGetUniformLocation(e->shapeProg, "uRadius");
    const GLint uBorder = glGetUniformLocation(e->shapeProg, "uBorder");
    const GLint uSoft   = glGetUniformLocation(e->shapeProg, "uSoft");
    const GLint uColor  = glGetUniformLocation(e->shapeProg, "uColor");
    const GLint aPos    = glGetAttribLocation(e->shapeProg, "aPos");
    const GLint aUV     = glGetAttribLocation(e->shapeProg, "aUV");
    const float ca = cosf(ang), sa = sinf(ang);
    const float sx[4] = {-qw, qw, qw, -qw};
    const float sy[4] = {-qh, -qh, qh, qh};
    float q[4][5];
    for (int i = 0; i < 4; ++i) {
        const float ox = sx[i]*ca - sy[i]*sa;
        const float oy = sx[i]*sa + sy[i]*ca;
        q[i][0] = c[0] + r[0]*ox + up[0]*oy - c[0]*toward;
        q[i][1] = c[1] + r[1]*ox + up[1]*oy - c[1]*toward;
        q[i][2] = c[2] + r[2]*ox + up[2]*oy - c[2]*toward;
        q[i][3] = sx[i] > 0.0f ? 1.0f : -1.0f;
        q[i][4] = sy[i] > 0.0f ? 1.0f : -1.0f;
    }
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp.m);
    glUniform2f(uQuad, qw, qh);
    glUniform2f(uBox, bw, bh);
    glUniform1f(uRadius, radius);
    glUniform1f(uBorder, border);
    glUniform1f(uSoft, soft);
    glUniform4fv(uColor, 1, col);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
}

void drawPanels(HudEngine* e, const Mat4& viewProj) {
    if (e->panels.empty()) return;
    const float hw = kPanelW / 2, hh = kPanelH / 2;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // shadows first: they sit behind the panels and must not cover a
    // neighbouring window
    glUseProgram(e->shapeProg);
    for (auto& p : e->panels) {
        if (p.minimized) continue;
        float c[3], r[3], up[3];
        panelCenter(p, e->ringPos, c, r, up);
        const float shw = hw + 0.10f, shh = (hh + kBarGap + kBarH) + 0.10f;
        const float shd = (kBarGap + kBarH) * 0.5f + 0.02f;
        const float shc[3] = {c[0] - up[0] * shd, c[1] - up[1] * shd,
                              c[2] - up[2] * shd};
        const float col[4] = {0.0f, 0.0f, 0.0f, 0.36f};
        shapeQuad(e, viewProj, shc, r, up, -0.03f, 0.0f, shw, shh,
                  shw - 0.10f, shh - 0.10f, 0.10f, -1.0f, 0.10f, col);
    }

    const int mid = middleIndex(e->panels);
    for (int i = 0; i < (int)e->panels.size(); ++i) {
        Panel& p = e->panels[i];
        if (p.minimized) continue;
        float c[3], r[3], up[3];
        panelCenter(p, e->ringPos, c, r, up);
        const bool hov = (e->hover == i);
        // the library panel is the shell's own launcher: no window buttons
        const bool btns = p.pkg != kLibraryPkg;

        // the label is measured first: the pill under the window hugs the
        // text instead of spanning the window's full width
        float s = 0.0014f, bold = 0.0f, w = 0.0f;
        if (!p.label.empty() && e->font.ok) {
            bold = 0.8f * s;
            w = measureText(e, p.label.c_str(), s) + bold;
            if (w > pillTextLimit(hw, btns)) {
                s *= pillTextLimit(hw, btns) / w;
                bold = 0.8f * s;
                w = measureText(e, p.label.c_str(), s) + bold;
            }
        }

        // bottom bar: dark pill under the window with the app label, sized
        // to the text plus the button strip, capped inside the window's
        // edges. pillHW is stored so the gaze pick can hit-test the buttons
        const float barOff = -(hh + kBarGap + kBarH * 0.5f);
        const float barHW = pillHalfWidth(w, hw, btns);
        p.pillHW = barHW;
        const float barCol[4] = {hov ? 0.16f : 0.085f, hov ? 0.18f : 0.095f,
                                 hov ? 0.24f : 0.13f, hov ? 0.95f : 0.88f};
        const float barC[3] = {c[0] + up[0] * barOff, c[1] + up[1] * barOff,
                               c[2] + up[2] * barOff};
        glUseProgram(e->shapeProg);
        shapeQuad(e, viewProj, barC, r, up, 0.004f, 0.0f, barHW, kBarH * 0.5f,
                  barHW, kBarH * 0.5f, kBarH * 0.5f, 0.0f, 0.002f, barCol);

        // minimize + close discs on the pill's right end; glyphs are small
        // capsules, the close pair rotated into an x
        if (btns) {
            const float icon[4] = {1.0f, 1.0f, 1.0f, 0.92f};
            const float il = kPillBtnR * 0.55f, it = 0.0028f;
            for (int b = 0; b < 2; ++b) {
                const int zone = b == 0 ? ZONE_MIN : ZONE_CLOSE;
                const float bx = b == 0 ? pillMinX(barHW) : pillCloseX(barHW);
                const bool bhov = hov && e->hoverZone == zone;
                const float bc[3] = {c[0] + r[0]*bx + up[0]*barOff,
                                     c[1] + r[1]*bx + up[1]*barOff,
                                     c[2] + r[2]*bx + up[2]*barOff};
                const float bg[4] = {bhov && zone == ZONE_CLOSE ? 0.72f : 1.0f,
                                     bhov && zone == ZONE_CLOSE ? 0.25f : 1.0f,
                                     bhov && zone == ZONE_CLOSE ? 0.25f : 1.0f,
                                     bhov ? 0.32f : 0.13f};
                shapeQuad(e, viewProj, bc, r, up, 0.006f, 0.0f,
                          kPillBtnR, kPillBtnR, kPillBtnR, kPillBtnR,
                          kPillBtnR, 0.0f, 0.002f, bg);
                if (zone == ZONE_MIN) {
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

        // drag handle: a short white line centred under the middle window's
        // pill; holding it drags the whole ring, so brighten it while gazed
        if (i == mid) {
            const bool hhov = hov && e->hoverZone == ZONE_HANDLE;
            const float hd = handleDrop();
            const float hc[3] = {c[0] - up[0] * hd, c[1] - up[1] * hd,
                                 c[2] - up[2] * hd};
            const float hcol[4] = {1.0f, 1.0f, 1.0f, hhov ? 0.95f : 0.55f};
            shapeQuad(e, viewProj, hc, r, up, 0.006f, 0.0f, kHandleW, kHandleT,
                      kHandleW, kHandleT, kHandleT, 0.0f, 0.0015f, hcol);
        }

        // the app surface itself, corners rounded in the shader
        glUseProgram(e->floatProg);
        glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
        glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), hw, hh);
        glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"), kCornerR);
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

        // hairline border, brightened while gazed at
        glUseProgram(e->shapeProg);
        const float bdCol[4] = {1.0f, 1.0f, 1.0f, hov ? 0.55f : 0.14f};
        shapeQuad(e, viewProj, c, r, up, 0.006f, 0.0f, hw + 0.006f,
                  hh + 0.006f, hw + 0.006f, hh + 0.006f, kCornerR + 0.006f,
                  0.0016f, 0.0012f, bdCol);

        // app label centred in the bar's text region (the pill minus the
        // button strip), bold, shrunk to fit if the name is long. Centering
        // uses the real glyph bounds, not the font's nominal ascent, so
        // descenders don't push the text off-centre
        if (!p.label.empty() && e->font.ok) {
            float boff = barOff;
            float gt, gb;
            if (textBounds(e->font.set, p.label.c_str(), s, &gt, &gb))
                boff = barOff - (gt + gb) * 0.5f;
            const float tw = w * 0.5f + (btns ? kPillBtnW * 0.5f : 0.0f);
            float to[3] = {c[0] - r[0] * tw + up[0] * boff,
                           c[1] - r[1] * tw + up[1] * boff,
                           c[2] - r[2] * tw + up[2] * boff};
            to[0] -= c[0] * 0.010f; to[1] -= c[1] * 0.010f;
            to[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, viewProj.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        1.0f, 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, p.label.c_str(), to, r, up, s, bold);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
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
    const float col[4] = {1.0f, 1.0f, 1.0f, 0.95f};
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
    if (e->hover < 0 || e->hover >= (int)e->panels.size()) return;
    const Panel& p = e->panels[e->hover];
    float c[3], r[3], up[3];
    panelCenter(p, e->ringPos, c, r, up);
    const float u = e->hitX / kVdW * 2.0f - 1.0f;
    const float v = 1.0f - e->hitY / kVdH * 2.0f;
    const float hw = kPanelW / 2, hh = kPanelH / 2;
    float pos[3] = {c[0] + r[0]*u*hw + up[0]*v*hh,
                    c[1] + r[1]*u*hw + up[1]*v*hh,
                    c[2] + r[2]*u*hw + up[2]*v*hh};
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(e->shapeProg);
    const float ringCol[4] = {1.0f, 1.0f, 1.0f, 0.85f};
    const float dotCol[4]  = {1.0f, 1.0f, 1.0f, 0.90f};
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.014f, 0.014f, 0.014f,
              0.014f, 0.014f, 0.0016f, 0.001f, ringCol);
    shapeQuad(e, viewProj, pos, r, up, 0.012f, 0.0f, 0.005f, 0.005f, 0.005f,
              0.005f, 0.005f, 0.0f, 0.001f, dotCol);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
