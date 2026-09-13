#include "bridge.h"

#include "../engine.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../panels/panels.h"

#include <deque>
#include <mutex>

// launch requests arrive from Java (LauncherActivity, test hook)
static std::deque<std::string> gLaunchQ;
static std::mutex gLaunchMu;

static volatile bool gWantRecenter = false;

extern "C" JNIEXPORT void JNICALL
Java_org_pn2_vrhome_ShellBridge_nativeQueueLaunch(JNIEnv* env, jclass, jstring pkg) {
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    queueLaunch(p);
    env->ReleaseStringUTFChars(pkg, p);
}

extern "C" JNIEXPORT void JNICALL
Java_org_pn2_vrhome_PanelActivity_nativeHome(JNIEnv*, jclass) {
    gWantRecenter = true;
}

void queueLaunch(const char* pkg) {
    std::lock_guard<std::mutex> l(gLaunchMu);
    gLaunchQ.push_back(pkg);
}

bool takeWantRecenter() {
    const bool r = gWantRecenter;
    gWantRecenter = false;
    return r;
}

JNIEnv* threadEnv(android_app* app) {
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    return env;
}

jclass loadAppClass(JNIEnv* env, jobject activity, const char* name) {
    jclass actCls = env->GetObjectClass(activity);
    jmethodID getCL = env->GetMethodID(actCls, "getClassLoader",
                                     "()Ljava/lang/ClassLoader;");
    jobject cl = env->CallObjectMethod(activity, getCL);
    jclass clCls = env->FindClass("java/lang/ClassLoader");
    jmethodID load = env->GetMethodID(clCls, "loadClass",
                                      "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring jn = env->NewStringUTF(name);
    jclass c = (jclass)env->CallObjectMethod(cl, load, jn);
    env->DeleteLocalRef(jn);
    return c;
}

void initBridge(Engine* e) {
    JNIEnv* env = threadEnv(e->app);
    jclass bc = loadAppClass(env, e->app->activity->clazz,
                             "org.pn2.vrhome.ShellBridge");
    if (!bc) { e->bridgeDead = true; LOGE("no ShellBridge class"); return; }
    jmethodID ctor = env->GetMethodID(bc, "<init>", "(Landroid/content/Context;)V");
    jobject br = env->NewObject(bc, ctor, e->app->activity->clazz);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe(); env->ExceptionClear();
        e->bridgeDead = true; LOGE("ShellBridge ctor failed"); return;
    }
    e->bridge = env->NewGlobalRef(br);
    e->mCreatePanel  = env->GetMethodID(bc, "createPanel", "(IIII)I");
    e->mPanelTex     = env->GetMethodID(bc, "panelTexture",
                        "(I)Landroid/graphics/SurfaceTexture;");
    e->mLaunchPkg    = env->GetMethodID(bc, "launchPackageOn",
                        "(Ljava/lang/String;I)V");
    e->mLaunchLauncher = env->GetMethodID(bc, "launchLauncherOn", "(I)V");
    e->mAdopt        = env->GetMethodID(bc, "adoptTaskOn", "(II)V");
    e->mReleasePanel = env->GetMethodID(bc, "releasePanel", "(I)V");
    e->mTakeAdopt    = env->GetMethodID(bc, "takePendingAdopt",
                        "()Lorg/pn2/vrhome/ShellBridge$Pending;");
    e->mTakeRelease  = env->GetMethodID(bc, "takePendingRelease", "()I");
    e->mInjectTap    = env->GetMethodID(bc, "injectTap", "(IFF)V");
    e->mRemoveTask   = env->GetMethodID(bc, "removeTask", "(I)V");
    e->mFocusTask    = env->GetMethodID(bc, "focusTask", "(I)V");
    e->mAppLabel     = env->GetMethodID(bc, "appLabel",
                        "(Ljava/lang/String;)Ljava/lang/String;");

    jclass stc = env->FindClass("android/graphics/SurfaceTexture");
    e->stUpdate = env->GetMethodID(stc, "updateTexImage", "()V");
    e->stMatrix = env->GetMethodID(stc, "getTransformMatrix", "([F)V");

    e->pendingCls = (jclass)env->NewGlobalRef(loadAppClass(env,
        e->app->activity->clazz, "org.pn2.vrhome.ShellBridge$Pending"));
    e->fPendTask = env->GetFieldID(e->pendingCls, "taskId", "I");
    e->fPendPkg  = env->GetFieldID(e->pendingCls, "pkg", "Ljava/lang/String;");
    LOGI("bridge ready");
}

void pumpBridge(Engine* e) {
    if (!e->bridge) return;
    JNIEnv* env = threadEnv(e->app);

    // app launches requested by the library panel / test hook
    for (;;) {
        std::string pkg;
        {
            std::lock_guard<std::mutex> l(gLaunchMu);
            if (gLaunchQ.empty()) break;
            pkg = gLaunchQ.front(); gLaunchQ.pop_front();
        }
        if ((int)e->panels.size() >= kMaxPanels && !evictOldestApp(e)) continue;
        int idx = openPanel(e, freeSlotYaw(e->panels, e->gazeYaw));
        if (idx < 0) continue;
        e->panels[idx].pkg = pkg;
        jstring jpkg = env->NewStringUTF(pkg.c_str());
        env->CallVoidMethod(e->bridge, e->mLaunchPkg, jpkg,
                            e->panels[idx].displayId);
        env->DeleteLocalRef(jpkg);
        if (env->ExceptionCheck()) { env->ExceptionClear(); }
    }

    if (!e->pendingCls) return;

    // stray display-0 tasks the poller wants us to take
    for (;;) {
        jobject p = env->CallObjectMethod(e->bridge, e->mTakeAdopt);
        if (env->ExceptionCheck()) { env->ExceptionClear(); break; }
        if (!p) break;
        int taskId = env->GetIntField(p, e->fPendTask);
        jstring jpkg = (jstring)env->GetObjectField(p, e->fPendPkg);
        if ((int)e->panels.size() >= kMaxPanels) evictOldestApp(e);
        int idx = openPanel(e, freeSlotYaw(e->panels, e->gazeYaw));
        if (idx >= 0) {
            e->panels[idx].taskId = taskId;
            if (jpkg) {
                const char* c = env->GetStringUTFChars(jpkg, nullptr);
                e->panels[idx].pkg = c;
                env->ReleaseStringUTFChars(jpkg, c);
            }
            env->CallVoidMethod(e->bridge, e->mAdopt, taskId,
                                e->panels[idx].displayId);
            if (env->ExceptionCheck()) { env->ExceptionClear(); }
        } else {
            // no free panel slots: kill the stray rather than leave a mono
            // app stuck to the physical display retrying forever
            env->CallVoidMethod(e->bridge, e->mRemoveTask, taskId);
            if (env->ExceptionCheck()) { env->ExceptionClear(); }
        }
        env->DeleteLocalRef(p);
    }

    // empty displays the poller wants torn down
    for (;;) {
        int id = env->CallIntMethod(e->bridge, e->mTakeRelease);
        if (env->ExceptionCheck()) { env->ExceptionClear(); break; }
        if (id < 0) break;
        for (int i = 0; i < (int)e->panels.size(); ++i)
            if (e->panels[i].displayId == id) { closePanel(e, i); break; }
    }

    // window-bar labels: resolve once per panel, after pkg is known
    if (e->mAppLabel) {
        for (auto& p : e->panels) {
            if (p.pkg.empty() || !p.label.empty()) continue;
            jstring jpkg = env->NewStringUTF(p.pkg.c_str());
            jstring jl = (jstring)env->CallObjectMethod(e->bridge, e->mAppLabel,
                                                      jpkg);
            env->DeleteLocalRef(jpkg);
            if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
            if (!jl) continue;
            const char* c = env->GetStringUTFChars(jl, nullptr);
            p.label = c;
            env->ReleaseStringUTFChars(jl, c);
            env->DeleteLocalRef(jl);
        }
    }
}
