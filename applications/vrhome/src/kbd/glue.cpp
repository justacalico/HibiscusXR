#include "kbd.h"

#include "../hud/engine.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"

#include <GLES2/gl2.h>
#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// one-time: GL texture + java-side Surface/SurfaceTexture the IME draws
// into. Unlike the panel displays the HUD never owns a display for it -
// the keyboard process creates its own private virtual display on the
// shared surface, so Presentation's private-display rule is satisfied
static void kbdEnsure(HudEngine* e) {
    if (e->kbd.st) return;
    JNIEnv* env = threadEnv(e->vm);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const jboolean ok = env->CallBooleanMethod(e->bridge, e->mKbdCreate,
                                               (jint)tex, kKbdW, kKbdH,
                                               kKbdDpi);
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    if (!ok) { glDeleteTextures(1, &tex); return; }
    jobject st = env->CallObjectMethod(e->bridge, e->mKbdTex);
    if (!st) { glDeleteTextures(1, &tex); return; }
    e->kbd.tex = tex;
    e->kbd.st = env->NewGlobalRef(st);
    e->kbd.stArr = env->NewGlobalRef(env->NewFloatArray(16));
    memset(e->kbd.stMat, 0, sizeof(e->kbd.stMat));
    env->DeleteLocalRef(st);
    LOGI("kbd surface ready");
}

void kbdTick(HudEngine* e) {
    if (!e->bridge || e->context == EGL_NO_CONTEXT || !e->mKbdCreate) return;
    JNIEnv* env = threadEnv(e->vm);
    kbdEnsure(e);
    if (!e->kbd.st) return;

    // the IME asks for the surface every time it shows; the reply carries
    // the Surface parcelable so a HUD restart re-handshakes on the next
    // field focus instead of staying bound to a dead texture
    if (env->CallBooleanMethod(e->bridge, e->mKbdTakeQuery) == JNI_TRUE) {
        env->CallVoidMethod(e->bridge, e->mKbdSend);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    jintArray a = (jintArray)env->CallObjectMethod(e->bridge, e->mKbdState);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (a) {
        jint v[3] = {0, -1, 0};
        env->GetIntArrayRegion(a, 0, 3, v);
        e->kbd.shown = v[0] != 0;
        e->kbd.displayId = v[1];
        e->kbd.only = v[2] != 0;
        env->DeleteLocalRef(a);
    }
    if (!e->kbd.shown) {
        if (e->kbd.hover) { e->kbd.hover = false; e->kbd.pressed = false; }
        return;
    }
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, e->kbd.tex);
    env->CallVoidMethod((jobject)e->kbd.st, e->stUpdate);
    env->CallVoidMethod((jobject)e->kbd.st, e->stMatrix, (jfloatArray)e->kbd.stArr);
    env->GetFloatArrayRegion((jfloatArray)e->kbd.stArr, 0, 16, e->kbd.stMat);
    if (env->ExceptionCheck()) env->ExceptionClear();
}
