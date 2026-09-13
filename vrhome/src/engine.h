#pragma once

#include <android_native_app_glue.h>
#include <android/sensor.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <vector>

#include "panels/panel.h"
#include "text/font.h"

struct Eye { GLuint fbo = 0, tex = 0, depth = 0; int w = 0, h = 0; };

struct Engine {
    android_app* app = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig eglConfig = nullptr;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLSurface pbuffer = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width = 0, height = 0;
    bool ready = false;    // window surface valid
    bool glInit = false;   // programs/buffers/targets created once per context

    GLuint sceneProg = 0, warpProg = 0, textProg = 0, floatProg = 0,
           shapeProg = 0;
    GLuint quadVbo = 0, gridVbo = 0, textVbo = 0, panelVbo = 0, skyVbo = 0;
    int gridVerts = 0, skyVerts = 0;
    Font font;
    Eye eye[2];

    std::vector<Panel> panels;

    // ShellBridge java object + cached method ids
    jobject bridge = nullptr;
    jmethodID mCreatePanel = nullptr, mPanelTex = nullptr, mLaunchPkg = nullptr,
              mLaunchLauncher = nullptr, mAdopt = nullptr, mReleasePanel = nullptr,
              mTakeAdopt = nullptr, mTakeRelease = nullptr, mInjectTap = nullptr,
              mInjectTouch = nullptr,
              mRemoveTask = nullptr, mFocusTask = nullptr, mAppLabel = nullptr;
    jmethodID stUpdate = nullptr, stMatrix = nullptr;
    jclass pendingCls = nullptr;
    jfieldID fPendTask = nullptr, fPendPkg = nullptr;
    bool bridgeDead = false;

    ASensorManager* sensorMgr = nullptr;
    const ASensor* rotSensor = nullptr;
    ASensorEventQueue* sensorQueue = nullptr;
    float quat[4] = {0, 0, 0, 1};
    bool haveQuat = false;

    int hover = -1;              // panel index under the gaze ray
    float hitX = 0, hitY = 0;    // display px coords of the hit
    float gazeYaw = 0.0f;        // world yaw the user currently faces
    bool launcherSpawned = false;
    bool confirmHeld = false;
    int dragDisp = -1;           // display a confirm-drag started on
    float dragX = 0, dragY = 0;  // last injected drag position, px
    float grabX = 0, grabY = 0;  // where the drag grabbed, px

    char hud[96] = "";
    int  hudLen = 0;
    int  frames = 0;
    int  fps = 0;
    long long fpsMark = 0;
    int  sensorEv = 0;
    int  sensorHz = 0;
    int  sensorNew = 0;
    int  sensorNewHz = 0;
    float lastQ[4] = {0,0,0,0};
};
