// The home environment for the Pico Neo 2: a NativeActivity that owns the
// physical display and renders only the scenery. All UI chrome - panels,
// the app library, the summonable menu - lives in the HUD service
// (gitlab.neosalsa.hud), a system overlay that draws over this window or
// over a running VR app. This process only has to be a valid HOME target
// and a pretty place to stand.
//
// CoverWatch (Java side of this APK) polls the task list and feeds
// nativeSetCovered so the render loop can sleep while a fullscreen app owns
// the panel.

#include "../engine.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../common/jni.h"
#include "../math/head.h"
#include "../render/egl.h"
#include "../render/frame.h"
#include "../render/scene.h"
#include "../sensor/sensor.h"
#include "../sensor/qvr.h"

#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cmath>
#include <cstring>

#ifndef AWINDOW_FLAG_FULLSCREEN
#define AWINDOW_FLAG_FULLSCREEN     0x00000400
#endif
#ifndef AWINDOW_FLAG_KEEP_SCREEN_ON
#define AWINDOW_FLAG_KEEP_SCREEN_ON 0x00000080
#endif

static Engine* gEnv = nullptr;

// CoverWatch: a non-env task grabbed display 0, or let it go
extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_home_CoverWatch_nativeSetCovered(JNIEnv*, jclass,
                                                    jboolean c) {
    if (gEnv) gEnv->covered = c == JNI_TRUE;
}

static void drawFrame(Engine* e) {
    // no GL context at all yet (window never arrived): nothing to do
    if (e->context == EGL_NO_CONTEXT) { usleep(50000); return; }

    qvrPoll(e);
    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    const float fakePos[3] = {propF("debug.vrhome.fpx", 0.0f),
                              propF("debug.vrhome.fpy", 0.0f),
                              propF("debug.vrhome.fpz", 0.0f)};
    const float sensRoll = e->quatFromQvr ? propF("debug.vrhome.qvrsensroll", 0.0f)
                                          : propF("debug.vrhome.sensroll", kSensRoll);
    const float worldX = e->quatFromQvr ? propF("debug.vrhome.qvrworldx", 0.0f)
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
    const Mat4 head = headMatrix(e->quat, propI("debug.vrhome.tq", 1) != 0,
        sensRoll, worldX,
        propF("debug.vrhome.roll",     kRoll), useSensor, headPos);
    float gy;
    if (gazeYaw(head, &gy)) e->gazeYaw = gy;

    // covered by a fullscreen app: nothing to present and no vsync to pace
    // us, but the GL context stays warm on the pbuffer
    if (!e->ready || e->covered) { usleep(33000); return; }

    updateHud(e, "");
    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const float fov = propF("debug.vrhome.fov", kFovY);
    drawEyes(e, head, perspective(fov, aspect, 0.05f, 100.0f), false,
             drawScene);
    warpPresent(e);
    updateFps(e);
}

static void onAppCmd(android_app* app, int32_t cmd) {
    Engine* e = (Engine*)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) initWindow(e, app->window);
        break;
    case APP_CMD_TERM_WINDOW:
        termWindow(e);
        break;
    // keep sensors running regardless of focus: the HUD draws from the same
    // head pose source and must not freeze when its window is the focused one
    }
}

static void hideSystemUi(android_app* app) {
    JNIEnv* env = threadEnv(app->activity->vm);
    jobject activity = app->activity->clazz;
    jclass actCls = env->GetObjectClass(activity);
    jmethodID getWindow = env->GetMethodID(actCls, "getWindow",
                                         "()Landroid/view/Window;");
    jobject win = env->CallObjectMethod(activity, getWindow);
    jclass winCls = env->GetObjectClass(win);
    jmethodID getDecor = env->GetMethodID(winCls, "getDecorView",
                                        "()Landroid/view/View;");
    jobject decor = env->CallObjectMethod(win, getDecor);
    jclass viewCls = env->FindClass("android/view/View");
    jmethodID setVis = env->GetMethodID(viewCls, "setSystemUiVisibility",
                                      "(I)V");
    // IMMERSIVE_STICKY|HIDE_NAVIGATION|FULLSCREEN|LAYOUT_STABLE|
    // LAYOUT_HIDE_NAVIGATION|LAYOUT_FULLSCREEN
    env->CallVoidMethod(decor, setVis,
                        0x1000 | 0x0002 | 0x0004 | 0x0100 | 0x0200 | 0x0400);
}

void android_main(android_app* app) {
    Engine e{};
    gEnv = &e;
    e.vm = app->activity->vm;
    e.ctx = app->activity->clazz;
    app->userData = &e;
    app->onAppCmd = onAppCmd;
    app->onInputEvent = nullptr;   // all keys belong to the HUD now

    ANativeActivity_setWindowFlags(app->activity,
        AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    ASensorManager* sensorMgr =
        ASensorManager_getInstanceForPackage("gitlab.neosalsa.home");
    e.sensorMgr = sensorMgr;
    const ASensor* rot = ASensorManager_getDefaultSensor(sensorMgr,
        ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    if (!rot)
        rot = ASensorManager_getDefaultSensor(sensorMgr,
            ASENSOR_TYPE_ROTATION_VECTOR);
    e.rotSensor = rot;
    e.sensorQueue = ASensorManager_createEventQueue(sensorMgr,
        app->looper, kSensorIdent, nullptr, nullptr);
    if (rot) {
        ASensorEventQueue_enableSensor(e.sensorQueue, rot);
        ASensorEventQueue_setEventRate(e.sensorQueue, rot, 2000);
    }

    hideSystemUi(app);

    while (!app->destroyRequested) {
        int ident, events;
        android_poll_source* src;
        while ((ident = ALooper_pollOnce(0, nullptr, &events, (void**)&src)) >= 0) {
            if (src) src->process(app, src);
            if (ident == kSensorIdent) drainSensor(&e);
            if (app->destroyRequested) break;
        }
        drawFrame(&e);
    }
    termDisplay(&e);
    gEnv = nullptr;
}
