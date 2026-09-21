// Minimal stereo VR renderer for the Pico Neo 2 running a plain Android 10 GSI.
//
// Deliberately does NOT use the Pico/Qualcomm VR stack. This is the "does the
// open path work at all" test: standard EGL + GLES2 on a normal fullscreen
// surface, split into two eye viewports with barrel distortion applied in the
// fragment shader, and 3DoF orientation pulled from the IMU through the
// NDK-stable ASensorManager. If this looks right in the lenses then the display,
// the Adreno driver and the sensor HAL are all usable without any closed
// compositor, which is what the port actually needs to prove.
//
// Panel is 2160x3840 portrait; forced landscape gives us 3840x2160 and each eye
// takes half. No timewarp, no reprojection, no positional tracking - those come
// later and belong in a real runtime, not in a smoke test.

#include <android/log.h>
#include <android/native_activity.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

// Not exposed by every NDK revision's headers; these are the stable framework
// WindowManager.LayoutParams values and are safe to spell out.
#ifndef AWINDOW_FLAG_FULLSCREEN
#define AWINDOW_FLAG_FULLSCREEN     0x00000400
#endif
#ifndef AWINDOW_FLAG_KEEP_SCREEN_ON
#define AWINDOW_FLAG_KEEP_SCREEN_ON 0x00000080
#endif
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <sys/system_properties.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

#define TAG "pn2vr"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Everything below is live-tunable with setprop, re-read once a second. Testing
// in a headset is slow - you have to put it on, look, take it off - so a rebuild
// per candidate value is the wrong loop. Defaults are what the code starts with.
//
//   setprop debug.pn2vr.k1     0.22    barrel term
//   setprop debug.pn2vr.k2     0.24    barrel term
//   setprop debug.pn2vr.ipd    0.063   metres between eye centres
//   setprop debug.pn2vr.fov    90      vertical degrees
//   setprop debug.pn2vr.roll     90    in-plane image roll, degrees
//   setprop debug.pn2vr.worldx   90    Y-up scene -> Android's Z-up ENU world
//   setprop debug.pn2vr.sensroll 0     spare device-space Z term
//   setprop debug.pn2vr.sensor   1     0=off 1=game_rv 2=rotation_vector
//
// The sensor default is only safe because libsensorservice.so is patched. Stock
// Android 10 fatally CHECKs on any sensor event type between 36 and 65535, and
// Pico's HAL emits 57, 58, 126 and 127 - which crash-loops system_server and
// reboots the device. See overlay/README.md. On an unpatched build set
// debug.pn2vr.sensor=0.
static float kDistK1 = 0.22f;
static float kDistK2 = 0.24f;
static float kIPD    = 0.063f;   // metres between eye centres
static float kFovY   = 90.0f;    // degrees, vertical
// Confirmed correct in the headset on the Neo 2: roll 90, sensroll 0, worldx 90.
static float kRoll     = 90.0f;  // degrees, in-plane roll of the final image
static float kSensRoll = 0.0f;   // degrees, spare device-space Z term
static float kWorldX   = 90.0f;  // degrees, Y-up scene -> Android's Z-up world
static int   kSensor   = 1;      // 1 = 3DoF from the IMU (safe post sensor patch)

// looper ident for our sensor queue; must match what we poll for
static const int kSensorIdent = 3;

static float propF(const char* key, float dflt) {
    char buf[PROP_VALUE_MAX];
    if (__system_property_get(key, buf) > 0) return (float)atof(buf);
    return dflt;
}

static int propI(const char* key, int dflt) {
    char buf[PROP_VALUE_MAX];
    if (__system_property_get(key, buf) > 0) return atoi(buf);
    return dflt;
}

static void refreshTunables() {
    kDistK1 = propF("debug.pn2vr.k1",   0.22f);
    kDistK2 = propF("debug.pn2vr.k2",   0.24f);
    kIPD    = propF("debug.pn2vr.ipd",  0.063f);
    kFovY   = propF("debug.pn2vr.fov",  90.0f);
    kRoll     = propF("debug.pn2vr.roll",     0.0f);
    kSensRoll = propF("debug.pn2vr.sensroll", 0.0f);
    kWorldX   = propF("debug.pn2vr.worldx",  90.0f);
}

// ---------------------------------------------------------------- math

struct Mat4 { float m[16]; };

static Mat4 identity() {
    Mat4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f; return r;
}

static Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 4; ++i) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + i] * b.m[c * 4 + k];
            r.m[c * 4 + i] = s;
        }
    return r;
}

static Mat4 perspective(float fovYDeg, float aspect, float zn, float zf) {
    Mat4 r{};
    const float f = 1.0f / tanf(fovYDeg * (float)M_PI / 360.0f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zf + zn) / (zn - zf);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zf * zn) / (zn - zf);
    return r;
}

static Mat4 translate(float x, float y, float z) {
    Mat4 r = identity(); r.m[12] = x; r.m[13] = y; r.m[14] = z; return r;
}

// rotation about Z, degrees
static Mat4 rotZ(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[0] =  cosf(r); m.m[1] = sinf(r);
    m.m[4] = -sinf(r); m.m[5] = cosf(r);
    return m;
}

// rotation about X, degrees
static Mat4 rotX(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[5] =  cosf(r); m.m[6] = sinf(r);
    m.m[9] = -sinf(r); m.m[10] = cosf(r);
    return m;
}

// Quaternion (x,y,z,w) -> rotation matrix. The sensor gives us head orientation;
// the view matrix wants its inverse, so transpose the rotation part.
static Mat4 quatToView(const float* q) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    Mat4 r = identity();
    r.m[0] = 1 - 2 * (y * y + z * z);  r.m[4] = 2 * (x * y + z * w);      r.m[8]  = 2 * (x * z - y * w);
    r.m[1] = 2 * (x * y - z * w);      r.m[5] = 1 - 2 * (x * x + z * z);  r.m[9]  = 2 * (y * z + x * w);
    r.m[2] = 2 * (x * z + y * w);      r.m[6] = 2 * (y * z - x * w);      r.m[10] = 1 - 2 * (x * x + y * y);
    return r;
}

// ---------------------------------------------------------------- shaders

static const char* kSceneVS = R"(
attribute vec3 aPos;
attribute vec3 aCol;
uniform mat4 uMVP;
varying vec3 vCol;
void main() {
    vCol = aCol;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kSceneFS = R"(
precision mediump float;
varying vec3 vCol;
void main() { gl_FragColor = vec4(vCol, 1.0); }
)";

static const char* kWarpVS = R"(
attribute vec2 aPos;
varying vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Pre-distort so the lens's own pincushion cancels it. Sampling further out at
// the edges squeezes the image inward, i.e. barrel. Anything that lands off the
// eye texture is black rather than clamped, so the edge doesn't smear.
static const char* kWarpFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uLensCenter;
uniform float uK1;
uniform float uK2;
void main() {
    vec2 p = vUV - uLensCenter;
    float r2 = dot(p, p);
    float scale = 1.0 + uK1 * r2 + uK2 * r2 * r2;
    vec2 uv = uLensCenter + p * scale;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = texture2D(uTex, uv);
    }
}
)";

// ---------------------------------------------------------------- gl helpers

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        LOGE("shader compile failed: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint link(const char* vs, const char* fs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        LOGE("program link failed: %s", log);
        glDeleteProgram(p);
        p = 0;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// ---------------------------------------------------------------- geometry

// unit cube, 36 verts, per-face colour so orientation is unambiguous
static const float kCube[] = {
#define F(r,g,b, x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4) \
    x1,y1,z1, r,g,b,  x2,y2,z2, r,g,b,  x3,y3,z3, r,g,b, \
    x1,y1,z1, r,g,b,  x3,y3,z3, r,g,b,  x4,y4,z4, r,g,b,
    F(0.9f,0.2f,0.2f, -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1)   // +Z
    F(0.2f,0.9f,0.2f,  1,-1,-1, -1,-1,-1, -1, 1,-1,  1, 1,-1)   // -Z
    F(0.2f,0.4f,0.9f,  1,-1, 1,  1,-1,-1,  1, 1,-1,  1, 1, 1)   // +X
    F(0.9f,0.9f,0.2f, -1,-1,-1, -1,-1, 1, -1, 1, 1, -1, 1,-1)   // -X
    F(0.9f,0.5f,0.1f, -1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1)   // +Y
    F(0.6f,0.2f,0.8f, -1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1)   // -Y
#undef F
};

// floor grid, built once; gives the eye something to judge scale and stability by
static const int   kGridHalf  = 10;
static const float kGridStep  = 0.5f;
static float       gGrid[(kGridHalf * 2 + 1) * 4 * 6];
static int         gGridVerts = 0;

static void buildGrid() {
    int n = 0;
    const float e = kGridHalf * kGridStep;
    for (int i = -kGridHalf; i <= kGridHalf; ++i) {
        const float t = i * kGridStep;
        const bool axis = (i == 0);
        const float r = axis ? 0.9f : 0.25f, g = axis ? 0.9f : 0.35f, b = axis ? 0.9f : 0.45f;
        const float pts[4][3] = {{t, 0, -e}, {t, 0, e}, {-e, 0, t}, {e, 0, t}};
        for (int k = 0; k < 4; ++k) {
            gGrid[n++] = pts[k][0]; gGrid[n++] = pts[k][1]; gGrid[n++] = pts[k][2];
            gGrid[n++] = r;         gGrid[n++] = g;         gGrid[n++] = b;
        }
    }
    gGridVerts = n / 6;
}

// ---------------------------------------------------------------- engine

struct Eye {
    GLuint fbo = 0, tex = 0, depth = 0;
    int w = 0, h = 0;
};

struct Engine {
    android_app* app = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width = 0, height = 0;
    bool ready = false;

    GLuint sceneProg = 0, warpProg = 0;
    GLuint cubeVbo = 0, gridVbo = 0, quadVbo = 0;
    Eye eye[2];

    ASensorManager*    sensorMgr   = nullptr;
    const ASensor*     rotSensor   = nullptr;
    ASensorEventQueue* sensorQueue = nullptr;
    float quat[4] = {0, 0, 0, 1};
    bool  haveQuat = false;

    float t = 0.0f;
    int   tuneTick = 0;
};

static bool initEyeTargets(Engine* e) {
    const int ew = e->width / 2, eh = e->height;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        if (y.fbo) { glDeleteFramebuffers(1, &y.fbo); glDeleteTextures(1, &y.tex); glDeleteRenderbuffers(1, &y.depth); }
        y.w = ew; y.h = eh;

        glGenTextures(1, &y.tex);
        glBindTexture(GL_TEXTURE_2D, y.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ew, eh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenRenderbuffers(1, &y.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, y.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, ew, eh);

        glGenFramebuffers(1, &y.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, y.tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, y.depth);

        const GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) { LOGE("eye %d FBO incomplete: 0x%x", i, st); return false; }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    LOGI("eye targets ready: %dx%d each", ew, eh);
    return true;
}

static int initDisplay(Engine* e) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, nullptr, nullptr);

    EGLConfig config; EGLint numConfigs = 0;
    eglChooseConfig(dpy, attribs, &config, 1, &numConfigs);
    if (numConfigs < 1) { LOGE("no EGL config"); return -1; }

    EGLint format = 0;
    eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(e->app->window, 0, 0, format);

    EGLSurface surf = eglCreateWindowSurface(dpy, config, e->app->window, nullptr);
    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctxAttribs);
    if (eglMakeCurrent(dpy, surf, surf, ctx) == EGL_FALSE) { LOGE("eglMakeCurrent failed"); return -1; }

    eglQuerySurface(dpy, surf, EGL_WIDTH,  &e->width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &e->height);
    e->display = dpy; e->surface = surf; e->context = ctx;

    LOGI("GL_VENDOR=%s", glGetString(GL_VENDOR));
    LOGI("GL_RENDERER=%s", glGetString(GL_RENDERER));
    LOGI("GL_VERSION=%s", glGetString(GL_VERSION));
    LOGI("surface %dx%d", e->width, e->height);

    e->sceneProg = link(kSceneVS, kSceneFS);
    e->warpProg  = link(kWarpVS,  kWarpFS);
    if (!e->sceneProg || !e->warpProg) return -1;

    buildGrid();
    glGenBuffers(1, &e->cubeVbo);
    glBindBuffer(GL_ARRAY_BUFFER, e->cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCube), kCube, GL_STATIC_DRAW);

    glGenBuffers(1, &e->gridVbo);
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glBufferData(GL_ARRAY_BUFFER, gGridVerts * 6 * sizeof(float), gGrid, GL_STATIC_DRAW);

    const float quad[] = { -1,-1,  1,-1,  -1, 1,   1,-1,  1, 1,  -1, 1 };
    glGenBuffers(1, &e->quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    if (!initEyeTargets(e)) return -1;

    glEnable(GL_DEPTH_TEST);
    e->ready = true;
    return 0;
}

static void termDisplay(Engine* e) {
    if (e->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(e->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (e->context != EGL_NO_CONTEXT) eglDestroyContext(e->display, e->context);
        if (e->surface != EGL_NO_SURFACE) eglDestroySurface(e->display, e->surface);
        eglTerminate(e->display);
    }
    e->display = EGL_NO_DISPLAY;
    e->context = EGL_NO_CONTEXT;
    e->surface = EGL_NO_SURFACE;
    e->ready = false;
}

static void drawScene(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);

    // floor grid, at world origin
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glDrawArrays(GL_LINES, 0, gGridVerts);

    // ring of cubes, one spinning, so judder and tracking lag are obvious
    glBindBuffer(GL_ARRAY_BUFFER, e->cubeVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    for (int i = 0; i < 6; ++i) {
        const float a = (float)i / 6.0f * 2.0f * (float)M_PI;
        Mat4 model = translate(cosf(a) * 3.0f, 0.3f, sinf(a) * 3.0f);
        const float s = 0.25f;
        Mat4 scale = identity();
        scale.m[0] = scale.m[5] = scale.m[10] = s;
        Mat4 spin = identity();
        const float c = cosf(e->t + a), sn = sinf(e->t + a);
        spin.m[0] = c; spin.m[2] = -sn; spin.m[8] = sn; spin.m[10] = c;
        const Mat4 mvp = multiply(viewProj, multiply(model, multiply(spin, scale)));
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, mvp.m);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
}

// Drain the IMU and keep only the newest sample.
//
// This MUST be called from inside the ALooper_pollAll loop, not from drawFrame.
// The sensor queue is attached to the looper, so while events are pending
// pollAll keeps returning >= 0; if the only place we drain is after the loop,
// the loop never exits, drawFrame never runs, and the scene freezes while the
// queue grows. That is a livelock you only see once a sensor is actually
// attached - with debug.pn2vr.sensor=0 everything renders fine.
static void drainSensor(Engine* e) {
    if (!e->sensorQueue) return;
    ASensorEvent ev;
    while (ASensorEventQueue_getEvents(e->sensorQueue, &ev, 1) > 0) {
        if (ev.type == ASENSOR_TYPE_GAME_ROTATION_VECTOR ||
            ev.type == ASENSOR_TYPE_ROTATION_VECTOR) {
            e->quat[0] = ev.data[0];
            e->quat[1] = ev.data[1];
            e->quat[2] = ev.data[2];
            // some HALs leave w unset; rebuild it from the vector part
            const float sq = ev.data[0]*ev.data[0] + ev.data[1]*ev.data[1] + ev.data[2]*ev.data[2];
            e->quat[3] = (ev.data[3] != 0.0f) ? ev.data[3] : (sq < 1.0f ? sqrtf(1.0f - sq) : 0.0f);
            e->haveQuat = true;
        }
    }
}

static void drawFrame(Engine* e) {
    if (!e->ready) return;

    e->t += 1.0f / 72.0f;   // panel runs at 72 Hz

    // pick up setprop changes without a relaunch; once a second is plenty
    if (++e->tuneTick >= 72) { e->tuneTick = 0; refreshTunables(); }

    // Two DIFFERENT rotations, which is why one knob could not satisfy both:
    //
    //   sensroll - the sensor reports in the panel's natural (portrait) frame,
    //              but we render landscape, so the sensor axes need remapping
    //              before use. Post-multiply: acts in device space.
    //   roll     - in-plane roll of the finished image. Pre-multiply: acts in
    //              display space, and is what makes a head-locked scene upright.
    //
    // Sharing one value meant correcting the static scene broke head tracking
    // and vice versa.
    // Android's rotation vector is expressed in an ENU world frame: X east,
    // Y north, Z UP. This scene is Y-up. Without converting between them,
    // world-space rotations land on the wrong axes - roll looks fine but pitch
    // and yaw swap, which is exactly what the headset showed.
    //
    //   view = rotZ(roll) * R^T * rotX(worldx)
    //
    // rotX(+90) maps our Y-up world into Android's Z-up world (post-multiply, so
    // it acts in world space). rotZ handles portrait->landscape (pre-multiply,
    // display space). sensroll is kept as a spare device-space Z term.
    Mat4 head = e->haveQuat ? quatToView(e->quat) : identity();
    if (e->haveQuat) {
        head = multiply(head, rotZ(kSensRoll));
        head = multiply(head, rotX(kWorldX));
    }
    head = multiply(rotZ(kRoll), head);

    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const Mat4 proj = perspective(kFovY, aspect, 0.05f, 100.0f);

    for (int i = 0; i < 2; ++i) {
        const Eye& y = e->eye[i];
        glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glViewport(0, 0, y.w, y.h);
        glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float dx = (i == 0 ? +1.0f : -1.0f) * kIPD * 0.5f;
        // head-locked at standing height; no positional tracking yet
        const Mat4 view = multiply(translate(dx, 0.0f, 0.0f), multiply(head, translate(0.0f, -1.6f, 0.0f)));
        drawScene(e, multiply(proj, view));
    }

    // distortion pass straight to the display, one half per eye
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(e->warpProg);

    const GLint aPos2  = glGetAttribLocation(e->warpProg, "aPos");
    const GLint uTex   = glGetUniformLocation(e->warpProg, "uTex");
    const GLint uCen   = glGetUniformLocation(e->warpProg, "uLensCenter");
    const GLint uK1    = glGetUniformLocation(e->warpProg, "uK1");
    const GLint uK2    = glGetUniformLocation(e->warpProg, "uK2");

    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glEnableVertexAttribArray(aPos2);
    glVertexAttribPointer(aPos2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glUniform1f(uK1, kDistK1);
    glUniform1f(uK2, kDistK2);
    glUniform1i(uTex, 0);
    glActiveTexture(GL_TEXTURE0);

    for (int i = 0; i < 2; ++i) {
        glViewport(i * (e->width / 2), 0, e->width / 2, e->height);
        glBindTexture(GL_TEXTURE_2D, e->eye[i].tex);
        glUniform2f(uCen, 0.5f, 0.5f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisableVertexAttribArray(aPos2);
    glEnable(GL_DEPTH_TEST);

    eglSwapBuffers(e->display, e->surface);
}

static void onAppCmd(android_app* app, int32_t cmd) {
    Engine* e = (Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                ANativeActivity_setWindowFlags(app->activity,
                    AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
                if (initDisplay(e) != 0) LOGE("initDisplay failed");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            termDisplay(e);
            break;
        case APP_CMD_GAINED_FOCUS:
            if (e->sensorQueue && e->rotSensor) {
                ASensorEventQueue_enableSensor(e->sensorQueue, e->rotSensor);
                ASensorEventQueue_setEventRate(e->sensorQueue, e->rotSensor, 1000000 / 100);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (e->sensorQueue && e->rotSensor)
                ASensorEventQueue_disableSensor(e->sensorQueue, e->rotSensor);
            break;
        default: break;
    }
}

void android_main(android_app* app) {
    Engine engine{};
    engine.app = app;
    app->userData = &engine;
    app->onAppCmd = onAppCmd;

    refreshTunables();
    kSensor = propI("debug.pn2vr.sensor", 0);

    // Opt-in only. See the note by the tunables: subscribing to a fused sensor
    // makes Pico's HAL stream event types 126/127, which Android 10 turns into a
    // fatal CHECK inside sensorservice and reboots the device.
    if (kSensor == 0) {
        LOGI("sensor disabled (debug.pn2vr.sensor=0), running head-locked");
    } else {
        engine.sensorMgr = ASensorManager_getInstanceForPackage("org.pn2.vrdemo");
        if (engine.sensorMgr) {
            const int want = (kSensor == 2) ? ASENSOR_TYPE_ROTATION_VECTOR
                                            : ASENSOR_TYPE_GAME_ROTATION_VECTOR;
            engine.rotSensor = ASensorManager_getDefaultSensor(engine.sensorMgr, want);
            if (engine.rotSensor) {
                engine.sensorQueue = ASensorManager_createEventQueue(
                        engine.sensorMgr, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS),
                    kSensorIdent, nullptr, nullptr);
                LOGI("rotation sensor: %s", ASensor_getName(engine.rotSensor));
            } else {
                LOGE("requested sensor %d unavailable; running head-locked", kSensor);
            }
        }
    }

    while (true) {
        int events, ident;
        android_poll_source* source;
        // don't block while we have a surface, we want to keep drawing
        while ((ident = ALooper_pollAll(engine.ready ? 0 : -1, nullptr, &events,
                                        (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            // drain here, inside the loop - see drainSensor()
            if (ident == kSensorIdent) drainSensor(&engine);
            if (app->destroyRequested) { termDisplay(&engine); return; }
        }
        drawFrame(&engine);
    }
}
