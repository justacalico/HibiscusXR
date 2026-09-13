// Panel shell for the Pico Neo 2 running a plain Android 10 GSI.
//
// Owns the physical display and renders empty space plus one textured quad
// per virtual display. 2D apps live on those virtual displays: ShellBridge
// (Java side of this APK) creates them, launches/adopts tasks onto them and
// injects input. This process is the HOME app so display 0 always has a
// valid activity; there is no launcher UI in the scene itself - the app
// library is a regular 2D activity on its own panel.
//
// Headset buttons: confirm taps at the gaze point on the hovered panel,
// back closes the newest panel when nothing else has focus, home recenters
// the panel ring on the current gaze.
//
// Layout of the native code:
//   common/   constants, logging, system property readers
//   math/     matrix + head-tracking chain (pure, unit tested on host)
//   panels/   ring layout policy (pure) + display/panel lifecycle
//   bridge/   JNI side: ShellBridge, launch queue, adopt/release pumping
//   render/   shaders, EGL, scenery, window chrome drawing
//   text/     utf8 + glyph layout (pure), font atlas + text drawing
//   input/    headset keys, sensor/  rotation-vector drain

#include "engine.h"
#include "bridge/bridge.h"
#include "common/config.h"
#include "common/log.h"
#include "common/props.h"
#include "input/input.h"
#include "math/head.h"
#include "panels/layout.h"
#include "panels/panels.h"
#include "render/egl.h"
#include "render/scene.h"
#include "sensor/sensor.h"
#include "text/draw.h"

#include <android/native_window.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifndef AWINDOW_FLAG_FULLSCREEN
#define AWINDOW_FLAG_FULLSCREEN     0x00000400
#endif
#ifndef AWINDOW_FLAG_KEEP_SCREEN_ON
#define AWINDOW_FLAG_KEEP_SCREEN_ON 0x00000080
#endif

// test hook: setprop debug.vrhome.launch <pkg> queues a panel launch
static void debugLaunchHook(Engine* e) {
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
static void debugTapHook(Engine* e) {
    static char lastTap[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (__system_property_get("debug.vrhome.tap", tb) > 0 &&
            strcmp(tb, lastTap) != 0) {
        strncpy(lastTap, tb, sizeof(lastTap) - 1);
        int d, x, y;
        if (sscanf(tb, "%d,%d,%d", &d, &x, &y) == 3 && e->bridge) {
            JNIEnv* env = threadEnv(e->app);
            env->CallVoidMethod(e->bridge, e->mInjectTap, d,
                                (float)x, (float)y);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }
}

// one-time: library panel dead ahead once tracking is live. gazeYaw is
// still stale on the first quat frame (pick runs below), so take the yaw
// straight from the head matrix.
static void spawnLauncher(Engine* e, const Mat4& head) {
    if (!e->bridge || !e->haveQuat || e->launcherSpawned) return;
    e->launcherSpawned = true;
    float gy = 0.0f;
    gazeYaw(head, &gy);
    int idx = openPanel(e, gy);
    if (idx >= 0) {
        e->panels[idx].pkg = kLibraryPkg;
        JNIEnv* env = threadEnv(e->app);
        env->CallVoidMethod(e->bridge, e->mLaunchLauncher,
                            e->panels[idx].displayId);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

static void updateHud(Engine* e) {
    float yaw, pitch, roll;
    quatToYpr(e->quat, &yaw, &pitch, &roll);
    e->hudLen = snprintf(e->hud, sizeof(e->hud),
        "YAW %+4.0f PIT %+4.0f ROL %+4.0f  FPS %d  SEN %d  PNL %zu%s",
        yaw, pitch, roll, e->fps, e->sensorNewHz, e->panels.size(),
        e->bridge ? "" : "  BRIDGE:OFF");
}

static void drawEyes(Engine* e, const Mat4& head, const Mat4& proj) {
    static int errTick = 0;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glViewport(0, 0, y.w, y.h);
        if (propI("debug.vrhome.fill", 0))
            glClearColor(i == 0 ? 0.8f : 0.1f, 0.1f, i == 1 ? 0.8f : 0.1f, 1.0f);
        else
            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const Mat4 vp = multiply(proj, eyeMatrix(head, kIPD, i));
        drawScene(e, vp);
        if (propI("debug.vrhome.hud", 1)) drawHud(e, proj);
        if (++errTick >= 144) {
            errTick = 0;
            GLenum ge = glGetError();
            if (ge != GL_NO_ERROR) LOGE("GL error 0x%x", ge);
        }
    }
}

// warp pass: each eye texture through barrel distortion to its half
static void warpPresent(Engine* e) {
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
    glUniform2f(glGetUniformLocation(e->warpProg, "uLensCenter"), 0.5f, 0.5f);
    glUniform1f(glGetUniformLocation(e->warpProg, "uK1"), kDistK1);
    glUniform1f(glGetUniformLocation(e->warpProg, "uK2"), kDistK2);
    for (int i = 0; i < 2; ++i) {
        glViewport(i * e->eye[i].w, 0, e->eye[i].w, e->eye[i].h);
        glBindTexture(GL_TEXTURE_2D, e->eye[i].tex);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisableVertexAttribArray(aPos);
    glEnable(GL_DEPTH_TEST);
    eglSwapBuffers(e->display, e->surface);
}

static void updateFps(Engine* e) {
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

static void drawFrame(Engine* e) {
    // no GL context at all yet (window never arrived): nothing to do
    if (e->context == EGL_NO_CONTEXT) { usleep(50000); return; }

    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    const Mat4 head = headMatrix(e->quat, propI("debug.vrhome.tq", 1) != 0,
        propF("debug.vrhome.sensroll", kSensRoll),
        propF("debug.vrhome.worldx",   kWorldX),
        propF("debug.vrhome.roll",     kRoll), useSensor);

    debugLaunchHook(e);
    debugTapHook(e);
    spawnLauncher(e, head);

    if (takeWantRecenter()) recenterSlots(e->panels, e->gazeYaw);
    pumpBridge(e);

    // gaze pick: nearest panel under the head ray, hit in display px
    const Pick pk = pickPanel(e->panels, head);
    e->hover = pk.idx;
    if (pk.idx >= 0) {
        e->hitX = (pk.u * 0.5f + 0.5f) * kVdW;
        e->hitY = (0.5f - pk.v * 0.5f) * kVdH;
    }
    float gy;
    if (gazeYaw(head, &gy)) e->gazeYaw = gy;

    dragTick(e, head);
    updatePanels(e);

    // covered by a fullscreen app (a VR game or an adopted stray):
    // management above must still run - pumpBridge is what adopts strays and
    // gets our surface back - but there is nothing to present and no vsync
    // to pace us
    if (!e->ready || e->covered) { usleep(33000); return; }

    updateHud(e);
    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    drawEyes(e, head, perspective(kFovY, aspect, 0.05f, 100.0f));
    warpPresent(e);
    updateFps(e);
}

static void onAppCmd(android_app* app, int32_t cmd) {
    Engine* e = (Engine*)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) initWindow(e);
        break;
    case APP_CMD_TERM_WINDOW:
        termWindow(e);
        break;
    // keep sensors running regardless of focus: a focused panel app must
    // not freeze head tracking of the shell that renders it
    }
}

static void hideSystemUi(android_app* app) {
    JNIEnv* env = threadEnv(app);
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
    e.app = app;
    app->userData = &e;
    app->onAppCmd = onAppCmd;
    app->onInputEvent = onInputEvent;

    ANativeActivity_setWindowFlags(app->activity,
        AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    e.sensorMgr = ASensorManager_getInstanceForPackage("org.pn2.vrhome");
    e.rotSensor = ASensorManager_getDefaultSensor(e.sensorMgr,
        ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    if (!e.rotSensor)
        e.rotSensor = ASensorManager_getDefaultSensor(e.sensorMgr,
            ASENSOR_TYPE_ROTATION_VECTOR);
    e.sensorQueue = ASensorManager_createEventQueue(e.sensorMgr,
        app->looper, kSensorIdent, nullptr, nullptr);
    if (e.rotSensor) {
        ASensorEventQueue_enableSensor(e.sensorQueue, e.rotSensor);
        ASensorEventQueue_setEventRate(e.sensorQueue, e.rotSensor, 2000);
    }

    hideSystemUi(app);
    initBridge(&e);

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
}
