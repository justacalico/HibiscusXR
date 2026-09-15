#include "bridge.h"

#include "../hud/engine.h"
#include "../common/jni.h"
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
Java_gitlab_neosalsa_hud_ShellBridge_nativeQueueLaunch(JNIEnv* env, jclass, jstring pkg) {
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    queueLaunch(p);
    env->ReleaseStringUTFChars(pkg, p);
}

void queueLaunch(const char* pkg) {
    std::lock_guard<std::mutex> l(gLaunchMu);
    gLaunchQ.push_back(pkg);
}

void wantRecenter() { gWantRecenter = true; }

bool takeWantRecenter() {
    const bool r = gWantRecenter;
    gWantRecenter = false;
    return r;
}

void initBridge(HudEngine* e, JNIEnv* env, jobject br) {
    jclass bc = env->GetObjectClass(br);
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
                        "()Lgitlab/neosalsa/hud/ShellBridge$Pending;");
    e->mTakeRelease  = env->GetMethodID(bc, "takePendingRelease", "()I");
    e->mInjectTap    = env->GetMethodID(bc, "injectTap", "(IFF)V");
    e->mInjectTouch  = env->GetMethodID(bc, "injectTouch", "(IFFI)V");
    e->mRemoveTask   = env->GetMethodID(bc, "removeTask", "(I)V");
    e->mFocusTask    = env->GetMethodID(bc, "focusTask", "(I)V");
    e->mAppLabel     = env->GetMethodID(bc, "appLabel",
                        "(Ljava/lang/String;)Ljava/lang/String;");
    e->mIsVr         = env->GetMethodID(bc, "isVrApp",
                        "(Ljava/lang/String;)Z");
    e->mLaunchVr     = env->GetMethodID(bc, "launchVrApp",
                        "(Ljava/lang/String;)V");
    e->mIsCovered    = env->GetMethodID(bc, "isCovered", "()Z");

    jclass stc = env->FindClass("android/graphics/SurfaceTexture");
    e->stUpdate = env->GetMethodID(stc, "updateTexImage", "()V");
    e->stMatrix = env->GetMethodID(stc, "getTransformMatrix", "([F)V");

    e->pendingCls = (jclass)env->NewGlobalRef(loadAppClass(env,
        e->ctx, "gitlab.neosalsa.hud.ShellBridge$Pending"));
    e->fPendTask = env->GetFieldID(e->pendingCls, "taskId", "I");
    e->fPendPkg  = env->GetFieldID(e->pendingCls, "pkg", "Ljava/lang/String;");
    LOGI("bridge ready");
}

void pumpBridge(HudEngine* e) {
    if (!e->bridge) return;
    JNIEnv* env = threadEnv(e->vm);

    // app launches requested by the library panel / test hook
    for (;;) {
        std::string pkg;
        {
            std::lock_guard<std::mutex> l(gLaunchMu);
            if (gLaunchQ.empty()) break;
            pkg = gLaunchQ.front(); gLaunchQ.pop_front();
        }
        jstring jpkg = env->NewStringUTF(pkg.c_str());
        // Pico VR apps take over the headset; no panel is spent on them
        if (e->mIsVr && env->CallBooleanMethod(e->bridge, e->mIsVr, jpkg)) {
            env->CallVoidMethod(e->bridge, e->mLaunchVr, jpkg);
            env->DeleteLocalRef(jpkg);
            if (env->ExceptionCheck()) { env->ExceptionClear(); }
            continue;
        }
        // a minimized window for this app comes back instead of opening a
        // new panel: its task and display are still alive
        const int mi = minimizedIndex(e->panels, pkg);
        if (mi >= 0) {
            e->panels[mi].minimized = false;
            if (e->panels[mi].taskId >= 0) {
                env->CallVoidMethod(e->bridge, e->mFocusTask,
                                    e->panels[mi].taskId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            LOGI("unminimized %s on disp %d", pkg.c_str(),
                 e->panels[mi].displayId);
            env->DeleteLocalRef(jpkg);
            continue;
        }
        if ((int)e->panels.size() >= kMaxPanels && !evictOldestApp(e)) {
            env->DeleteLocalRef(jpkg);
            continue;
        }
        int idx = openPanel(e, freeSlotYaw(e->panels, e->gazeYaw),
                            ringPitch(e->panels));
        if (idx < 0) { env->DeleteLocalRef(jpkg); continue; }
        e->panels[idx].pkg = pkg;
        env->CallVoidMethod(e->bridge, e->mLaunchPkg, jpkg,
                            e->panels[idx].displayId);
        env->DeleteLocalRef(jpkg);
        if (env->ExceptionCheck()) { env->ExceptionClear(); }
    }

    // a fullscreen task owns the HMD; when it lets go the panels come back,
    // so recenter the ring on wherever the user ended up looking
    if (e->mIsCovered) {
        const bool was = e->covered;
        e->covered = env->CallBooleanMethod(e->bridge, e->mIsCovered) == JNI_TRUE;
        if (was && !e->covered) wantRecenter();
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
        int idx = openPanel(e, freeSlotYaw(e->panels, e->gazeYaw),
                            ringPitch(e->panels));
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
