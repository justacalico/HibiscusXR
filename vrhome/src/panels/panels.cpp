#include "panels.h"

#include "layout.h"
#include "../engine.h"
#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../common/config.h"

#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

int openPanel(Engine* e, float yaw, float pitch) {
    if (!e->bridge || (int)e->panels.size() >= kMaxPanels) return -1;
    JNIEnv* env = threadEnv(e->app);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int dispId = env->CallIntMethod(e->bridge, e->mCreatePanel,
                                    (jint)tex, kVdW, kVdH, kVdDpi);
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    if (dispId < 0) { glDeleteTextures(1, &tex); return -1; }

    jobject st = env->CallObjectMethod(e->bridge, e->mPanelTex, dispId);
    if (!st) { glDeleteTextures(1, &tex);
        env->CallVoidMethod(e->bridge, e->mReleasePanel, dispId); return -1; }

    Panel p;
    p.displayId = dispId;
    p.tex = tex;
    p.st = env->NewGlobalRef(st);
    p.stArr = env->NewGlobalRef(env->NewFloatArray(16));
    memset(p.stMat, 0, sizeof(p.stMat));
    p.yaw = yaw;
    p.pitch = pitch;
    p.grabYaw = yaw;
    p.grabPitch = pitch;
    // the pick can hit the pill before the first draw measures the label
    p.pillHW = pillHalfWidth(0.0f, kPanelW * 0.5f, true);
    e->panels.push_back(p);
    LOGI("panel %d on display %d yaw %.2f", (int)e->panels.size() - 1,
         dispId, yaw);
    return (int)e->panels.size() - 1;
}

void closePanel(Engine* e, int idx) {
    Panel& p = e->panels[idx];
    JNIEnv* env = threadEnv(e->app);
    if (e->bridge && p.displayId >= 0)
        env->CallVoidMethod(e->bridge, e->mReleasePanel, p.displayId);
    if (p.st) env->DeleteGlobalRef((jobject)p.st);
    if (p.stArr) env->DeleteGlobalRef((jobject)p.stArr);
    if (p.tex) { GLuint t = p.tex; glDeleteTextures(1, &t); }
    e->panels.erase(e->panels.begin() + idx);
    if (e->hover == idx) { e->hover = -1; e->hoverZone = ZONE_NONE; }
    else if (e->hover > idx) e->hover--;
}

bool evictOldestApp(Engine* e) {
    const int i = evictIndex(e->panels);
    if (i < 0) return false;
    Panel& p = e->panels[i];
    if (e->bridge && p.taskId >= 0) {
        JNIEnv* env = threadEnv(e->app);
        env->CallVoidMethod(e->bridge, e->mRemoveTask, p.taskId);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    closePanel(e, i);
    return true;
}

void updatePanels(Engine* e) {
    if (e->panels.empty()) return;
    JNIEnv* env = threadEnv(e->app);
    for (auto& p : e->panels) {
        if (!p.st) continue;
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, p.tex);
        env->CallVoidMethod((jobject)p.st, e->stUpdate);
        env->CallVoidMethod((jobject)p.st, e->stMatrix, (jfloatArray)p.stArr);
        env->GetFloatArrayRegion((jfloatArray)p.stArr, 0, 16, p.stMat);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}
