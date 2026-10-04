// Passthrough camera session for the env process. The Pico Neo 2's
// tracking pair shows up as camera device "0" delivering a single
// 1280x400 YUV stream: the left eye in x [0,640), the right in
// [640,1280). The NDK camera2 API drives it into a SurfaceTexture owned
// by the render thread; scene.cpp warps each half to its eye.
//
// Everything here is glue: the state machine is "want feed + permission +
// no session -> open; gone away -> close", and no geometry lives here.

#include "cam.h"

#include "envmap.h"
#include "../engine.h"
#include "../common/log.h"
#include "../common/jni.h"
#include "../common/props.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCaptureRequest.h>
#include <camera/NdkCameraCaptureSession.h>

#include <GLES2/gl2.h>

#include <sys/system_properties.h>
#include <cstdio>
#include <ctime>
#include <vector>
#include <cstdint>
#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace {

constexpr int kCamW = 1280, kCamH = 400;

struct PtCam {
    ACameraManager* mgr = nullptr;
    ACameraDevice* dev = nullptr;
    ACaptureSessionOutputContainer* outs = nullptr;
    ACaptureSessionOutput* out = nullptr;
    ACameraOutputTarget* tgt = nullptr;
    ACaptureRequest* req = nullptr;
    ACameraCaptureSession* ses = nullptr;
    ANativeWindow* anw = nullptr;
    bool perm = false;
    bool opening = false;      // a start is in flight this frame
    long long retryMs = 0;     // CLOCK_MONOTONIC ms of the last failed start
};

void onDisconnected(void*, ACameraDevice*) {
    LOGI("pt: camera disconnected");
}

void onError(void*, ACameraDevice*, int err) {
    LOGE("pt: camera error %d", err);
}

void onClosed(void* ctx, ACameraCaptureSession*) {
    // the session died under us: flag it so ptTick rebuilds on the next
    // frame rather than drawing a frozen feed
    Engine* e = (Engine*)ctx;
    e->ptLive = false;
    LOGI("pt: session closed");
}

void teardown(Engine* e, PtCam* c) {
    if (c->ses) { ACameraCaptureSession_stopRepeating(c->ses); }
    if (c->req) { ACaptureRequest_free(c->req); c->req = nullptr; }
    if (c->tgt) { ACameraOutputTarget_free(c->tgt); c->tgt = nullptr; }
    if (c->out) { ACaptureSessionOutput_free(c->out); c->out = nullptr; }
    if (c->outs) {
        ACaptureSessionOutputContainer_free(c->outs);
        c->outs = nullptr;
    }
    if (c->ses) { ACameraCaptureSession_close(c->ses); c->ses = nullptr; }
    if (c->anw) { ANativeWindow_release(c->anw); c->anw = nullptr; }
    if (c->dev) { ACameraDevice_close(c->dev); c->dev = nullptr; }
    if (c->mgr) { ACameraManager_delete(c->mgr); c->mgr = nullptr; }
    e->ptLive = false;
}

// builds the SurfaceTexture + session on the engine's context. Called
// only when the GL context is current.
bool start(Engine* e, PtCam* c) {
    JNIEnv* env = threadEnv(e->vm);
    jclass act = loadAppClass(env, e->ctx, "gitlab.neosalsa.home.PanelActivity");
    if (!act) { env->ExceptionClear(); LOGE("pt: no PanelActivity class"); return false; }
    jmethodID mTex = env->GetStaticMethodID(act, "camTexture",
                                          "(III)Landroid/graphics/SurfaceTexture;");
    jmethodID mSurf = env->GetStaticMethodID(act, "camSurface",
                                           "()Landroid/view/Surface;");
    if (!mTex || !mSurf) { LOGE("pt: no cam methods"); return false; }

    if (!e->ptTex) {
        glGenTextures(1, &e->ptTex);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->ptTex);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                        GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                        GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    jobject st = env->CallStaticObjectMethod(act, mTex, (jint)e->ptTex,
                                             kCamW, kCamH);
    if (env->ExceptionCheck() || !st) {
        env->ExceptionClear();
        LOGE("pt: camTexture call failed");
        return false;
    }
    e->ptSt = env->NewGlobalRef(st);
    jclass stCls = env->GetObjectClass(st);
    e->stUpdate = env->GetMethodID(stCls, "updateTexImage", "()V");
    e->stMatrix = env->GetMethodID(stCls, "getTransformMatrix", "([F)V");
    e->ptStArr = env->NewGlobalRef(env->NewFloatArray(16));

    jobject surf = env->CallStaticObjectMethod(act, mSurf);
    if (!surf) { LOGE("pt: no surface"); return false; }
    c->anw = ANativeWindow_fromSurface(env, surf);
    env->DeleteLocalRef(surf);
    if (!c->anw) { LOGE("pt: no native window"); return false; }

    c->mgr = ACameraManager_create();
    if (!c->mgr) { LOGE("pt: no camera manager"); return false; }

    ACameraDevice_StateCallbacks dcbs = {};
    dcbs.context = e;
    dcbs.onDisconnected = onDisconnected;
    dcbs.onError = onError;
    if (ACameraManager_openCamera(c->mgr, "0", &dcbs, &c->dev) !=
            ACAMERA_OK) {
        LOGE("pt: openCamera failed");
        return false;
    }

    ACaptureSessionOutputContainer_create(&c->outs);
    ACaptureSessionOutput_create(c->anw, &c->out);
    ACaptureSessionOutputContainer_add(c->outs, c->out);

    ACameraCaptureSession_stateCallbacks scbs = {};
    scbs.context = e;
    scbs.onClosed = onClosed;
    if (ACameraDevice_createCaptureSession(c->dev, c->outs, &scbs,
                                           &c->ses) != ACAMERA_OK) {
        LOGE("pt: createCaptureSession failed");
        return false;
    }

    ACameraDevice_createCaptureRequest(c->dev, TEMPLATE_PREVIEW, &c->req);
    ACameraOutputTarget_create(c->anw, &c->tgt);
    ACaptureRequest_addTarget(c->req, c->tgt);
    ACameraCaptureSession_setRepeatingRequest(c->ses, nullptr, 1,
                                              &c->req, nullptr);
    e->ptLive = true;
    LOGI("pt: camera streaming");
    return true;
}

} // namespace

void ptTick(Engine* e) {
    // the camera only runs while the home backdrop is passthrough - a
    // built-in or zip environment never opens the tracking pair
    const bool want = e->envMode == kEnvPassthrough &&
                      propI("debug.vrhome.passthrough", 1) &&
                      e->ready && !e->covered && e->context != EGL_NO_CONTEXT;
    PtCam* c = (PtCam*)e->ptCam;
    if (!want) {
        if (c) teardown(e, c);
        return;
    }
    if (c && c->ses && e->ptLive) return;   // healthy
    if (c && c->opening) return;            // a start already ran this tick
    if (!c) {
        c = new PtCam();
        e->ptCam = c;
    }
    // permission: the Java side holds the activity; without it the camera
    // service refuses the open, so keep the sky scene up meanwhile
    JNIEnv* env = threadEnv(e->vm);
    if (!c->perm) {
        jclass act = loadAppClass(env, e->ctx, "gitlab.neosalsa.home.PanelActivity");
        jmethodID mOk = act ? env->GetStaticMethodID(act, "camPermOk",
                                                     "()Z") : nullptr;
        if (mOk && env->CallStaticBooleanMethod(act, mOk)) {
            c->perm = true;
        } else {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }
    }
    if (c->ses || c->dev || c->mgr) teardown(e, c);   // half-dead: rebuild clean
    // a failed open churns the camera service hard - retry on a slow
    // backoff, not every frame
    struct timespec rts;
    clock_gettime(CLOCK_MONOTONIC, &rts);
    const long long nowMs =
        (long long)rts.tv_sec * 1000 + rts.tv_nsec / 1000000;
    if (nowMs - c->retryMs < 2000) return;
    c->retryMs = nowMs;
    c->opening = true;
    if (!start(e, c)) {
        // stay on the sky scene; retry in a couple seconds
        e->ptLive = false;
    }
    c->opening = false;
}

// debug.vrhome.ptdump: copy the live camera texture into a PPM so the
// real frame packing is inspectable without an ImageReader
static void ptDump(Engine* e) {
    // external textures can't be read back or rendered into, so draw the
    // feed through floatProg into a scratch 2D texture and read that
    GLuint fbo = 0, tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kCamW, kCamH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("pt: dump fbo incomplete 0x%x",
             glCheckFramebufferStatus(GL_FRAMEBUFFER));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &tex);
        glDeleteFramebuffers(1, &fbo);
        return;
    }
    glViewport(0, 0, kCamW, kCamH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(e->floatProg);
    glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
    glUniform2f(glGetUniformLocation(e->floatProg, "uHalf"), 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadius"), 0.0f);
    glUniform1f(glGetUniformLocation(e->floatProg, "uRadiusB"), 0.0f);
    const float id[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uMVP"),
                       1, GL_FALSE, id);
    glUniformMatrix4fv(glGetUniformLocation(e->floatProg, "uST"),
                       1, GL_FALSE, e->ptMat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->ptTex);
    const float q[4][5] = {{-1,-1,0, 0,0}, {1,-1,0, 1,0},
                           {1,1,0, 1,1},  {-1,1,0, 0,1}};
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
    std::vector<uint8_t> px(kCamW * kCamH * 4);
    glReadPixels(0, 0, kCamW, kCamH, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);
    glEnable(GL_DEPTH_TEST);
    FILE* f = fopen("/data/data/gitlab.neosalsa.home/files/pt.ppm", "wb");
    if (!f) f = fopen("/sdcard/pt.ppm", "wb");
    if (!f) return;
    fprintf(f, "P6 %d %d 255\n", kCamW, kCamH);
    // glReadPixels is bottom-up; flip rows
    for (int y = kCamH - 1; y >= 0; --y)
        for (int x = 0; x < kCamW; ++x) {
            const uint8_t* p = &px[(y * kCamW + x) * 4];
            fwrite(p, 3, 1, f);
        }
    fclose(f);
    LOGI("pt: dumped frame to pt.ppm");
}

void ptUpdate(Engine* e) {
    if (!e->ptLive || !e->ptSt || !e->stUpdate) return;
    JNIEnv* env = threadEnv(e->vm);
    env->CallVoidMethod(e->ptSt, e->stUpdate);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        e->ptLive = false;   // stale texture: drop the feed
        return;
    }
    env->CallVoidMethod(e->ptSt, e->stMatrix, (jfloatArray)e->ptStArr);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    jfloat* m = env->GetFloatArrayElements((jfloatArray)e->ptStArr,
                                           nullptr);
    memcpy(e->ptMat, m, sizeof(e->ptMat));
    env->ReleaseFloatArrayElements((jfloatArray)e->ptStArr, m, JNI_ABORT);
    if (propI("debug.vrhome.ptdump", 0)) {
        ptDump(e);
        __system_property_set("debug.vrhome.ptdump", "0");
    }
}

void ptStop(Engine* e) {
    PtCam* c = (PtCam*)e->ptCam;
    if (!c) return;
    JNIEnv* env = e->vm ? threadEnv(e->vm) : nullptr;
    if (env) {
        jclass act = loadAppClass(env, e->ctx, "gitlab.neosalsa.home.PanelActivity");
        jmethodID mRel = act ? env->GetStaticMethodID(act, "camRelease",
                                                    "()V") : nullptr;
        if (mRel) env->CallStaticVoidMethod(act, mRel);
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (e->ptSt) { env->DeleteGlobalRef(e->ptSt); e->ptSt = nullptr; }
        if (e->ptStArr) {
            env->DeleteGlobalRef(e->ptStArr);
            e->ptStArr = nullptr;
        }
    }
    teardown(e, c);
    delete c;
    e->ptCam = nullptr;
}
