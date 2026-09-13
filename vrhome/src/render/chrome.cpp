#include "chrome.h"

#include "../engine.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../text/draw.h"

#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// one rounded quad on a panel's plane centred at c. toward>0 shifts it toward
// the viewer so layered chrome never z-fights the surface under it
static void shapeQuad(Engine* e, const Mat4& vp, const float c[3],
                      const float r[3], float toward,
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
    const float q[4][5] = {
        {c[0]-r[0]*qw - c[0]*toward, c[1]-qh, c[2]-r[2]*qw - c[2]*toward, -1,-1},
        {c[0]+r[0]*qw - c[0]*toward, c[1]-qh, c[2]+r[2]*qw - c[2]*toward,  1,-1},
        {c[0]+r[0]*qw - c[0]*toward, c[1]+qh, c[2]+r[2]*qw - c[2]*toward,  1, 1},
        {c[0]-r[0]*qw - c[0]*toward, c[1]+qh, c[2]-r[2]*qw - c[2]*toward, -1, 1},
    };
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

void drawPanels(Engine* e, const Mat4& viewProj) {
    if (e->panels.empty()) return;
    const float hw = kPanelW / 2, hh = kPanelH / 2;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // shadows first: they sit behind the panels and must not cover a
    // neighbouring window
    glUseProgram(e->shapeProg);
    for (auto& p : e->panels) {
        float c[3], r[3];
        panelCenter(p, c, r);
        const float shw = hw + 0.10f, shh = (hh + kBarGap + kBarH) + 0.10f;
        const float shc[3] = {c[0], c[1] - (kBarGap + kBarH) * 0.5f - 0.02f,
                              c[2]};
        const float col[4] = {0.0f, 0.0f, 0.0f, 0.36f};
        shapeQuad(e, viewProj, shc, r, -0.03f, shw, shh,
                  shw - 0.10f, shh - 0.10f, 0.10f, -1.0f, 0.10f, col);
    }

    for (int i = 0; i < (int)e->panels.size(); ++i) {
        Panel& p = e->panels[i];
        float c[3], r[3];
        panelCenter(p, c, r);
        const bool hov = (e->hover == i);

        // bottom bar: dark pill under the window with the app label
        const float barY = c[1] - hh - kBarGap - kBarH * 0.5f;
        const float barCol[4] = {hov ? 0.16f : 0.085f, hov ? 0.18f : 0.095f,
                                 hov ? 0.24f : 0.13f, hov ? 0.95f : 0.88f};
        const float barC[3] = {c[0], barY, c[2]};
        glUseProgram(e->shapeProg);
        shapeQuad(e, viewProj, barC, r, 0.004f, hw, kBarH * 0.5f,
                  hw, kBarH * 0.5f, kBarH * 0.45f, 0.0f, 0.002f, barCol);

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
            {c[0]-r[0]*hw, c[1]-hh, c[2]-r[2]*hw, 0.0f, 0.0f},
            {c[0]+r[0]*hw, c[1]-hh, c[2]+r[2]*hw, 1.0f, 0.0f},
            {c[0]+r[0]*hw, c[1]+hh, c[2]+r[2]*hw, 1.0f, 1.0f},
            {c[0]-r[0]*hw, c[1]+hh, c[2]-r[2]*hw, 0.0f, 1.0f},
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
        shapeQuad(e, viewProj, c, r, 0.006f, hw + 0.006f, hh + 0.006f,
                  hw + 0.006f, hh + 0.006f, kCornerR + 0.006f,
                  0.0016f, 0.0012f, bdCol);

        // app label centred in the bar, bold, shrunk to fit if the name is
        // long. Centering uses the real glyph bounds, not the font's nominal
        // ascent, so descenders don't push the text off-centre
        if (!p.label.empty() && e->font.ok) {
            float s = 0.0014f;
            float bold = 0.8f * s;
            float w = measureText(e, p.label.c_str(), s) + bold;
            if (w > hw * 1.9f) {
                s *= hw * 1.9f / w;
                bold = 0.8f * s;
                w = measureText(e, p.label.c_str(), s) + bold;
            }
            float baseline = barY;
            float gt, gb;
            if (textBounds(e->font.set, p.label.c_str(), s, &gt, &gb))
                baseline = barY - (gt + gb) * 0.5f;
            float to[3] = {c[0] - r[0] * w * 0.5f, baseline,
                           c[2] - r[2] * w * 0.5f};
            to[0] -= c[0] * 0.010f; to[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, viewProj.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        1.0f, 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, p.label.c_str(), to, r, s, bold);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawCursor(Engine* e, const Mat4& viewProj) {
    if (e->hover < 0 || e->hover >= (int)e->panels.size()) return;
    const Panel& p = e->panels[e->hover];
    float c[3], r[3];
    panelCenter(p, c, r);
    const float u = e->hitX / kVdW * 2.0f - 1.0f;
    const float v = 1.0f - e->hitY / kVdH * 2.0f;
    const float hw = kPanelW / 2, hh = kPanelH / 2;
    float pos[3] = {c[0] + r[0]*u*hw, c[1] + v*hh, c[2] + r[2]*u*hw};
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(e->shapeProg);
    const float ringCol[4] = {1.0f, 1.0f, 1.0f, 0.85f};
    const float dotCol[4]  = {1.0f, 1.0f, 1.0f, 0.90f};
    shapeQuad(e, viewProj, pos, r, 0.012f, 0.014f, 0.014f, 0.014f, 0.014f,
              0.014f, 0.0016f, 0.001f, ringCol);
    shapeQuad(e, viewProj, pos, r, 0.012f, 0.005f, 0.005f, 0.005f, 0.005f,
              0.005f, 0.0f, 0.001f, dotCol);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
