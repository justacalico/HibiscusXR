#pragma once

#include <GLES2/gl2.h>

#ifdef __ANDROID__
#include <jni.h>
#include <EGL/egl.h>
#include <android/sensor.h>
#endif

#include "text/font.h"
#include "math/head.h"

struct Eye { GLuint fbo = 0, tex = 0, depth = 0; int w = 0, h = 0; };

// Render + sensor state shared by the shell processes. The GL fields are
// platform-neutral; the Android process handles (JNI, EGL, sensors) are
// compiled out for the desktop preview.
struct Engine {
#ifdef __ANDROID__
    JavaVM* vm = nullptr;
    jobject ctx = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig eglConfig = nullptr;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLSurface pbuffer = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
#endif
    int width = 0, height = 0;
    bool ready = false;
    bool glInit = false;

    GLuint sceneProg = 0, warpProg = 0, textProg = 0, floatProg = 0,
           shapeProg = 0, holdProg = 0, iconProg = 0;
    GLuint quadVbo = 0, gridVbo = 0, textVbo = 0, panelVbo = 0, skyVbo = 0;
    GLuint glyphTex = 0;
    int gridVerts = 0, skyVerts = 0;
    Font font;
    Eye eye[2];

#ifdef __ANDROID__
    ASensorManager* sensorMgr = nullptr;
    const ASensor* rotSensor = nullptr;
    ASensorEventQueue* sensorQueue = nullptr;
#endif
    float quat[4] = {0, 0, 0, 1};
    bool haveQuat = false;
    float lastQ[4] = {0, 0, 0, 0};

    float headPos[3] = {0, 0, 0};
    bool headPosValid = false;
    bool quatFromQvr = false;
    float headAv[3] = {0, 0, 0};
    int  qvrState = -1;
    void* qvrClient = nullptr;

    PoseFilt viewPose;
    long long viewPoseMs = 0;

    bool covered = false;

#ifdef __ANDROID__
    jobject ptSt = nullptr;
    jobject ptStArr = nullptr;
    jmethodID stUpdate = nullptr, stMatrix = nullptr;
    void* ptCam = nullptr;
#endif
    GLuint ptTex = 0;
    GLuint ptVbo[2] = {0, 0};
    int ptVerts[2] = {0, 0};
    float ptMat[16] = {};
    bool ptLive = false;
    int curEye = 0;

    int envMode = 0;
    char envSel[96] = "";
    GLuint envVbo = 0;
    int envVerts = 0;
    long long envPollAt = 0;
    long long envRetryAt = 0;

    float gazeYaw = 0.0f;
    float gazePitch = 0.0f;

    char hud[128] = "";
    int  hudLen = 0;
    char hud2[128] = "";
    int  frames = 0;
    int  fps = 0;
    long long fpsMark = 0;
    int  presents = 0;
    int  presentFps = -1;
    long long lastPresentNs = -1;
    long long waitFrameId = 0;
    bool presentOk = false;
    int  sensorEv = 0;
    int  sensorHz = 0;
    int  sensorNew = 0;
    int  sensorNewHz = 0;
};
