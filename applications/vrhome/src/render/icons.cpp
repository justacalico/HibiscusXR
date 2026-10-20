#include "icons.h"

#include "iconatlas.h"
#include "../hud/engine.h"

#include <GLES2/gl2.h>

#include <cstring>

void loadGlyphs(Engine* e) {
    if (e->glyphTex) return;
    glGenTextures(1, &e->glyphTex);
    glBindTexture(GL_TEXTURE_2D, e->glyphTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, iconAtlasW(), iconAtlasH(),
                 0, GL_ALPHA, GL_UNSIGNED_BYTE, iconAtlas());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void drawGlyph(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s, float toward,
               const float col[4]) {
    if (!e->glyphTex) return;
    float uv[4];
    if (!iconUv(id, uv)) return;
    glUseProgram(e->textProg);
    const GLint uMVP = glGetUniformLocation(e->textProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->textProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->textProg, "aUV");
    const float qs = s / 0.36f;
    const float q[4][5] = {
        {ic[0]-r[0]*qs-up[0]*qs - ic[0]*toward,
         ic[1]-r[1]*qs-up[1]*qs - ic[1]*toward,
         ic[2]-r[2]*qs-up[2]*qs - ic[2]*toward, uv[0], uv[3]},
        {ic[0]+r[0]*qs-up[0]*qs - ic[0]*toward,
         ic[1]+r[1]*qs-up[1]*qs - ic[1]*toward,
         ic[2]+r[2]*qs-up[2]*qs - ic[2]*toward, uv[2], uv[3]},
        {ic[0]+r[0]*qs+up[0]*qs - ic[0]*toward,
         ic[1]+r[1]*qs+up[1]*qs - ic[1]*toward,
         ic[2]+r[2]*qs+up[2]*qs - ic[2]*toward, uv[2], uv[1]},
        {ic[0]-r[0]*qs+up[0]*qs - ic[0]*toward,
         ic[1]-r[1]*qs+up[1]*qs - ic[1]*toward,
         ic[2]-r[2]*qs+up[2]*qs - ic[2]*toward, uv[0], uv[1]},
    };
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                col[0], col[1], col[2]);
    glUniform1f(glGetUniformLocation(e->textProg, "uAlpha"), col[3]);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->glyphTex);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    glUseProgram(e->shapeProg);
}
