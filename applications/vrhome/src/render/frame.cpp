#include "frame.h"

#include "warp.h"
#include "../engine.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../common/config.h"
#include "../math/head.h"
#include "../text/draw.h"

#include <cstdio>
#include <ctime>

void drawEyes(Engine* e, const Mat4& head, const Mat4& proj, bool translucent,
              bool status, void (*scene)(Engine*, const Mat4&)) {
    static int errTick = 0;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glViewport(0, 0, y.w, y.h);
        if (propI("debug.vrhome.fill", 0))
            glClearColor(i == 0 ? 0.8f : 0.1f, 0.1f, i == 1 ? 0.8f : 0.1f, 1.0f);
        else if (translucent)
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        else
            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const Mat4 vp = multiply(proj, eyeMatrix(head, kIPD, i));
        scene(e, vp);
        if (status && propI("debug.vrhome.hud", 1)) drawHud(e, proj);
        if (++errTick >= 144) {
            errTick = 0;
            GLenum ge = glGetError();
            if (ge != GL_NO_ERROR) LOGE("GL error 0x%x", ge);
        }
    }
}

void warpPresent(Engine* e) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, e->width, e->height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(e->warpProg);
    const GLint aPos = glGetAttribLocation(e->warpProg, "aPos");
    glEnableVertexAttribArray(aPos);
    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glUniform1i(glGetUniformLocation(e->warpProg, "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    const Warp wp = makeWarp(e->eye[0].w, e->eye[0].h);
    // lens axis sits ~1.25mm outboard of each half centre on a 63mm IPD /
    // ~121mm panel, roughly 0.02 in eye uv
    const float lensX = propF("debug.vrhome.lensx", 0.02f);
    const float lensY = propF("debug.vrhome.lensy", wp.cy);
    glUniform1f(glGetUniformLocation(e->warpProg, "uAspect"), wp.aspect);
    glUniform1f(glGetUniformLocation(e->warpProg, "uK0"),
                propF("debug.vrhome.k0", wp.k0));
    glUniform1f(glGetUniformLocation(e->warpProg, "uK2"),
                propF("debug.vrhome.k2", wp.k2));
    glUniform1f(glGetUniformLocation(e->warpProg, "uK4"),
                propF("debug.vrhome.k4", wp.k4));
    glUniform1f(glGetUniformLocation(e->warpProg, "uK6"),
                propF("debug.vrhome.k6", wp.k6));
    glUniform2f(glGetUniformLocation(e->warpProg, "uChroma"),
                propF("debug.vrhome.cr", kLensChr),
                propF("debug.vrhome.cb", kLensChb));
    for (int i = 0; i < 2; ++i) {
        glViewport(i * e->eye[i].w, 0, e->eye[i].w, e->eye[i].h);
        glUniform2f(glGetUniformLocation(e->warpProg, "uLensCenter"),
                    i == 0 ? wp.cx - lensX : wp.cx + lensX, lensY);
        glBindTexture(GL_TEXTURE_2D, e->eye[i].tex);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisableVertexAttribArray(aPos);
    glEnable(GL_DEPTH_TEST);
    eglSwapBuffers(e->display, e->surface);
}

void updateFps(Engine* e) {
    ++e->frames;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    if (e->fpsMark == 0) e->fpsMark = now;
    else if (now - e->fpsMark >= 1000) {
        e->fps = (int)(e->frames * 1000 / (now - e->fpsMark));
        e->sensorHz = (int)(e->sensorEv * 1000 / (now - e->fpsMark));
        e->sensorNewHz = (int)(e->sensorNew * 1000 / (now - e->fpsMark));
        e->frames = 0;
        e->sensorEv = 0;
        e->sensorNew = 0;
        e->fpsMark = now;
    }
}

void updateHud(Engine* e, const char* extra) {
    float yaw, pitch, roll;
    quatToYpr(e->quat, &yaw, &pitch, &roll);
    char pos[32] = "";
    if (e->headPosValid)
        fmtPosArrows(e->headPos, pos, sizeof(pos));
    char trk[12];
    fmtTrackState(e->qvrState, trk, sizeof(trk));
    e->hudLen = snprintf(e->hud, sizeof(e->hud),
        "YAW %+4.0f PIT %+4.0f ROL %+4.0f  FPS %d  SEN %d  TRK %s  %s%s%s",
        yaw, pitch, roll, e->fps, e->sensorNewHz, trk,
        e->qvrState == QVR_TRACKED ? "6DOF" : "3DOF", pos, extra ? extra : "");
}
