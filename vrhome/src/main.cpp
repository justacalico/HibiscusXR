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

#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/sensor.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <sys/system_properties.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#define TAG "vrhome"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#ifndef AWINDOW_FLAG_FULLSCREEN
#define AWINDOW_FLAG_FULLSCREEN     0x00000400
#endif
#ifndef AWINDOW_FLAG_KEEP_SCREEN_ON
#define AWINDOW_FLAG_KEEP_SCREEN_ON 0x00000080
#endif
#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

// tunables confirmed on the headset in the vrdemo: worldx 90
static float kDistK1 = 0.22f, kDistK2 = 0.24f;
static float kIPD  = 0.063f;
static float kFovY = 90.0f;
// roll confirmed on the headset: 90 left the world upside down through the
// lenses, 270 puts it upright. Still live-tunable via debug.vrhome.roll.
static float kRoll = 270.0f, kSensRoll = 0.0f, kWorldX = 90.0f;

static const int kSensorIdent = 3;
static const int kInputIdent  = 4;

// panel defaults: roughly Quest-size panels
static const int   kVdW = 1600, kVdH = 900, kVdDpi = 240;
static const float kPanelDist = 2.2f;    // metres
static const float kPanelW = 1.30f, kPanelH = 0.73f;
static const float kPanelY = 0.05f;      // metres above horizon
static const int   kMaxPanels = 6;
// yaw offsets of the ring slots, relative to ring centre
static const float kSlotYaw[kMaxPanels] =
    {0.0f, -0.42f, 0.42f, -0.84f, 0.84f, -1.26f};

// Pico's custom keycode, installed via the patched libinput + gpio-keys.kl
static const int kPicoConfirm = 1001;

// ---------------------------------------------------------------- font

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

struct TGlyph {
    float u0, v0, u1, v1;
    float xoff, yoff, w, h;
    float advance;
    bool valid = false;
};

struct Font {
    stbtt_fontinfo info;
    std::vector<uint8_t> data;
    bool ok = false;
    float scale = 0, ascent = 0;
    static const int PX = 36;
    static const int TEX = 1024;
    GLuint tex = 0;
    int packX = 0, packY = 0, packRowH = 0;
    int cp[512];
    TGlyph g[512];
    int n = 0;
};

static float propF(const char* key, float dflt) {
    char b[PROP_VALUE_MAX];
    if (__system_property_get(key, b) > 0) return (float)atof(b);
    return dflt;
}
static int propI(const char* key, int dflt) {
    char b[PROP_VALUE_MAX];
    if (__system_property_get(key, b) > 0) return atoi(b);
    return dflt;
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
    r.m[0] = f / aspect; r.m[5] = f;
    r.m[10] = (zf + zn) / (zn - zf); r.m[11] = -1.0f;
    r.m[14] = (2.0f * zf * zn) / (zn - zf);
    return r;
}
static Mat4 rotZ(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[0] = cosf(r); m.m[1] = sinf(r); m.m[4] = -sinf(r); m.m[5] = cosf(r);
    return m;
}
static Mat4 rotX(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[5] = cosf(r); m.m[6] = sinf(r); m.m[9] = -sinf(r); m.m[10] = cosf(r);
    return m;
}
// Quaternion (x,y,z,w) -> rotation matrix, column-major. inv=true transposes it:
// the sensor reports device->world and the view matrix wants the inverse.
static Mat4 quatToMat(const float* q, bool inv) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    Mat4 r = identity();
    float f[9] = {
        1-2*(y*y+z*z), 2*(x*y-z*w),   2*(x*z+y*w),
        2*(x*y+z*w),   1-2*(x*x+z*z), 2*(y*z-x*w),
        2*(x*z-y*w),   2*(y*z+x*w),   1-2*(x*x+y*y),
    };
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < 3; ++i)
            r.m[c*4+i] = inv ? f[i*3+c] : f[c*3+i];
    return r;
}

// world-space direction a unit view-space vector points after view matrix V
// (rotation part only, orthonormal, so transpose == inverse)
static void viewDirToWorld(const Mat4& v, const float in[3], float out[3]) {
    out[0] = v.m[0]*in[0] + v.m[1]*in[1] + v.m[2]*in[2];
    out[1] = v.m[4]*in[0] + v.m[5]*in[1] + v.m[6]*in[2];
    out[2] = v.m[8]*in[0] + v.m[9]*in[1] + v.m[10]*in[2];
}

// ---------------------------------------------------------------- panels

struct Panel {
    int displayId = -1;
    int taskId = -1;
    GLuint tex = 0;
    jobject st = nullptr;        // global ref to SurfaceTexture
    jfloatArray stArr = nullptr; // global ref, 16 floats
    float stMat[16];
    float yaw = 0;               // world yaw of panel centre
    std::string pkg;
};
static std::vector<Panel> gPanels;

// launch requests arrive from Java (LauncherActivity, test hook)
static std::deque<std::string> gLaunchQ;
static std::mutex gLaunchMu;

extern "C" JNIEXPORT void JNICALL
Java_org_pn2_vrhome_ShellBridge_nativeQueueLaunch(JNIEnv* env, jclass, jstring pkg) {
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    {
        std::lock_guard<std::mutex> l(gLaunchMu);
        gLaunchQ.push_back(p);
    }
    env->ReleaseStringUTFChars(pkg, p);
}

// HOME presses reach us as onNewIntent on PanelActivity - flag handled in
// the render loop where the gaze yaw is current
static volatile bool gWantRecenter = false;

extern "C" JNIEXPORT void JNICALL
Java_org_pn2_vrhome_PanelActivity_nativeHome(JNIEnv*, jclass) {
    gWantRecenter = true;
}

// ---------------------------------------------------------------- shaders

static const char* kSceneVS = R"(
attribute vec3 aPos;
attribute vec3 aCol;
uniform mat4 uMVP;
varying vec3 vCol;
void main() { vCol = aCol; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kSceneFS = R"(
precision mediump float;
varying vec3 vCol;
void main() { gl_FragColor = vec4(vCol, 1.0); }
)";

static const char* kWarpVS = R"(
attribute vec2 aPos;
varying vec2 vUV;
void main() { vUV = aPos * 0.5 + 0.5; gl_Position = vec4(aPos, 0.0, 1.0); }
)";

static const char* kFloatVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

// app panel: samples an external OES texture fed by the virtual display
static const char* kFloatFS = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2 vUV;
uniform samplerExternalOES uTex;
uniform mat4 uST;
void main() {
    vec2 uv = (uST * vec4(vUV, 0.0, 1.0)).xy;
    gl_FragColor = texture2D(uTex, uv);
}
)";

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
    gl_FragColor = (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        ? vec4(0.0, 0.0, 0.0, 1.0) : texture2D(uTex, uv);
}
)";

static const char* kTextVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kTextFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uFont;
uniform vec3 uColor;
void main() {
    float a = texture2D(uFont, vUV).a;
    if (a < 0.05) discard;
    gl_FragColor = vec4(uColor, a);
}
)";

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(s, sizeof(l), nullptr, l);
        LOGE("shader: %s", l); glDeleteShader(s); return 0; }
    return s;
}
static GLuint link(const char* vs, const char* fs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char l[512]; glGetProgramInfoLog(p, sizeof(l), nullptr, l);
        LOGE("link: %s", l); glDeleteProgram(p); return 0; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// ---------------------------------------------------------------- engine

struct Eye { GLuint fbo = 0, tex = 0, depth = 0; int w = 0, h = 0; };

struct Engine {
    android_app* app = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int width = 0, height = 0;
    bool ready = false;

    GLuint sceneProg = 0, warpProg = 0, textProg = 0, floatProg = 0;
    GLuint quadVbo = 0, gridVbo = 0, textVbo = 0, panelVbo = 0;
    Font font;
    Eye eye[2];

    // ShellBridge java object + cached method ids
    jobject bridge = nullptr;
    jmethodID mCreatePanel = nullptr, mPanelTex = nullptr, mLaunchPkg = nullptr,
              mLaunchLauncher = nullptr, mAdopt = nullptr, mReleasePanel = nullptr,
              mTakeAdopt = nullptr, mTakeRelease = nullptr, mInjectTap = nullptr,
              mRemoveTask = nullptr, mFocusTask = nullptr;
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

static bool loadFont(Engine* e);

// floor grid: gives the eye something to lock onto so a "black" scene is never
// just empty space
static const int   kGridHalf = 10;
static const float kGridStep = 0.5f;
static float       gGrid[(kGridHalf * 2 + 1) * 4 * 6];
static int         gGridVerts = 0;

static void buildGrid() {
    int n = 0;
    const float e = kGridHalf * kGridStep;
    for (int i = -kGridHalf; i <= kGridHalf; ++i) {
        const float t = i * kGridStep;
        const bool axis = (i == 0);
        const float r = axis ? 0.9f : 0.16f, g = axis ? 0.9f : 0.22f, b = axis ? 0.9f : 0.32f;
        const float pts[4][3] = {{t, -1.2f, -e}, {t, -1.2f, e}, {-e, -1.2f, t}, {e, -1.2f, t}};
        for (int k = 0; k < 4; ++k) {
            gGrid[n++] = pts[k][0]; gGrid[n++] = pts[k][1]; gGrid[n++] = pts[k][2];
            gGrid[n++] = r;         gGrid[n++] = g;         gGrid[n++] = b;
        }
    }
    gGridVerts = n / 6;
}

// ---------------------------------------------------------- bridge calls

static JNIEnv* threadEnv(android_app* app) {
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    return env;
}

// FindClass on a natively-attached thread only sees the boot classpath; app
// classes must go through the activity's ClassLoader
static jclass loadAppClass(JNIEnv* env, jobject activity, const char* name) {
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

static void initBridge(Engine* e) {
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

    jclass stc = env->FindClass("android/graphics/SurfaceTexture");
    e->stUpdate = env->GetMethodID(stc, "updateTexImage", "()V");
    e->stMatrix = env->GetMethodID(stc, "getTransformMatrix", "([F)V");

    e->pendingCls = (jclass)env->NewGlobalRef(loadAppClass(env,
        e->app->activity->clazz, "org.pn2.vrhome.ShellBridge$Pending"));
    e->fPendTask = env->GetFieldID(e->pendingCls, "taskId", "I");
    e->fPendPkg  = env->GetFieldID(e->pendingCls, "pkg", "Ljava/lang/String;");
    LOGI("bridge ready");
}

// create a GL texture + virtual display + panel record. taskId stays -1 for
// launcher/explicit launches. Returns gPanels index or -1.
static int openPanel(Engine* e, float yaw) {
    if (!e->bridge || (int)gPanels.size() >= kMaxPanels) return -1;
    JNIEnv* env = threadEnv(e->app);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int dispId = env->CallIntMethod(e->bridge, e->mCreatePanel,
                                    (jint)tex, kVdW, kVdH, kVdDpi);
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    if (dispId < 0) { glDeleteTextures(1, &tex); return -1; }

    jobject st = env->CallObjectMethod(e->bridge, e->mPanelTex, dispId);
    if (!st) { glDeleteTextures(1, &tex);
        env->CallVoidMethod(e->bridge, e->mReleasePanel, dispId); return -1; }

    Panel p;
    p.displayId = dispId;
    p.tex = tex;
    p.st = env->NewGlobalRef(st);
    p.stArr = (jfloatArray)env->NewGlobalRef(env->NewFloatArray(16));
    memset(p.stMat, 0, sizeof(p.stMat));
    p.yaw = yaw;
    gPanels.push_back(p);
    LOGI("panel %d on display %d yaw %.2f", (int)gPanels.size() - 1, dispId, yaw);
    return (int)gPanels.size() - 1;
}

static void closePanel(Engine* e, int idx) {
    Panel& p = gPanels[idx];
    JNIEnv* env = threadEnv(e->app);
    if (e->bridge && p.displayId >= 0)
        env->CallVoidMethod(e->bridge, e->mReleasePanel, p.displayId);
    if (p.st) env->DeleteGlobalRef(p.st);
    if (p.stArr) env->DeleteGlobalRef(p.stArr);
    if (p.tex) glDeleteTextures(1, &p.tex);
    gPanels.erase(gPanels.begin() + idx);
    if (e->hover == idx) e->hover = -1;
    else if (e->hover > idx) e->hover--;
}

// next free yaw slot around a centre yaw
static float freeSlotYaw(float centre) {
    bool used[kMaxPanels] = {};
    for (auto& p : gPanels) {
        for (int s = 0; s < kMaxPanels; ++s) {
            float d = p.yaw - (centre + kSlotYaw[s]);
            while (d > (float)M_PI) d -= 2.0f * (float)M_PI;
            while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
            if (fabsf(d) < 0.05f) used[s] = true;
        }
    }
    for (int s = 0; s < kMaxPanels; ++s)
        if (!used[s]) return centre + kSlotYaw[s];
    return centre;
}

// drain everything the bridge has queued; run on the render thread
static void pumpBridge(Engine* e) {
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
        int idx = openPanel(e, freeSlotYaw(e->gazeYaw));
        if (idx < 0) continue;
        gPanels[idx].pkg = pkg;
        jstring jpkg = env->NewStringUTF(pkg.c_str());
        env->CallVoidMethod(e->bridge, e->mLaunchPkg, jpkg,
                            gPanels[idx].displayId);
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
        int idx = openPanel(e, freeSlotYaw(e->gazeYaw));
        if (idx >= 0) {
            gPanels[idx].taskId = taskId;
            if (jpkg) {
                const char* c = env->GetStringUTFChars(jpkg, nullptr);
                gPanels[idx].pkg = c;
                env->ReleaseStringUTFChars(jpkg, c);
            }
            env->CallVoidMethod(e->bridge, e->mAdopt, taskId,
                                gPanels[idx].displayId);
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
        for (int i = 0; i < (int)gPanels.size(); ++i)
            if (gPanels[i].displayId == id) { closePanel(e, i); break; }
    }
}

// pull the newest frame of each virtual display into its texture
static void updatePanels(Engine* e) {
    if (gPanels.empty()) return;
    JNIEnv* env = threadEnv(e->app);
    for (auto& p : gPanels) {
        if (!p.st) continue;
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, p.tex);
        env->CallVoidMethod(p.st, e->stUpdate);
        env->CallVoidMethod(p.st, e->stMatrix, p.stArr);
        env->GetFloatArrayRegion(p.stArr, 0, 16, p.stMat);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
}

// panel quad in world space, facing the viewer at the origin
static void panelCenter(const Panel& p, float out[3], float right[3]) {
    out[0] = sinf(p.yaw) * kPanelDist;
    out[1] = kPanelY;
    out[2] = -cosf(p.yaw) * kPanelDist;
    // fwd = (-sin,0,cos) toward origin; right = cross(up, fwd) so the panel's
    // right edge lands on the viewer's right
    right[0] = cosf(p.yaw); right[1] = 0; right[2] = sinf(p.yaw);
}

static void drawPanels(Engine* e, const Mat4& viewProj) {
    if (gPanels.empty()) return;
    glUseProgram(e->floatProg);
    glUniform1i(glGetUniformLocation(e->floatProg, "uTex"), 0);
    const GLint uMVP = glGetUniformLocation(e->floatProg, "uMVP");
    const GLint uST  = glGetUniformLocation(e->floatProg, "uST");
    const GLint aPos = glGetAttribLocation(e->floatProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->floatProg, "aUV");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float hw = kPanelW / 2, hh = kPanelH / 2;
    for (auto& p : gPanels) {
        float c[3], r[3];
        panelCenter(p, c, r);
        // vv: 0 bottom, 1 top; uu: 0 left, 1 right
        const float q[4][5] = {
            {c[0]-r[0]*hw, c[1]-hh, c[2]-r[2]*hw, 0.0f, 0.0f},
            {c[0]+r[0]*hw, c[1]-hh, c[2]+r[2]*hw, 1.0f, 0.0f},
            {c[0]+r[0]*hw, c[1]+hh, c[2]+r[2]*hw, 1.0f, 1.0f},
            {c[0]-r[0]*hw, c[1]+hh, c[2]-r[2]*hw, 0.0f, 1.0f},
        };
        const int tris[6] = {0,1,2, 0,2,3};
        float verts[30];
        for (int t = 0; t < 6; ++t) memcpy(verts + t*5, q[tris[t]], 20);
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
        glUniformMatrix4fv(uST, 1, GL_FALSE, p.stMat);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, p.tex);
        glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
        glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
        glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
}

// gaze ray vs panels; stores hovered panel + hit point in display px
static void pickPanel(Engine* e, const Mat4& head) {
    float d[3];
    const float fwd[3] = {0, 0, -1};
    viewDirToWorld(head, fwd, d);
    e->hover = -1;
    float bestT = 1e9f;
    for (int i = 0; i < (int)gPanels.size(); ++i) {
        Panel& p = gPanels[i];
        float c[3], r[3];
        panelCenter(p, c, r);
        // plane normal toward origin
        float n[3] = {-c[0], 0, -c[2]};
        const float nl = sqrtf(n[0]*n[0] + n[2]*n[2]);
        n[0] /= nl; n[2] /= nl;
        // normal points at the viewer, ray travels into the plane: d.n < 0
        const float dn = d[0]*n[0] + d[2]*n[2];
        if (dn > -1e-5f) continue;
        const float t = (c[0]*n[0] + c[1]*n[1] + c[2]*n[2]) / dn;
        if (t <= 0 || t >= bestT) continue;
        const float px = d[0]*t - c[0], py = d[1]*t - c[1], pz = d[2]*t - c[2];
        const float u = (px*r[0] + pz*r[2]) / (kPanelW / 2);
        const float v = py / (kPanelH / 2);
        if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) continue;
        bestT = t;
        e->hover = i;
        e->hitX = (u * 0.5f + 0.5f) * kVdW;
        e->hitY = (0.5f - v * 0.5f) * kVdH;
    }
    if (d[0]*d[0] + d[2]*d[2] > 1e-6f)
        e->gazeYaw = atan2f(d[0], -d[2]);
}

// cursor dot on the hovered panel, just in front of its surface
static void drawCursor(Engine* e, const Mat4& viewProj) {
    if (e->hover < 0 || e->hover >= (int)gPanels.size()) return;
    const Panel& p = gPanels[e->hover];
    float c[3], r[3];
    panelCenter(p, c, r);
    const float u = e->hitX / kVdW * 2.0f - 1.0f;
    const float v = 1.0f - e->hitY / kVdH * 2.0f;
    const float hw = kPanelW / 2, hh = kPanelH / 2;
    // toward the viewer a touch so it never z-fights the panel
    float pos[3] = {c[0] + r[0]*u*hw - c[0]*0.01f,
                    c[1] + v*hh,
                    c[2] + r[2]*u*hw - c[2]*0.01f};
    const float s = 0.012f;
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glDisable(GL_DEPTH_TEST);
    // billboard in the panel plane: right x up offsets
    const float verts[6][6] = {
        {pos[0]-r[0]*s, pos[1]-s, pos[2]-r[2]*s, 1,1,1},
        {pos[0]+r[0]*s, pos[1]-s, pos[2]+r[2]*s, 1,1,1},
        {pos[0]+r[0]*s, pos[1]+s, pos[2]+r[2]*s, 1,1,1},
        {pos[0]-r[0]*s, pos[1]-s, pos[2]-r[2]*s, 1,1,1},
        {pos[0]+r[0]*s, pos[1]+s, pos[2]+r[2]*s, 1,1,1},
        {pos[0]-r[0]*s, pos[1]+s, pos[2]-r[2]*s, 1,1,1},
    };
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
}

static bool initEyeTargets(Engine* e) {
    const int ew = e->width / 2, eh = e->height;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        if (y.fbo) { glDeleteFramebuffers(1, &y.fbo); glDeleteTextures(1, &y.tex);
                     glDeleteRenderbuffers(1, &y.depth); }
        y.w = ew; y.h = eh;
        glGenTextures(1, &y.tex); glBindTexture(GL_TEXTURE_2D, y.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ew, eh, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenRenderbuffers(1, &y.depth); glBindRenderbuffer(GL_RENDERBUFFER, y.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, ew, eh);
        glGenFramebuffers(1, &y.fbo); glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, y.tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, y.depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("eye %d FBO incomplete", i); return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

static int initDisplay(Engine* e) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16,
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
    if (eglMakeCurrent(dpy, surf, surf, ctx) == EGL_FALSE) {
        LOGE("eglMakeCurrent failed"); return -1;
    }
    eglQuerySurface(dpy, surf, EGL_WIDTH, &e->width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &e->height);
    e->display = dpy; e->surface = surf; e->context = ctx;
    LOGI("surface %dx%d  %s", e->width, e->height, glGetString(GL_RENDERER));

    e->sceneProg = link(kSceneVS, kSceneFS);
    e->warpProg  = link(kWarpVS,  kWarpFS);
    e->textProg  = link(kTextVS,  kTextFS);
    e->floatProg = link(kFloatVS, kFloatFS);
    if (!e->sceneProg || !e->warpProg || !e->textProg || !e->floatProg)
        return -1;

    if (!loadFont(e)) LOGE("font load failed, HUD text disabled");
    glGenBuffers(1, &e->textVbo);
    glGenBuffers(1, &e->panelVbo);
    glGenBuffers(1, &e->quadVbo);
    glGenBuffers(1, &e->gridVbo);
    const float quad[] = {-1,-1, 1,-1, -1,1,  1,-1, 1,1, -1,1};
    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    buildGrid();
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glBufferData(GL_ARRAY_BUFFER, gGridVerts * 6 * sizeof(float), gGrid, GL_STATIC_DRAW);

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
    e->display = EGL_NO_DISPLAY; e->context = EGL_NO_CONTEXT;
    e->surface = EGL_NO_SURFACE; e->ready = false;
}

// drain sensor + input queues - MUST run inside the looper poll, else the
// pending events keep the looper non-idle and drawFrame never runs
static void drainSensor(Engine* e) {
    if (!e->sensorQueue) return;
    ASensorEvent ev;
    while (ASensorEventQueue_getEvents(e->sensorQueue, &ev, 1) > 0) {
        if (ev.type == ASENSOR_TYPE_GAME_ROTATION_VECTOR ||
            ev.type == ASENSOR_TYPE_ROTATION_VECTOR) {
            e->quat[0] = ev.data[0]; e->quat[1] = ev.data[1]; e->quat[2] = ev.data[2];
            const float sq = ev.data[0]*ev.data[0] + ev.data[1]*ev.data[1] +
                             ev.data[2]*ev.data[2];
            e->quat[3] = (ev.data[3] != 0.0f) ? ev.data[3]
                : (sq < 1.0f ? sqrtf(1.0f - sq) : 0.0f);
            e->haveQuat = true;
            ++e->sensorEv;
            if (fabsf(ev.data[0] - e->lastQ[0]) > 1e-5f ||
                fabsf(ev.data[1] - e->lastQ[1]) > 1e-5f ||
                fabsf(ev.data[2] - e->lastQ[2]) > 1e-5f ||
                fabsf(ev.data[3] - e->lastQ[3]) > 1e-5f) {
                ++e->sensorNew;
                e->lastQ[0]=ev.data[0]; e->lastQ[1]=ev.data[1];
                e->lastQ[2]=ev.data[2]; e->lastQ[3]=ev.data[3];
            }
        }
    }
}

static bool isConfirm(int32_t code) {
    return code == AKEYCODE_ENTER || code == AKEYCODE_DPAD_CENTER ||
           code == AKEYCODE_BUTTON_A || code == kPicoConfirm;
}

static void recenter(Engine* e);

// the glue drains the input queue itself and calls this per event - polling the
// queue manually finds it already empty, so input MUST be handled here
static int32_t onInputEvent(android_app* app, AInputEvent* ev) {
    Engine* e = (Engine*)app->userData;
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_KEY)
        return 0;
    const int32_t code = AKeyEvent_getKeyCode(ev);
    const int32_t action = AKeyEvent_getAction(ev);

    if (isConfirm(code)) {
        if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(ev) == 0)
            LOGI("confirm down, hover %d", e->hover);
        if (action == AKEY_EVENT_ACTION_UP && e->confirmHeld) {
            e->confirmHeld = false;
            if (e->bridge && e->hover >= 0 && e->hover < (int)gPanels.size()) {
                JNIEnv* env = threadEnv(app);
                const Panel& p = gPanels[e->hover];
                LOGI("tap disp %d @ %.0f,%.0f", p.displayId, e->hitX, e->hitY);
                env->CallVoidMethod(e->bridge, e->mInjectTap,
                                    p.displayId, (float)e->hitX, (float)e->hitY);
                if (env->ExceptionCheck()) env->ExceptionClear();
                if (p.taskId >= 0) {
                    env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                    if (env->ExceptionCheck()) env->ExceptionClear();
                }
            }
        } else if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(ev) == 0) {
            e->confirmHeld = true;
        }
        return 1;
    }
    if (code == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP) {
        // display-0 focus: close the newest panel; when a panel app has
        // focus the key never reaches us - the app handles it natively
        if (e->bridge && !gPanels.empty()) {
            JNIEnv* env = threadEnv(app);
            Panel& p = gPanels.back();
            if (p.taskId >= 0) {
                env->CallVoidMethod(e->bridge, e->mRemoveTask, p.taskId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            closePanel(e, (int)gPanels.size() - 1);
        }
        return 1;
    }
    if (code == AKEYCODE_HOME && action == AKEY_EVENT_ACTION_UP) {
        // recenter: the ring's slot layout recentres on the current gaze yaw
        recenter(e);
        return 1;
    }
    return 0;
}

static void drawScene(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);

    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glDrawArrays(GL_LINES, 0, gGridVerts);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);

    drawPanels(e, viewProj);
    drawCursor(e, viewProj);
}

// ---------------------------------------------------------------- text

static bool loadFont(Engine* e) {
    static const char* paths[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/DroidSans.ttf",
        "/system/fonts/Roboto-Regular.ttf",
    };
    FILE* f = nullptr;
    for (const char* p : paths) { f = fopen(p, "rb"); if (f) break; }
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    e->font.data.resize(sz);
    fread(e->font.data.data(), 1, sz, f);
    fclose(f);
    const int off = stbtt_GetFontOffsetForIndex(e->font.data.data(), 0);
    if (!stbtt_InitFont(&e->font.info, e->font.data.data(), off)) return false;
    e->font.scale = stbtt_ScaleForPixelHeight(&e->font.info, Font::PX);
    int a, d, lg;
    stbtt_GetFontVMetrics(&e->font.info, &a, &d, &lg);
    e->font.ascent = a * e->font.scale;

    glGenTextures(1, &e->font.tex);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    std::vector<uint8_t> zero(Font::TEX * Font::TEX, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Font::TEX, Font::TEX, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    e->font.ok = true;
    return true;
}

static int nextCp(const char*& p) {
    const unsigned char c = (unsigned char)*p++;
    if (c < 0x80) return c;
    int r;
    if      ((c & 0xF8) == 0xF0) r = c & 0x07;
    else if ((c & 0xF0) == 0xE0) r = c & 0x0F;
    else if ((c & 0xE0) == 0xC0) r = c & 0x1F;
    else return c;
    while (*p && (*p & 0xC0) == 0x80) r = (r << 6) | (*p++ & 0x3F);
    return r;
}

static const TGlyph* fontGlyph(Engine* e, int cp) {
    Font& f = e->font;
    for (int i = 0; i < f.n; ++i)
        if (f.cp[i] == cp) return &f.g[i];
    if (f.n >= 512) return nullptr;

    int adv, lsb;
    stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&f.info, cp, f.scale, f.scale, &x0, &y0, &x1, &y1);
    int gw = x1 - x0, gh = y1 - y0;

    TGlyph g{};
    g.xoff = (float)x0; g.yoff = (float)y0;
    g.w = (float)gw; g.h = (float)gh;
    g.advance = adv * f.scale;
    g.valid = true;

    if (gw > 0 && gh > 0) {
        if (f.packX + gw + 3 > Font::TEX) {
            f.packX = 0; f.packY += f.packRowH + 2; f.packRowH = 0;
        }
        if (f.packY + gh + 2 <= Font::TEX) {
            std::vector<uint8_t> bmp(gw * gh);
            stbtt_MakeCodepointBitmap(&f.info, bmp.data(), gw, gh, gw,
                                      f.scale, f.scale, cp);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glBindTexture(GL_TEXTURE_2D, f.tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, f.packX, f.packY, gw, gh,
                            GL_ALPHA, GL_UNSIGNED_BYTE, bmp.data());
            g.u0 = f.packX / (float)Font::TEX; g.u1 = (f.packX + gw) / (float)Font::TEX;
            g.v0 = f.packY / (float)Font::TEX; g.v1 = (f.packY + gh) / (float)Font::TEX;
            f.packX += gw + 2;
            if (gh > f.packRowH) f.packRowH = gh;
        }
    }
    f.cp[f.n] = cp;
    f.g[f.n] = g;
    ++f.n;
    return &f.g[f.n - 1];
}

static float textWidth(Engine* e, const char* utf8, float mPerPx) {
    const char* p = utf8;
    float w = 0;
    while (*p) {
        const TGlyph* g = fontGlyph(e, nextCp(p));
        if (g) w += g->advance * mPerPx;
    }
    return w;
}

static float drawText(Engine* e, const char* utf8, float x, float y, float z,
                      float mPerPx) {
    if (!e->font.ok) return 0;
    std::vector<float> v;
    const char* p = utf8;
    float pen = x;
    while (*p) {
        const TGlyph* g = fontGlyph(e, nextCp(p));
        if (!g) continue;
        if (g->w > 0 && g->h > 0) {
            const float gx = pen + g->xoff * mPerPx;
            const float gtop = y - g->yoff * mPerPx;
            const float gbot = gtop - g->h * mPerPx;
            const float gw = g->w * mPerPx;
            const float quad[6][5] = {
                {gx,    gbot, z, g->u0, g->v1},
                {gx+gw, gbot, z, g->u1, g->v1},
                {gx+gw, gtop, z, g->u1, g->v0},
                {gx,    gbot, z, g->u0, g->v1},
                {gx+gw, gtop, z, g->u1, g->v0},
                {gx,    gtop, z, g->u0, g->v0},
            };
            for (auto& q : quad)
                for (int k = 0; k < 5; ++k) v.push_back(q[k]);
        }
        pen += g->advance * mPerPx;
    }
    if (v.empty()) return 0;
    const GLint aPos = glGetAttribLocation(e->textProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->textProg, "aUV");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glBindBuffer(GL_ARRAY_BUFFER, e->textVbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(v.size() / 5));
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
    return pen - x;
}

// HUD: head-locked status line so the pipeline can be verified without adb
static void drawHud(Engine* e, const Mat4& proj) {
    if (!e->font.ok) return;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"), 1, GL_FALSE,
                     proj.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"), 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float s = 0.0026f;
    const float z = -1.2f;
    float w = textWidth(e, e->hud, s);
    drawText(e, e->hud, -w * 0.5f, 0.30f, z, s);
    if (e->hover >= 0 && e->hover < (int)gPanels.size() &&
            !gPanels[e->hover].pkg.empty()) {
        const char* lbl = gPanels[e->hover].pkg.c_str();
        w = textWidth(e, lbl, s);
        drawText(e, lbl, -w * 0.5f, 0.18f, z, s);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

static void recenter(Engine* e) {
    for (auto& p : gPanels) {
        // keep the panel's slot offset, re-centre the ring on current gaze
        float off = p.yaw - e->gazeYaw;
        while (off > (float)M_PI)  off -= 2.0f * (float)M_PI;
        while (off < -(float)M_PI) off += 2.0f * (float)M_PI;
        // find nearest slot offset and snap to it around the new centre
        float best = 1e9f; int bs = 0;
        for (int s = 0; s < kMaxPanels; ++s) {
            float d = fabsf(off - kSlotYaw[s]);
            if (d < best) { best = d; bs = s; }
        }
        p.yaw = e->gazeYaw + kSlotYaw[bs];
    }
}

static void drawFrame(Engine* e) {
    if (!e->ready) return;

    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    const bool transpose = propI("debug.vrhome.tq", 1) != 0;
    const float dRoll   = propF("debug.vrhome.roll",     kRoll);
    const float sensroll= propF("debug.vrhome.sensroll", kSensRoll);
    const float worldx  = propF("debug.vrhome.worldx",   kWorldX);
    Mat4 head = useSensor ? quatToMat(e->quat, transpose) : identity();
    if (useSensor) {
        head = multiply(head, rotZ(sensroll));
        head = multiply(head, rotX(worldx));
    }
    head = multiply(rotZ(dRoll), head);

    // test hook: setprop debug.vrhome.launch <pkg> queues a panel launch
    {
        static bool testFired = false;
        char tb[PROP_VALUE_MAX];
        if (!testFired && __system_property_get("debug.vrhome.launch", tb) > 0
                && e->frames > 30) {
            testFired = true;
            std::lock_guard<std::mutex> l(gLaunchMu);
            gLaunchQ.push_back(tb);
        }
    }
    // test hook: setprop debug.vrhome.tap "disp,x,y" injects a tap there.
    // Fires once per new value; the prop may not be clearable from this uid.
    {
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
    // still stale on the first quat frame (pickPanel runs below), so take the
    // yaw straight from the head matrix.
    if (e->bridge && e->haveQuat && !e->launcherSpawned) {
        e->launcherSpawned = true;
        float d[3];
        const float fwd[3] = {0, 0, -1};
        viewDirToWorld(head, fwd, d);
        int idx = openPanel(e, atan2f(d[0], -d[2]));
        if (idx >= 0) {
            gPanels[idx].pkg = "org.pn2.vrhome.library";
            JNIEnv* env = threadEnv(e->app);
            env->CallVoidMethod(e->bridge, e->mLaunchLauncher,
                                gPanels[idx].displayId);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }

    if (gWantRecenter) {
        gWantRecenter = false;
        recenter(e);
    }
    pumpBridge(e);
    pickPanel(e, head);
    updatePanels(e);

    const float qx = e->quat[0], qy = e->quat[1], qz = e->quat[2], qw = e->quat[3];
    const float yaw   = atan2f(2*(qw*qy + qx*qz), 1 - 2*(qy*qy + qx*qx)) * 180.0f / (float)M_PI;
    const float pitch = asinf(fmaxf(-1.0f, fminf(1.0f, 2*(qw*qx - qy*qz)))) * 180.0f / (float)M_PI;
    const float roll  = atan2f(2*(qw*qz + qx*qy), 1 - 2*(qz*qz + qx*qx)) * 180.0f / (float)M_PI;
    e->hudLen = snprintf(e->hud, sizeof(e->hud),
        "YAW %+4.0f PIT %+4.0f ROL %+4.0f  FPS %d  SEN %d  PNL %zu%s",
        yaw, pitch, roll, e->fps, e->sensorNewHz, gPanels.size(),
        e->bridge ? "" : "  BRIDGE:OFF");

    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const Mat4 proj = perspective(kFovY, aspect, 0.05f, 100.0f);

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
        Mat4 eyeView = head;
        const float off = (i == 0 ? -kIPD / 2 : kIPD / 2);
        Mat4 shift = identity(); shift.m[12] = off;
        eyeView = multiply(eyeView, shift);
        const Mat4 vp = multiply(proj, eyeView);
        drawScene(e, vp);
        if (propI("debug.vrhome.hud", 1)) drawHud(e, proj);
        if (++errTick >= 144) {
            errTick = 0;
            GLenum ge = glGetError();
            if (ge != GL_NO_ERROR) LOGE("GL error 0x%x", ge);
        }
    }

    // warp pass: each eye texture through barrel distortion to its half
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

// ---------------------------------------------------------------- lifecycle

static void onAppCmd(android_app* app, int32_t cmd) {
    Engine* e = (Engine*)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window) initDisplay(e);
        break;
    case APP_CMD_TERM_WINDOW:
        termDisplay(e);
        break;
    // keep sensors running regardless of focus: a focused panel app must
    // not freeze head tracking of the shell that renders it
    }
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

    {
        JNIEnv* env = threadEnv(app);
        jobject activity = app->activity->clazz;
        jclass actCls = env->GetObjectClass(activity);
        jmethodID getWindow = env->GetMethodID(actCls, "getWindow", "()Landroid/view/Window;");
        jobject win = env->CallObjectMethod(activity, getWindow);
        jclass winCls = env->GetObjectClass(win);
        jmethodID getDecor = env->GetMethodID(winCls, "getDecorView", "()Landroid/view/View;");
        jobject decor = env->CallObjectMethod(win, getDecor);
        jclass viewCls = env->FindClass("android/view/View");
        jmethodID setVis = env->GetMethodID(viewCls, "setSystemUiVisibility", "(I)V");
        // IMMERSIVE_STICKY|HIDE_NAVIGATION|FULLSCREEN|LAYOUT_STABLE|LAYOUT_HIDE_NAVIGATION|LAYOUT_FULLSCREEN
        env->CallVoidMethod(decor, setVis, 0x1000 | 0x0002 | 0x0004 | 0x0100 | 0x0200 | 0x0400);
    }

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
