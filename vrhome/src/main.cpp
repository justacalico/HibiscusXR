// Proof-of-concept VR home for the Pico Neo 2 running a plain Android 10 GSI.
//
// Renders a wall of launchable apps in stereo on the open EGL path - no Pico
// stack, no closed compositor. 3DoF comes from the IMU through ASensorManager.
// Gaze dwells a panel; the headset Confirm/Back button launches it via JNI.
//
// This is a PoC, not a product: panels are colour-keyed rectangles and app
// identity goes to logcat. Icons, text, 2D-apps-as-panels and controllers are
// later work.

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

// tunables confirmed on the headset in the vrdemo: worldx 90
static float kDistK1 = 0.22f, kDistK2 = 0.24f;
static float kIPD  = 0.063f;
static float kFovY = 90.0f;
// roll confirmed on the headset: 90 left the world upside down through the
// lenses, 270 puts it upright. Still live-tunable via debug.vrhome.roll.
static float kRoll = 270.0f, kSensRoll = 0.0f, kWorldX = 90.0f;

static const int kSensorIdent = 3;
static const int kInputIdent  = 4;

// ---------------------------------------------------------------- font

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

// TrueType text via stb_truetype over the device's Noto CJK font, so any
// language works. Glyphs are rasterised on demand into a shared atlas.
struct TGlyph {
    float u0, v0, u1, v1;      // atlas uv, v0 = glyph top (low memory v)
    float xoff, yoff, w, h;    // px: bearing from pen + bitmap size
    float advance;
    bool valid = false;
};

struct Font {
    stbtt_fontinfo info;
    std::vector<uint8_t> data;
    bool ok = false;
    float scale = 0, ascent = 0;
    static const int PX = 36;    // rasterise height
    static const int TEX = 1024; // atlas edge
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

// Pico's custom keycode, installed via the patched libinput + gpio-keys.kl
static const int kPicoConfirm = 1001;

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
static Mat4 translate3(float x, float y, float z) {
    Mat4 r = identity(); r.m[12] = x; r.m[13] = y; r.m[14] = z; return r;
}

// world-space direction a unit view-space vector points after view matrix V
// (rotation part only, orthonormal, so transpose == inverse)
static void viewDirToWorld(const Mat4& v, const float in[3], float out[3]) {
    out[0] = v.m[0]*in[0] + v.m[1]*in[1] + v.m[2]*in[2];
    out[1] = v.m[4]*in[0] + v.m[5]*in[1] + v.m[6]*in[2];
    out[2] = v.m[8]*in[0] + v.m[9]*in[1] + v.m[10]*in[2];
}

// ---------------------------------------------------------------- app list

struct AppEntry {
    std::string pkg, label;
    std::vector<uint8_t> icon;   // rgba
    int iconW = 0, iconH = 0;
};
static std::vector<AppEntry> gApps;

// activity->env is only valid on the creating thread; android_main runs on its
// own thread, so pull the env for THIS thread out of the VM (no-op if attached)
static JNIEnv* threadEnv(android_app* app) {
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    return env;
}

static void refreshApps(android_app* app) {
    JNIEnv* env = threadEnv(app);
    jobject activity = app->activity->clazz;
    jclass actCls = env->GetObjectClass(activity);

    jmethodID getPM = env->GetMethodID(actCls, "getPackageManager",
        "()Landroid/content/pm/PackageManager;");
    jobject pm = env->CallObjectMethod(activity, getPM);

    jclass intentCls = env->FindClass("android/content/Intent");
    jmethodID ctor = env->GetMethodID(intentCls, "<init>", "(Ljava/lang/String;)V");
    jmethodID addCat = env->GetMethodID(intentCls, "addCategory",
        "(Ljava/lang/String;)Landroid/content/Intent;");
    jobject intent = env->NewObject(intentCls, ctor,
        env->NewStringUTF("android.intent.action.MAIN"));
    env->CallObjectMethod(intent, addCat,
        env->NewStringUTF("android.intent.category.LAUNCHER"));

    jclass pmCls = env->GetObjectClass(pm);
    jmethodID query = env->GetMethodID(pmCls, "queryIntentActivities",
        "(Landroid/content/Intent;I)Ljava/util/List;");
    jobject list = env->CallObjectMethod(pm, query, intent, 0);

    jclass listCls = env->GetObjectClass(list);
    jmethodID size = env->GetMethodID(listCls, "size", "()I");
    jmethodID get  = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    jint n = env->CallIntMethod(list, size);

    jclass riCls = env->FindClass("android/content/pm/ResolveInfo");
    jfieldID aiField = env->GetFieldID(riCls, "activityInfo",
        "Landroid/content/pm/ActivityInfo;");
    jclass aiCls = env->FindClass("android/content/pm/ActivityInfo");
    jfieldID pkgField = env->GetFieldID(aiCls, "packageName", "Ljava/lang/String;");
    jmethodID loadLabel = env->GetMethodID(riCls, "loadLabel",
        "(Landroid/content/pm/PackageManager;)Ljava/lang/CharSequence;");
    jmethodID loadIcon = env->GetMethodID(riCls, "loadIcon",
        "(Landroid/content/pm/PackageManager;)Landroid/graphics/drawable/Drawable;");

    jclass csCls = env->FindClass("java/lang/CharSequence");
    jmethodID toStr = env->GetMethodID(csCls, "toString", "()Ljava/lang/String;");

    gApps.clear();
    for (jint i = 0; i < n; ++i) {
        jobject ri = env->CallObjectMethod(list, get, i);
        jobject ai = env->GetObjectField(ri, aiField);
        jstring pkg = (jstring)env->GetObjectField(ai, pkgField);
        const char* p = env->GetStringUTFChars(pkg, nullptr);
        if (!strcmp(p, "org.pn2.vrhome")) {
            env->ReleaseStringUTFChars(pkg, p); continue;
        }
        AppEntry e; e.pkg = p;
        env->ReleaseStringUTFChars(pkg, p);

        jobject lbl = env->CallObjectMethod(ri, loadLabel, pm);
        if (lbl) {
            jstring s = (jstring)env->CallObjectMethod(lbl, toStr);
            const char* c = env->GetStringUTFChars(s, nullptr);
            e.label = c;
            env->ReleaseStringUTFChars(s, c);
        } else e.label = e.pkg;

        // launcher icon: Drawable -> Bitmap -> pixels
        jobject icon = env->CallObjectMethod(ri, loadIcon, pm);
        if (icon) {
            const int S = 96;
            jclass cfgCls = env->FindClass("android/graphics/Bitmap$Config");
            jfieldID argb = env->GetStaticFieldID(cfgCls, "ARGB_8888",
                "Landroid/graphics/Bitmap$Config;");
            jobject cfg = env->GetStaticObjectField(cfgCls, argb);
            jclass bmpCls = env->FindClass("android/graphics/Bitmap");
            jmethodID create = env->GetStaticMethodID(bmpCls, "createBitmap",
                "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
            jobject bmp = env->CallStaticObjectMethod(bmpCls, create, S, S, cfg);
            jclass cvCls = env->FindClass("android/graphics/Canvas");
            jobject canvas = env->NewObject(cvCls,
                env->GetMethodID(cvCls, "<init>", "(Landroid/graphics/Bitmap;)V"), bmp);
            jclass drCls = env->FindClass("android/graphics/drawable/Drawable");
            env->CallVoidMethod(icon,
                env->GetMethodID(drCls, "setBounds", "(IIII)V"), 0, 0, S, S);
            env->CallVoidMethod(icon,
                env->GetMethodID(drCls, "draw", "(Landroid/graphics/Canvas;)V"), canvas);
            jclass bbCls = env->FindClass("java/nio/ByteBuffer");
            jobject buf = env->CallStaticObjectMethod(bbCls,
                env->GetStaticMethodID(bbCls, "allocateDirect",
                    "(I)Ljava/nio/ByteBuffer;"), S * S * 4);
            env->CallVoidMethod(bmp,
                env->GetMethodID(bmpCls, "copyPixelsToBuffer",
                    "(Ljava/nio/Buffer;)V"), buf);
            uint8_t* px = (uint8_t*)env->GetDirectBufferAddress(buf);
            e.icon.resize(S * S * 4);
            // pixels arrive BGRA; swap to RGBA for the texture upload
            for (int k = 0; k < S * S; ++k) {
                e.icon[k*4+0] = px[k*4+2];
                e.icon[k*4+1] = px[k*4+1];
                e.icon[k*4+2] = px[k*4+0];
                e.icon[k*4+3] = px[k*4+3];
            }
            e.iconW = e.iconH = S;
            env->DeleteLocalRef(buf); env->DeleteLocalRef(canvas);
            env->DeleteLocalRef(bmp); env->DeleteLocalRef(cfg);
            env->DeleteLocalRef(icon);
        }
        gApps.push_back(e);
    }
    LOGI("apps: %zu launchable", gApps.size());
    for (auto& a : gApps) LOGI("  %s", a.pkg.c_str());
}

static void launchApp(android_app* app, const std::string& pkg) {
    JNIEnv* env = threadEnv(app);
    jobject activity = app->activity->clazz;
    jclass actCls = env->GetObjectClass(activity);
    jmethodID getPM = env->GetMethodID(actCls, "getPackageManager",
        "()Landroid/content/pm/PackageManager;");
    jobject pm = env->CallObjectMethod(activity, getPM);
    jclass pmCls = env->GetObjectClass(pm);
    jmethodID getLaunch = env->GetMethodID(pmCls, "getLaunchIntentForPackage",
        "(Ljava/lang/String;)Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(pm, getLaunch, env->NewStringUTF(pkg.c_str()));
    if (!intent) { LOGE("no launch intent for %s", pkg.c_str()); return; }
    jmethodID startAct = env->GetMethodID(actCls, "startActivity",
        "(Landroid/content/Intent;)V");
    env->CallVoidMethod(activity, startAct, intent);
    LOGI("launched %s", pkg.c_str());
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

// textured quad for app icons - aPos + aUV, samples the icon texture as rgba
static const char* kIconVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kIconFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
void main() { gl_FragColor = texture2D(uTex, vUV); }
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

// HUD text: view-space quads sampling a 5x7 bitmap font atlas
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

// ---------------------------------------------------------------- panels

// panel wall: a full ring of quads around the viewer, one per app, so there is
// always something in view regardless of which way the IMU happens to face
static const float kRadius  = 2.4f;   // metres - close enough to feel present
static const float kRowStep = 0.55f;  // metres between rows
static const float kPanelW  = 0.62f, kPanelH = 0.50f;

struct Panel { float x, y, z; float dirx, dirz; };  // centre + facing dir

static std::vector<Panel> gPanels;

static void buildPanels() {
    gPanels.clear();
    const int n = (int)gApps.size();
    // ring the full 360° so there is always a panel wherever the head faces
    const int cols = n > 0 ? (int)ceilf(n / 4.0f) : 0;
    for (int i = 0; i < n; ++i) {
        const int col = i % cols, row = i / cols;
        const float ang = (float)col / (float)cols * 2.0f * (float)M_PI;
        Panel p;
        p.x = sinf(ang) * kRadius;
        p.y = 0.6f - row * kRowStep;
        p.z = -cosf(ang) * kRadius;
        const float len = sqrtf(p.x * p.x + p.z * p.z);
        p.dirx = -p.x / len; p.dirz = -p.z / len;
        gPanels.push_back(p);
    }
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

    GLuint sceneProg = 0, warpProg = 0, textProg = 0, iconProg = 0;
    GLuint panelVbo = 0, quadVbo = 0, gridVbo = 0, cubeVbo = 0, textVbo = 0, iconVbo = 0;
    Font font;
    std::vector<GLuint> iconTex;   // per-app icon texture, same index as gApps
    Eye eye[2];

    ASensorManager* sensorMgr = nullptr;
    const ASensor* rotSensor = nullptr;
    ASensorEventQueue* sensorQueue = nullptr;
    float quat[4] = {0, 0, 0, 1};
    bool haveQuat = false;

    int gazed = -1;      // panel under the reticle
    int appTick = 0;
    bool panelsDirty = true;
    char hud[96] = "";
    int  hudLen = 0;
    int  frames = 0;
    int  fps = 0;
    long long fpsMark = 0;
    int  sensorEv = 0;
    int  sensorHz = 0;
    int  sensorNew = 0;
    int  sensorNewHz = 0;
    int  sensorLagMs = 0;
    float lastQ[4] = {0,0,0,0};
};

static bool loadFont(Engine* e);

// one quad per panel, rebuilt when the app list changes; colour per app so
// panels are distinguishable without text
static float gPanelVerts[256 * 6 * 6];
static int   gPanelVertCount = 0;

// unit cube, per-face colour so orientation reads clearly at a glance
static const float kCube[] = {
#define F(r,g,b, x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4) \
    x1,y1,z1, r,g,b,  x2,y2,z2, r,g,b,  x3,y3,z3, r,g,b, \
    x1,y1,z1, r,g,b,  x3,y3,z3, r,g,b,  x4,y4,z4, r,g,b,
    F(0.9f,0.2f,0.2f, -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1)
    F(0.2f,0.9f,0.2f,  1,-1,-1, -1,-1,-1, -1, 1,-1,  1, 1,-1)
    F(0.2f,0.4f,0.9f,  1,-1, 1,  1,-1,-1,  1, 1,-1,  1, 1, 1)
    F(0.9f,0.9f,0.2f, -1,-1,-1, -1,-1, 1, -1, 1, 1, -1, 1,-1)
    F(0.9f,0.5f,0.1f, -1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1)
    F(0.6f,0.2f,0.8f, -1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1)
#undef F
};

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
        const float r = axis ? 0.9f : 0.22f, g = axis ? 0.9f : 0.30f, b = axis ? 0.9f : 0.40f;
        const float pts[4][3] = {{t, -1.2f, -e}, {t, -1.2f, e}, {-e, -1.2f, t}, {e, -1.2f, t}};
        for (int k = 0; k < 4; ++k) {
            gGrid[n++] = pts[k][0]; gGrid[n++] = pts[k][1]; gGrid[n++] = pts[k][2];
            gGrid[n++] = r;         gGrid[n++] = g;         gGrid[n++] = b;
        }
    }
    gGridVerts = n / 6;
}

static void rebuildPanelVbo(Engine* e) {
    gPanelVertCount = 0;
    const int n = (int)gPanels.size();
    if (n > 256) return;
    float* v = gPanelVerts;
    for (int i = 0; i < n; ++i) {
        const Panel& p = gPanels[i];
        // panel basis: right = up x forward, up = world Y
        const float fx = p.dirx, fz = p.dirz;              // toward viewer
        const float rx = -fz, rz = fx;                     // right = cross(fwd,up)
        const float hw = kPanelW / 2, hh = kPanelH / 2;
        const float px[4] = {p.x - rx*hw, p.x + rx*hw, p.x + rx*hw, p.x - rx*hw};
        const float pz[4] = {p.z - rz*hw, p.z + rz*hw, p.z + rz*hw, p.z - rz*hw};
        const float py[4] = {p.y - hh, p.y - hh, p.y + hh, p.y + hh};
        // hue from package hash
        unsigned h = 5381;
        for (char c : gApps[i].pkg) h = h * 33 + (unsigned char)c;
        const float r = 0.35f + 0.5f * ((h >> 0)  & 255) / 255.0f;
        const float g = 0.35f + 0.5f * ((h >> 8)  & 255) / 255.0f;
        const float b = 0.35f + 0.5f * ((h >> 16) & 255) / 255.0f;
        const int tris[6] = {0, 1, 2, 0, 2, 3};
        for (int t : tris) {
            *v++ = px[t]; *v++ = py[t]; *v++ = pz[t];
            *v++ = r; *v++ = g; *v++ = b;
        }
        gPanelVertCount += 6;
    }
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glBufferData(GL_ARRAY_BUFFER, gPanelVertCount * 6 * sizeof(float),
                 gPanelVerts, GL_DYNAMIC_DRAW);
}

// upload each app's icon pixels into its own texture, one per gApps entry
static void uploadIcons(Engine* e) {
    for (GLuint t : e->iconTex) glDeleteTextures(1, &t);
    e->iconTex.clear();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (auto& a : gApps) {
        if (a.icon.empty()) { e->iconTex.push_back(0); continue; }
        GLuint t;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, a.iconW, a.iconH, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, a.icon.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        e->iconTex.push_back(t);
    }
}

// icon quad on each panel face, inset and pulled a little toward the viewer
static void drawIcons(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->iconProg);
    glUniformMatrix4fv(glGetUniformLocation(e->iconProg, "uMVP"), 1, GL_FALSE,
                     viewProj.m);
    glUniform1i(glGetUniformLocation(e->iconProg, "uTex"), 0);
    const GLint aPos = glGetAttribLocation(e->iconProg, "aPos");
    const GLint aUV  = glGetAttribLocation(e->iconProg, "aUV");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aUV);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float hs = 0.20f;   // icon half-size on the panel face
    for (size_t i = 0; i < gPanels.size(); ++i) {
        if (i >= e->iconTex.size() || !e->iconTex[i]) continue;
        const Panel& p = gPanels[i];
        const float fx = p.dirx, fz = p.dirz;        // toward viewer
        const float rx = -fz, rz = fx;               // panel right
        const float cx = p.x + fx * 0.03f, cz = p.z + fz * 0.03f;
        const float cy = p.y;
        const float quad[4][5] = {   // pos.xyz + uv, top of icon at low v
            {cx - rx*hs, cy - hs, cz - rz*hs, 0.0f, 1.0f},
            {cx + rx*hs, cy - hs, cz + rz*hs, 1.0f, 1.0f},
            {cx + rx*hs, cy + hs, cz + rz*hs, 1.0f, 0.0f},
            {cx - rx*hs, cy + hs, cz - rz*hs, 0.0f, 0.0f},
        };
        const int tris[6] = {0,1,2, 0,2,3};
        float v[30];
        for (int t = 0; t < 6; ++t) memcpy(v + t*5, quad[tris[t]], 20);
        glBindTexture(GL_TEXTURE_2D, e->iconTex[i]);
        glBindBuffer(GL_ARRAY_BUFFER, e->iconVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
        glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
        glVertexAttribPointer(aUV,  2, GL_FLOAT, GL_FALSE, 20, (void*)12);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aUV);
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
    e->iconProg  = link(kIconVS,  kIconFS);
    if (!e->sceneProg || !e->warpProg || !e->textProg || !e->iconProg) return -1;

    if (!loadFont(e)) LOGE("font load failed, HUD text disabled");
    glGenBuffers(1, &e->textVbo);
    glGenBuffers(1, &e->iconVbo);

    glGenBuffers(1, &e->panelVbo);
    glGenBuffers(1, &e->quadVbo);
    glGenBuffers(1, &e->gridVbo);
    const float quad[] = {-1,-1, 1,-1, -1,1,  1,-1, 1,1, -1,1};
    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    buildGrid();
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glBufferData(GL_ARRAY_BUFFER, gGridVerts * 6 * sizeof(float), gGrid, GL_STATIC_DRAW);
    glGenBuffers(1, &e->cubeVbo);
    glBindBuffer(GL_ARRAY_BUFFER, e->cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCube), kCube, GL_STATIC_DRAW);

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
            // event timestamp is ns since boot; track delivery lag so we can
            // tell a slow stream from a fast-but-stale one
            {
                struct timespec ts;
                clock_gettime(CLOCK_BOOTTIME, &ts);
                const long long now = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
                e->sensorLagMs = (int)((now - ev.timestamp) / 1000000);
            }
            // count only samples that actually changed the quat, to see if the
            // HAL is streaming fresh data or replaying a stale batch
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

// the glue drains the input queue itself and calls this per event - polling the
// queue manually finds it already empty, so input MUST be handled here
static int32_t onInputEvent(android_app* app, AInputEvent* ev) {
    Engine* e = (Engine*)app->userData;
    if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_KEY &&
        AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_DOWN) {
        const int32_t code = AKeyEvent_getKeyCode(ev);
        LOGI("key down %d (gazed=%d)", code, e->gazed);
        if (isConfirm(code)) {
            if (e->gazed >= 0 && e->gazed < (int)gApps.size())
                launchApp(app, gApps[e->gazed].pkg);
            return 1;
        }
    }
    return 0;
}

// pick the panel closest to the gaze ray; returns index or -1
static int pickGazed(const Mat4& view) {
    float fwd[3];
    const float center[3] = {0, 0, -1};
    viewDirToWorld(view, center, fwd);
    int best = -1; float bestCos = 0.992f;   // ~7 deg cone
    for (int i = 0; i < (int)gPanels.size(); ++i) {
        const Panel& p = gPanels[i];
        const float len = sqrtf(p.x*p.x + p.y*p.y + p.z*p.z);
        const float dx = p.x / len, dy = p.y / len, dz = p.z / len;
        const float c = dx*fwd[0] + dy*fwd[1] + dz*fwd[2];
        if (c > bestCos) { bestCos = c; best = i; }
    }
    return best;
}

static void drawScene(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);

    // floor grid first so there is always geometry in frame
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glDrawArrays(GL_LINES, 0, gGridVerts);

    // panels, gazed one brightened by redrawing it scaled up
    glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, viewProj.m);
    glDrawArrays(GL_TRIANGLES, 0, gPanelVertCount);

    // highlight: redraw the gazed panel scaled about its own centre
    if (e->gazed >= 0) {
        const Panel& p = gPanels[e->gazed];
        const float s = 1.12f;
        Mat4 m = identity(); m.m[0] = s; m.m[5] = s; m.m[10] = s;
        Mat4 mv = multiply(translate3(p.x, p.y, p.z),
                    multiply(m, translate3(-p.x, -p.y, -p.z)));
        const Mat4 mvp = multiply(viewProj, mv);
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, mvp.m);
        glDrawArrays(GL_TRIANGLES, e->gazed * 6, 6);
    }
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);

    drawIcons(e, viewProj);

    // 4 reference cubes at the cardinal points, so head rotation is obvious
    glUseProgram(e->sceneProg);
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glBindBuffer(GL_ARRAY_BUFFER, e->cubeVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    static const float kCubePos[4][3] = {
        { 0, 0, -4}, { 4, 0, 0}, { 0, 0, 4}, {-4, 0, 0}   // N E S W
    };
    for (int i = 0; i < 4; ++i) {
        Mat4 m = identity();
        const float s = 0.4f;
        m.m[0] = s; m.m[5] = s; m.m[10] = s;
        Mat4 mv = multiply(translate3(kCubePos[i][0], kCubePos[i][1], kCubePos[i][2]), m);
        const Mat4 mvp = multiply(viewProj, mv);
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, mvp.m);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
}

// reticle: ring + centre dot in clip space, dead centre of each eye's view.
// This is the gaze cursor for controller-free selection - brightens on a target.
static void drawReticle(Engine* e) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glDisable(GL_DEPTH_TEST);

    const float c = (e->gazed >= 0) ? 1.0f : 0.55f;   // brighter on a target
    const int seg = 48;
    // radii in NDC; the eye viewport is ~0.89 aspect so widen x a touch
    const float r1x = 0.014f, r1y = 0.026f;
    const float r2x = 0.018f, r2y = 0.034f;
    const float dotx = 0.0035f, doty = 0.0065f;

    // ring as a triangle strip between the two radii
    float ring[(seg + 1) * 2 * 6];
    for (int i = 0; i <= seg; ++i) {
        const float a = (float)i / seg * 6.2831853f;
        const float ca = cosf(a), sa = sinf(a);
        float* o = ring + i * 12;
        o[0]=ca*r2x; o[1]=sa*r2y; o[2]=0; o[3]=c; o[4]=c; o[5]=c;
        o[6]=ca*r1x; o[7]=sa*r1y; o[8]=0; o[9]=c; o[10]=c; o[11]=c;
    }
    GLuint vbo; glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ring), ring, GL_STREAM_DRAW);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, identity().m);   // clip space
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (seg + 1) * 2);

    // centre dot as a small fan
    float center[6 + (seg + 1) * 6];
    center[0]=0; center[1]=0; center[2]=0; center[3]=c; center[4]=c; center[5]=c;
    for (int i = 0; i <= seg; ++i) {
        const float a = (float)i / seg * 6.2831853f;
        float* p = center + 6 + i * 6;
        p[0]=cosf(a)*dotx; p[1]=sinf(a)*doty; p[2]=0; p[3]=c; p[4]=c; p[5]=c;
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(center), center, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, seg + 2);

    glDeleteBuffers(1, &vbo);
    glEnable(GL_DEPTH_TEST);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
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
            // the bitmap is tightly packed at gw stride; without this GL pads
            // each row to 4 bytes and reads the glyph skewed into strips
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

// draws a utf-8 string in view space; returns its width in the same units
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
            const float gtop = y - g->yoff * mPerPx;   // yoff < 0 sits above baseline
            const float gbot = gtop - g->h * mPerPx;
            const float gw = g->w * mPerPx;
            // glyph top row sits at low v in the atlas, so quad top -> v0
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

// HUD: head-locked text showing the live camera rotation + the gazed app, so
// tracking can be verified without adb. Two lines near the top, per eye.
static void drawHud(Engine* e, const Mat4& proj) {
    if (!e->font.ok) return;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"), 1, GL_FALSE,
                     proj.m);   // view space = identity view
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"), 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float s = 0.0026f;   // metres per font pixel in view space
    const float z = -1.2f;
    float w = textWidth(e, e->hud, s);
    drawText(e, e->hud, -w * 0.5f, 0.24f, z, s);

    if (e->gazed >= 0 && e->gazed < (int)gApps.size()) {
        const char* lbl = gApps[e->gazed].label.c_str();
        w = textWidth(e, lbl, s);
        drawText(e, lbl, -w * 0.5f, 0.12f, z, s);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

static void drawFrame(Engine* e) {
    if (!e->ready) return;

    // refresh the app list every ~5s so installs show up without a relaunch
    if (++e->appTick >= 360) {
        e->appTick = 0;
        refreshApps(e->app);
        buildPanels();
        e->panelsDirty = true;
    }
    if (e->panelsDirty) {
        rebuildPanelVbo(e);
        uploadIcons(e);
        e->panelsDirty = false;
    }

    // debug.vrhome.sensor=0 pins the view; debug.vrhome.tq=0 uses the quat
    // untransposed in case the track direction reads inverted in the headset.
    // roll/sensroll/worldx are live-tunable for display orientation.
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

    e->gazed = pickGazed(head);

    // yaw/pitch/roll from the sensor quat - drives both the logcat line and the
    // in-headset HUD
    const float qx = e->quat[0], qy = e->quat[1], qz = e->quat[2], qw = e->quat[3];
    const float yaw   = atan2f(2*(qw*qy + qx*qz), 1 - 2*(qy*qy + qx*qx)) * 180.0f / (float)M_PI;
    const float pitch = asinf(fmaxf(-1.0f, fminf(1.0f, 2*(qw*qx - qy*qz)))) * 180.0f / (float)M_PI;
    const float roll  = atan2f(2*(qw*qz + qx*qy), 1 - 2*(qz*qz + qx*qx)) * 180.0f / (float)M_PI;
    e->hudLen = snprintf(e->hud, sizeof(e->hud),
        "YAW %+4.0f  PIT %+4.0f  ROL %+4.0f  FPS %d  SEN %d  LAG %dMS%s", yaw, pitch, roll,
        e->fps, e->sensorNewHz, e->sensorLagMs, useSensor ? "" : "  SEN:OFF");

    if ((e->appTick % 144) == 0) {
        float fwd[3]; const float c[3] = {0,0,-1};
        viewDirToWorld(head, c, fwd);
        LOGI("rot quat=(%.2f,%.2f,%.2f,%.2f) ypr=(%.0f,%.0f,%.0f) fwd=(%.2f,%.2f,%.2f) gazed=%d",
             qx, qy, qz, qw, yaw, pitch, roll, fwd[0], fwd[1], fwd[2], e->gazed);
    }

    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const Mat4 proj = perspective(kFovY, aspect, 0.05f, 100.0f);

    static int errTick = 0;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glViewport(0, 0, y.w, y.h);
        // debug.vrhome.fill=1 paints each eye a solid colour to prove the path
        if (propI("debug.vrhome.fill", 0))
            glClearColor(i == 0 ? 0.8f : 0.1f, 0.1f, i == 1 ? 0.8f : 0.1f, 1.0f);
        else
            glClearColor(0.10f, 0.12f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // per-eye offset for IPD
        Mat4 eyeView = head;
        const float off = (i == 0 ? -kIPD / 2 : kIPD / 2);
        Mat4 shift = identity(); shift.m[12] = off;
        eyeView = multiply(eyeView, shift);
        const Mat4 vp = multiply(proj, eyeView);
        drawScene(e, vp);
        drawReticle(e);
        if (propI("debug.vrhome.hud", 1)) drawHud(e, proj);
        if (++errTick >= 144) {
            errTick = 0;
            GLenum ge = glGetError();
            if (ge != GL_NO_ERROR) LOGE("GL error 0x%x", ge);
        }
    }

    // warp pass: each eye texture through barrel distortion to its half.
    // depth test must be OFF here - the default framebuffer has a depth buffer
    // and the fullscreen quads would fail GL_LESS on equal depth after frame 1
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

    // fps: count presented frames, refresh the number once a second
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
    case APP_CMD_GAINED_FOCUS:
        if (e->rotSensor)
            ASensorEventQueue_enableSensor(e->sensorQueue, e->rotSensor);
        break;
    case APP_CMD_LOST_FOCUS:
        if (e->rotSensor)
            ASensorEventQueue_disableSensor(e->sensorQueue, e->rotSensor);
        break;
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
        // fastest rate - the fused output on this HAL lags badly at the
        // default, which reads as ~2fps head tracking in the headset
        ASensorEventQueue_setEventRate(e.sensorQueue, e.rotSensor, 2000);
    }

    refreshApps(app);
    buildPanels();

    // immersive: hide nav + status bars so nothing but our scene shows
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
    // panelVbo does not exist until initDisplay; the dirty flag handles it

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
