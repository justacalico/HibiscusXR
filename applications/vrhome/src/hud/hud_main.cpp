// The HUD: panels, the app-grid overlay and the summonable menu, drawn
// into a fullscreen TYPE_SYSTEM_OVERLAY window owned by HudService.
// Virtual displays live in this process (ShellBridge is constructed on
// the java side), so every panel keeps running no matter which app owns
// the physical display underneath.
//
// Threading: HudService runs on the main thread and feeds this file through
// JNI - surface post/gone, key presses. One pthread runs the render loop and
// owns the sensor queue; all GL and bridge calls happen on it.

#include "engine.h"
#include "dash.h"
#include "debug_hooks.h"
#include "status.h"
#include "quiet.h"

#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../grid/grid.h"
#include "../grid/layout.h"
#include "../notif/notif.h"
#include "../notif/layout.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../input/input.h"
#include "../input/ctrl.h"
#include "../input/ctrl_debug.h"
#include "../kbd/kbd.h"
#include "../math/head.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../pill/pill.h"
#include "../render/chrome.h"
#include "../render/ctrl_render.h"
#include "../render/egl.h"
#include "../render/frame.h"
#include "../render/warp.h"
#include "../sensor/sensor.h"
#include "../sensor/qvr.h"
#include "../sysmsg/layout.h"
#include "../sysmsg/sysmsg.h"

#include <android/looper.h>
#include <android/native_window.h>
#include <android/sensor.h>
#include <android/native_window_jni.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <pthread.h>

// postMs arrives on the wall-clock epoch, so expiry compares against
// CLOCK_REALTIME, not the monotonic clock the rest of the engine uses
static long long epochNowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static HudEngine gHud;
static pthread_t gThread;
static bool gStarted = false;

// gathers the live state into the POD the pure rule consumes; pose deltas
// are read off prevAim, which is refreshed on every call - busy or not
static void hudQuietSample(HudEngine* e, HudQuietIn* q) {
    *q = {};
    q->panels = !e->panels.empty();
    q->grid = e->grid.openT > 0.0f || e->grid.shown;
    q->notifs = !e->notifs.empty();
    q->sysmsgs = !e->sysMsgs.empty();
    q->toastOnly = e->toastOnly;
    q->sysMsgOnly = e->sysMsgOnly;
    q->debugOnly = e->debugOnly;
    q->kbd = e->kbd.only || e->kbd.shown || e->kbd.pressed || e->kbd.moveHeld;
    q->holdRing = e->holdStartMs != 0 || e->holdP > 0.0f;
    q->held = e->confirmHeld || e->moveHeld || e->sysPress;
    q->presses = e->dockPress >= 0 || e->shelfPress >= 0 ||
                 e->notifPress >= 0 || e->sysMsgPress >= 0 ||
                 e->grid.press >= 0 || e->grid.scrollHeld ||
                 e->pressDisp >= 0 || e->dragDisp >= 0 ||
                 e->dockPinP > 0.0f;
    q->recentSummon = e->frameMs - e->summonMs < 1500;
    {
        std::lock_guard<std::mutex> lk(e->keyMu);
        q->inputPending = !e->ctrlEv.empty() || !e->keyQ.empty();
    }
    const float dx = e->aimO[0] - e->prevAimO[0];
    const float dy = e->aimO[1] - e->prevAimO[1];
    const float dz = e->aimO[2] - e->prevAimO[2];
    const float ux = e->aimD[0] - e->prevAimD[0];
    const float uy = e->aimD[1] - e->prevAimD[1];
    const float uz = e->aimD[2] - e->prevAimD[2];
    q->aimMoved = dx * dx + dy * dy + dz * dz > 0.0001f ||
                  ux * ux + uy * uy + uz * uz > 0.0025f;
    memcpy(e->prevAimO, e->aimO, sizeof(e->prevAimO));
    memcpy(e->prevAimD, e->aimD, sizeof(e->prevAimD));
}

static void hudFrame(HudEngine* e) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000 +
                          ts.tv_nsec / 1000000;
    e->frameMs = now;

    // surface handoff from the SurfaceHolder callbacks
    ANativeWindow* nw = e->window.exchange(nullptr);
    if (nw) {
        // a fresh surface after being hidden is a summon: the strip plays
        // its slide/fade-in from this stamp
        const bool wasReady = e->ready;
        if (e->ready) termWindow(e);
        if (initWindow(e, nw) != 0) LOGE("initWindow failed");
        else if (!wasReady) e->summonMs = now;
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

    // dev hook: force the app grid open (1) or shut (0) for headless
    // testing; unset (-1) leaves the state to the UI
    const int gprop = propI("debug.vrhome.grid", -1);
    if (gprop == 1) e->grid.shown = true;
    else if (gprop == 0) e->grid.shown = false;

    qvrPoll(e);
    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    smoothPose(e, useSensor);
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

    ctrlTick(e, head, sensRoll, worldX, propF("debug.vrhome.roll", kRoll),
             now);

    runDebugHooks(e);

    tickDash(e, head, now);

    // hidden or no surface: management above must still run - pumpBridge is
    // what adopts strays and keeps the displays alive - but there is nothing
    // to present and no vsync to pace us
    if (!e->ready) { usleep(33000); return; }

    // covered and quiet: with no panels, cards, keyboard or gestures up the
    // only thing on screen is a static strip and cursor. A stereo+warp pass
    // every vsync then only starves the app underneath of GPU, so presents
    // drop to a low cadence; anything changing flips back on the next tick
    if (e->covered) {
        HudQuietIn q;
        hudQuietSample(e, &q);
        if (hudQuietBusy(q)) e->lastBusyMs = now;
        if (hudQuiet(q, now - e->lastBusyMs, 1000) &&
            ++e->quietDiv < 6) { usleep(8000); return; }
    }
    e->quietDiv = 0;

    char extra[48];
    snprintf(extra, sizeof(extra), "  PNL %zu%s", e->panels.size(),
             e->backend ? "" : "  BACKEND:OFF");
    updateHud(e, extra);
    fmtCtrlLine(e->hud2, sizeof(e->hud2), e->input, e->ctrl, e->ctrlPos,
                e->ctrlDir);
    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const float fov = propF("debug.vrhome.fov", kFovY);
    drawEyes(e, head, perspective(fov, aspect, 0.05f, 100.0f), true,
             statusLineVisible(propI("debug.vrhome.hud", -1), e->debugHud),
             drawDashScene);
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
