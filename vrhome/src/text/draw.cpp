#include "draw.h"

#include "font.h"
#include "../engine.h"

#include <vector>

float measureText(Engine* e, const char* utf8, float mPerPx) {
    if (!e->font.ok) return 0;
    ensureGlyphs(e, utf8);
    return textWidth(e->font.set, utf8, mPerPx);
}

static void flushText(Engine* e, const float* v, int n) {
    const GLint aPos = glGetAttribLocation(e->textProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->textProg, "aUV");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glBindBuffer(GL_ARRAY_BUFFER, e->textVbo);
    glBufferData(GL_ARRAY_BUFFER, n * 5 * sizeof(float), v, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glDrawArrays(GL_TRIANGLES, 0, n);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
}

float drawText(Engine* e, const char* utf8, float x, float y, float z,
               float mPerPx) {
    if (!e->font.ok) return 0;
    ensureGlyphs(e, utf8);
    std::vector<float> lv;
    const float w = emitText(e->font.set, utf8, mPerPx, lv);
    if (lv.empty()) return 0;
    std::vector<float> v;
    liftText(lv.data(), (int)lv.size() / 4, x, y, z, v);
    flushText(e, v.data(), (int)(v.size() / 5));
    return w;
}

void drawTextPanel(Engine* e, const char* utf8, const float o[3],
                   const float r[3], float mPerPx) {
    if (!e->font.ok) return;
    ensureGlyphs(e, utf8);
    std::vector<float> lv;
    emitText(e->font.set, utf8, mPerPx, lv);
    if (lv.empty()) return;
    std::vector<float> v;
    liftTextPanel(lv.data(), (int)lv.size() / 4, o, r, v);
    flushText(e, v.data(), (int)(v.size() / 5));
}

void drawHud(Engine* e, const Mat4& proj) {
    if (!e->font.ok) return;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"), 1, GL_FALSE,
                     proj.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                0.55f, 0.60f, 0.68f);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float s = 0.0016f;
    const float z = -1.2f;
    float w = measureText(e, e->hud, s);
    drawText(e, e->hud, -w * 0.5f, 0.34f, z, s);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
