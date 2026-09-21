#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/sensor.h>

#include "text/font.h"

struct Eye { GLuint fbo = 0, tex = 0, depth = 0; int w = 0, h = 0; };

// Render + sensor state shared by the two shell processes:
//   env (gitlab.neosalsa.home, PanelActivity) - the HOME activity; owns the
//     physical display surface and draws only the scenery
//   hud (gitlab.neosalsa.hud, HudService) - a system-overlay service; owns
//     panels, virtual displays and input, draws chrome over anything
// HudEngine extends this with everything only the HUD needs.
struct Engine {
    JavaVM* vm = nullptr;
    jobject ctx = nullptr;      // activity/service instance; its ClassLoader
                                // is the only one that sees app classes

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig eglConfig = nullptr;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLSurface pbuffer = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width = 0, height = 0;
    bool ready = false;    // window surface valid
    bool glInit = false;   // programs/buffers/targets created once per context

    GLuint sceneProg = 0, warpProg = 0, textProg = 0, floatProg = 0,
           shapeProg = 0, holdProg = 0, iconProg = 0;
    GLuint quadVbo = 0, gridVbo = 0, textVbo = 0, panelVbo = 0, skyVbo = 0;
    int gridVerts = 0, skyVerts = 0;
    Font font;
    Eye eye[2];

    // env feeds this from ASensorEventQueue; the HUD fills it from its java
    // sensor listener instead (a service has no ALooper queue of its own)
    ASensorManager* sensorMgr = nullptr;
    const ASensor* rotSensor = nullptr;
    ASensorEventQueue* sensorQueue = nullptr;
    float quat[4] = {0, 0, 0, 1};
    bool haveQuat = false;
    float lastQ[4] = {0, 0, 0, 0};

    // 6DoF head position in the sensor world frame, fed by qvrservice via
    // sensor/qvr.cpp; opaque client handle lives in qvrClient
    float headPos[3] = {0, 0, 0};
    bool headPosValid = false;
    bool quatFromQvr = false;   // rot-vec dead: e->quat carries the QVR quat
    int  qvrState = -1;         // raw service state: 3 tracked, other degraded, -1 gone
    void* qvrClient = nullptr;

    bool covered = false;        // a fullscreen app owns the physical display
    float gazeYaw = 0.0f;        // world yaw the user currently faces
    float gazePitch = 0.0f;      // world pitch the user currently faces

    char hud[96] = "";
    int  hudLen = 0;
    int  frames = 0;
    int  fps = 0;
    long long fpsMark = 0;
    int  sensorEv = 0;
    int  sensorHz = 0;
    int  sensorNew = 0;
    int  sensorNewHz = 0;
};
