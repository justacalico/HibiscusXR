#include "shape.h"

#include "../hud/engine.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>

void shapeQuad(HudEngine* e, const Mat4& vp, const float c[3],
               const float r[3], const float up[3],
               float toward, float ang, float qw, float qh,
               float bw, float bh, float radius, float border, float soft,
               const float col[4], float radB) {
    const GLint uMVP    = glGetUniformLocation(e->shapeProg, "uMVP");
    const GLint uQuad   = glGetUniformLocation(e->shapeProg, "uQuad");
    const GLint uBox    = glGetUniformLocation(e->shapeProg, "uBox");
    const GLint uRadius = glGetUniformLocation(e->shapeProg, "uRadius");
    const GLint uRadiusB = glGetUniformLocation(e->shapeProg, "uRadiusB");
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
    glUniform1f(uRadiusB, radB < 0.0f ? radius : radB);
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
