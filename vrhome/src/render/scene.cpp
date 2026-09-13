#include "scene.h"

#include "chrome.h"
#include "../engine.h"

void drawScene(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);

    // sky first, no depth write - it's the backdrop everything sits on
    glDepthMask(GL_FALSE);
    glBindBuffer(GL_ARRAY_BUFFER, e->skyVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, e->skyVerts);
    glDepthMask(GL_TRUE);

    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glDrawArrays(GL_LINES, 0, e->gridVerts);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);

    drawPanels(e, viewProj);
    drawCursor(e, viewProj);
}
