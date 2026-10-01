#include "scene.h"

#include "../engine.h"
#include "../common/config.h"
#include "../common/props.h"
#include "../render/pt_geo.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>
#include <vector>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// camera intrinsics from /persist/pvr/camera/device_calibration.xml on the
// shipping device: two 640x400 fisheye halves in the one 1280x400 frame.
// The right sensor's rig rotation is applied so both eyes see straight.
static const CamIntr kPtCams[2] = {
    {285.48572f, 285.48572f, 319.78693f, 207.4184f,
     -0.0084880358f, 0.0071489909f, 0.0024346174f, -0.0022024566f,
     {1,0,0, 0,1,0, 0,0,1}},
    {283.59317f, 283.59317f, 319.61478f, 208.24164f,
     -0.0043927087f, 0.0051424201f, 0.0041712555f, -0.0028565519f,
     {0.99986655f, 0.016029568f, -0.003151021f,
      -0.01605548f, 0.99983603f, -0.0083779357f,
      0.0030162097f, 0.0084274104f, 0.99995995f}},
};

// builds both eye meshes once, and again whenever the tuning props move -
// debug.vrhome.pt{swap,flipx,flipy,roll} exist to find the mount's real
// orientation without a rebuild
static void ptMeshes(Engine* e, float tanX, float tanY) {
    static int pSwap = -1, pFx = -1, pFy = -1, pRot = -1;
    static float pRoll = -9999.0f;
    const int sw = propI("debug.vrhome.ptswap", 0);
    const int fx = propI("debug.vrhome.ptflipx", 1);
    const int fy = propI("debug.vrhome.ptflipy", 0);
    const float roll = propF("debug.vrhome.ptroll", 0.0f);
    const int rot = propI("debug.vrhome.ptrot", 1);
    if (e->ptVbo[0] && sw == pSwap && fx == pFx && fy == pFy &&
            roll == pRoll && rot == pRot)
        return;
    if (!e->ptVbo[0]) glGenBuffers(2, e->ptVbo);
    const int cols = 40, rows = 30;
    std::vector<float> verts(ptMeshFloats(cols, rows));
    for (int i = 0; i < 2; ++i) {
        const int n = buildPtMesh(verts.data(), (int)verts.size(), i,
                                  kPtCams, cols, rows, tanX, tanY,
                                  sw != 0, fx != 0, fy != 0, roll, rot);
        glBindBuffer(GL_ARRAY_BUFFER, e->ptVbo[i]);
        glBufferData(GL_ARRAY_BUFFER, n * 5 * sizeof(float),
                     verts.data(), GL_STATIC_DRAW);
        e->ptVerts[i] = n;
    }
    pSwap = sw; pFx = fx; pFy = fy; pRoll = roll; pRot = rot;
}

// one eye's passthrough mesh fills the eye buffer: NDC-space grid, the
// external texture sampled through the SurfaceTexture matrix. Drawn at
// depth 0 with depth test off - it replaces the backdrop entirely
static void drawPt(Engine* e, float fovY, float aspect) {
    const float tanY = tanf(fovY * (float)M_PI / 360.0f);
    ptMeshes(e, tanY * aspect, tanY);
    glUseProgram(e->floatProg);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
    glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"), 0.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadiusB"), 0.0f);
    const Mat4 id = identity();
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uMVP"),
                       1, GL_FALSE, id.m);
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uST"),
                       1, GL_FALSE, e->ptMat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->ptTex);
    glBindBuffer(GL_ARRAY_BUFFER, e->ptVbo[e->curEye]);
    const GLint aPos = glGetAttribLocation(e->floatProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->floatProg, "aUV");
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, e->ptVerts[e->curEye]);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    glEnable(GL_DEPTH_TEST);
}

// debug.vrhome.ptraw: draw the camera texture flat across the eye so the
// real frame layout is visible while bring-up tunes the warp
static void drawPtRaw(Engine* e) {
    glUseProgram(e->floatProg);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
    glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"), 0.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadiusB"), 0.0f);
    const Mat4 id = identity();
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uMVP"),
                       1, GL_FALSE, id.m);
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uST"),
                       1, GL_FALSE, e->ptMat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->ptTex);
    const float q[4][5] = {
        {-1,-1,0, 0,0}, {1,-1,0, 1,0}, {1,1,0, 1,1}, {-1,1,0, 0,1}};
    const int tris[6] = {0,1,2, 0,2,3};
    float verts[30];
    for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    const GLint aPos = glGetAttribLocation(e->floatProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->floatProg, "aUV");
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    glEnable(GL_DEPTH_TEST);
}

void drawScene(Engine* e, const Mat4& viewProj) {
    // live camera: the fisheye mesh IS the scene, no sky behind it
    if (e->ptLive && e->ptTex && propI("debug.vrhome.ptraw", 0)) {
        drawPtRaw(e);
        return;
    }
    if (e->ptLive && e->ptTex) {
        const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
        drawPt(e, propF("debug.vrhome.fov", kFovY), aspect);
        return;
    }
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
}
