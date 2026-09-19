// The HUD: panels, the app library and the summonable menu, drawn into a
// fullscreen TYPE_SYSTEM_OVERLAY window owned by HudService. Virtual
// displays live in this process (ShellBridge is constructed on the java
// side), so every panel - and the library itself - keeps running no matter
// which app owns the physical display underneath.
//
// Threading: HudService runs on the main thread and feeds this file through
// JNI - surface post/gone, key presses. One pthread runs the render loop and
// owns the sensor queue; all GL and bridge calls happen on it.

#include "engine.h"

#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../input/input.h"
#include "../math/head.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../render/chrome.h"
#include "../render/egl.h"
#include "../render/frame.h"
#include "../render/warp.h"
#include "../sensor/sensor.h"
#include "../sensor/qvr.h"

#include <android/looper.h>
#include <android/native_window.h>
#include <android/sensor.h>
#include <android/native_window_jni.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <pthread.h>

static HudEngine gHud;
static pthread_t gThread;
static bool gStarted = false;

// test hook: setprop debug.vrhome.launch <pkg> queues a panel launch
static void debugLaunchHook(HudEngine* e) {
    static bool testFired = false;
    char tb[PROP_VALUE_MAX];
    if (!testFired && __system_property_get("debug.vrhome.launch", tb) > 0
            && e->frames > 30) {
        testFired = true;
        queueLaunch(tb);
    }
}

// test hook: setprop debug.vrhome.tap "disp,x,y" injects a tap there.
// Fires once per new value; the prop may not be clearable from this uid.
static void debugTapHook(HudEngine* e) {
    static char lastTap[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (__system_property_get("debug.vrhome.tap", tb) > 0 &&
            strcmp(tb, lastTap) != 0) {
        strncpy(lastTap, tb, sizeof(lastTap) - 1);
        int d, x, y;
        if (sscanf(tb, "%d,%d,%d", &d, &x, &y) == 3 && e->bridge) {
            JNIEnv* env = threadEnv(e->vm);
            env->CallVoidMethod(e->bridge, e->mInjectTap, d,
                                (float)x, (float)y);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }
}

// test hook: setprop debug.vrhome.summon <n> toggles the dash over the
// covered app, same as a short summon press. Each new value refires, so
// the prop doubles as an adb-driven summon for testing
static void debugSummonHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (__system_property_get("debug.vrhome.summon", tb) > 0 &&
            strcmp(tb, last) != 0 && e->ctx) {
        strncpy(last, tb, sizeof(last) - 1);
        JNIEnv* env = threadEnv(e->vm);
        jclass c = env->GetObjectClass(e->ctx);
        jmethodID m = env->GetMethodID(c, "debugSummon", "()V");
        if (m) env->CallVoidMethod(e->ctx, m);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

// test hook: setprop debug.vrhome.hold 1|0 feeds a summon-key down/up
// through the real onSummon path, so the hold ring and long-press recenter
// are drivable from adb - injected keyevents never reach the key filter
static void debugHoldHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (__system_property_get("debug.vrhome.hold", tb) > 0 &&
            strcmp(tb, last) != 0 && e->ctx) {
        strncpy(last, tb, sizeof(last) - 1);
        JNIEnv* env = threadEnv(e->vm);
        jclass c = env->GetObjectClass(e->ctx);
        jmethodID m = env->GetMethodID(c, "debugSummonKey", "(I)V");
        if (m) env->CallVoidMethod(e->ctx, m, tb[0] == '1' ? 0 : 1);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

// one-time: library panel dead ahead once tracking is live. recenterAngles
// covers the desk-flat case too, so a boot with the headset lying on its
// back still puts the launcher in front of the head's heading
static void spawnLauncher(HudEngine* e, const Mat4& head) {
    if (!e->bridge || !e->haveQuat || e->launcherSpawned) return;
    e->launcherSpawned = true;
    // the dash's first appearance anchors the ring at the head's spot too,
    // so a fresh boot doesn't park the panels around the tracking origin
    memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
    float gy = 0.0f, gp = 0.0f;
    recenterAngles(head, &gy, &gp);
    int idx = openPanel(e, gy, gp);
    if (idx >= 0) {
        e->panels[idx].pkg = kLibraryPkg;
        JNIEnv* env = threadEnv(e->vm);
        env->CallVoidMethod(e->bridge, e->mLaunchLauncher,
                            e->panels[idx].displayId);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

// the HUD's world is just the chrome: panels plus the gaze cursor, over
// whatever surface sits underneath (env scenery or a running app)
static void hudScene(Engine* e, const Mat4& vp) {
    HudEngine* h = (HudEngine*)e;
    drawPanels(h, vp);
    drawCursor(h, vp);
    // the hold ring is a flat overlay: it draws on top of the live scene and
    // ignores vp entirely, so it stays put while the world shifts around it
    drawHoldRing(h);
}

static void hudFrame(HudEngine* e) {
    // surface handoff from the SurfaceHolder callbacks
    ANativeWindow* nw = e->window.exchange(nullptr);
    if (nw) {
        if (e->ready) termWindow(e);
        if (initWindow(e, nw) != 0) LOGE("initWindow failed");
        ANativeWindow_release(nw);
    }
    if (e->windowGone.exchange(false)) termWindow(e);

    if (e->context == EGL_NO_CONTEXT) { usleep(50000); return; }

    // rotation-vector samples land on this thread's looper; drain them into
    // e->quat the same way the env does
    int ident, events;
    void* data;
    while ((ident = ALooper_pollOnce(0, nullptr, &events, &data)) >= 0) {
        if (ident == kSensorIdent) drainSensor(e);
    }

    // queued key presses from the window
    for (;;) {
        KeyIn k;
        {
            std::lock_guard<std::mutex> l(e->keyMu);
            if (e->keyQ.empty()) break;
            k = e->keyQ.front(); e->keyQ.pop_front();
        }
        hudKey(e, k.code, k.action, k.repeat);
    }

    qvrPoll(e);
    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    const float fakePos[3] = {propF("debug.vrhome.fpx", 0.0f),
                              propF("debug.vrhome.fpy", 0.0f),
                              propF("debug.vrhome.fpz", 0.0f)};
    const float sensRoll = e->quatFromQvr ? propF("debug.vrhome.qvrsensroll", kQvrSensRoll)
                                          : propF("debug.vrhome.sensroll", kSensRoll);
    const float worldX = e->quatFromQvr ? propF("debug.vrhome.qvrworldx", kQvrWorldX)
                                        : propF("debug.vrhome.worldx", kWorldX);
    const float* headPos = nullptr;
    float posGl[3];
    if (useSensor && e->headPosValid) {
        sensorPosToWorld(e->headPos, sensRoll, worldX, posGl);
        headPos = posGl;
    }
    if (useSensor && (fakePos[0] || fakePos[1] || fakePos[2])) {
        memcpy(e->headPos, fakePos, sizeof(fakePos));
        e->headPosValid = true;
        headPos = fakePos;
    }
    // the gaze ray starts where the head actually is: the mapped position
    // that went into the view matrix, or the world origin when position is
    // out - the pick must see the same eye the render does
    if (headPos) memcpy(e->eyePos, headPos, sizeof(e->eyePos));
    else memset(e->eyePos, 0, sizeof(e->eyePos));
    const Mat4 head = headMatrix(e->quat, propI("debug.vrhome.tq", 1) != 0,
        sensRoll, worldX,
        propF("debug.vrhome.roll",     kRoll), useSensor, headPos);

    debugLaunchHook(e);
    debugTapHook(e);
    debugSummonHook(e);
    debugHoldHook(e);
    spawnLauncher(e, head);

    // hold-to-recenter fill: the java side owns the threshold and fires the
    // recenter; native just turns the held time into the ring's 0..1
    if (e->holdStartMs > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        const long long now = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        const float p = (float)(now - e->holdStartMs) / (float)kHoldMs;
        e->holdP = p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p;
    } else {
        e->holdP = 0.0f;
    }

    if (takeWantRecenter()) {
        // the ring re-anchors to where the head is right now: panels keep
        // their slot offsets but the whole dash lands in front of the user
        memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
        float yaw, pitch;
        recenterAngles(head, &yaw, &pitch);
        recenterSlots(e->panels, yaw, pitch);
    }
    pumpBridge(e);

    // gaze pick: nearest panel under the head ray, hit in display px
    const Pick pk = pickPanel(e->panels, head, e->ringPos, e->eyePos);
    e->hover = pk.idx;
    e->hoverZone = pk.idx >= 0 ? pk.zone : ZONE_NONE;
    if (pk.idx >= 0) {
        e->hitX = (pk.u * 0.5f + 0.5f) * kVdW;
        e->hitY = (0.5f - pk.v * 0.5f) * kVdH;
    }
    float gy;
    if (gazeYaw(head, &gy)) e->gazeYaw = gy;
    e->gazePitch = gazePitch(head);

    dragTick(e, head);
    moveTick(e);
    updatePanels(e);

    // hidden or no surface: management above must still run - pumpBridge is
    // what adopts strays and keeps the displays alive - but there is nothing
    // to present and no vsync to pace us
    if (!e->ready) { usleep(33000); return; }

    char extra[48];
    snprintf(extra, sizeof(extra), "  PNL %zu%s", e->panels.size(),
             e->bridge ? "" : "  BRIDGE:OFF");
    updateHud(e, extra);
    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const float fov = propF("debug.vrhome.fov", kFovY);
    drawEyes(e, head, perspective(fov, aspect, 0.05f, 100.0f), true, true,
             hudScene);
    warpPresent(e);
    updateFps(e);
}

static void* hudThread(void* arg) {
    HudEngine* e = (HudEngine*)arg;
    // own looper for the sensor queue: the java listener path never delivers
    // to a background service on this build, so the HUD reads the same NDK
    // event queue the env uses
    ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorManager* sensorMgr =
        ASensorManager_getInstanceForPackage("gitlab.neosalsa.hud");
    e->sensorMgr = sensorMgr;
    // pick any handle other than the env's default non-wakeup one: this HAL
    // only emits the first-flush meta event on a real activation, so a
    // second connection on the streaming handle stays "First flush pending"
    // forever and never sees a sample. Non-wakeup first - the wakeup handle
    // works but the vendor ack path spams sendAck errors per event
    const ASensor* def = ASensorManager_getDefaultSensor(sensorMgr,
        ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    ASensorList list = nullptr;
    const int n = ASensorManager_getSensorList(sensorMgr, &list);
    const ASensor* rot = nullptr;
    for (int i = 0; i < n && !rot; ++i)
        if (ASensor_getType(list[i]) == ASENSOR_TYPE_GAME_ROTATION_VECTOR &&
                list[i] != def && !ASensor_isWakeUpSensor(list[i]))
            rot = list[i];
    for (int i = 0; i < n && !rot; ++i)
        if (ASensor_getType(list[i]) == ASENSOR_TYPE_ROTATION_VECTOR)
            rot = list[i];
    if (!rot) rot = def;
    e->rotSensor = rot;
    e->sensorQueue = ASensorManager_createEventQueue(sensorMgr,
        ALooper_forThread(), kSensorIdent, nullptr, nullptr);
    if (rot) {
        ASensorEventQueue_enableSensor(e->sensorQueue, rot);
        ASensorEventQueue_setEventRate(e->sensorQueue, rot, 2000);
    }
    // GL up front on the pbuffer: panels and display adoption must work even
    // before the overlay is shown for the first time
    initEglContext(e);
    while (e->running) hudFrame(e);
    termDisplay(e);
    return nullptr;
}

// ---------------------------------------------------------------- jni

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeInit(JNIEnv* env, jclass,
                                             jobject ctx, jobject bridge) {
    HudEngine* e = &gHud;
    env->GetJavaVM(&e->vm);
    e->ctx = env->NewGlobalRef(ctx);
    // bridge may be null if the java ctor threw: panels stay dead but the
    // service (and its window) still comes up
    if (bridge) initBridge(e, env, bridge);
    if (!gStarted) {
        gStarted = true;
        pthread_create(&gThread, nullptr, hudThread, e);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeWindow(JNIEnv* env, jclass,
                                                jobject surface) {
    ANativeWindow* nw = ANativeWindow_fromSurface(env, surface);
    ANativeWindow* old = gHud.window.exchange(nw);
    if (old) ANativeWindow_release(old);
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeWindowGone(JNIEnv*, jclass) {
    gHud.windowGone = true;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeKey(JNIEnv*, jclass,
                                            jint code, jint action,
                                            jint repeat) {
    HudEngine* e = &gHud;
    std::lock_guard<std::mutex> l(e->keyMu);
    e->keyQ.push_back({(int)code, (int)action, (int)repeat});
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeRecenter(JNIEnv*, jclass) {
    wantRecenter();
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeHoldStart(JNIEnv*, jclass) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    gHud.holdStartMs = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeHoldEnd(JNIEnv*, jclass) {
    gHud.holdStartMs = 0;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeShutdown(JNIEnv*, jclass) {
    gHud.running = false;
}
