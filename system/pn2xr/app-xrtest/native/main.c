// Minimal OpenXR test app for the Pico Neo 2 Monado runtime.
// Loads libopenxr_monado.so directly (in-process), renders a simple stereo
// scene with GLES2, and draws a world-locked debug panel with live tracking
// state: head pose, view flags, per-controller poses/buttons/stick, fps.

#include <android_native_app_glue.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>

#include <dlfcn.h>
#include <jni.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/system_properties.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

#define LOG_TAG "xrtest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr_fn;

#define XRP(fn) static PFN_##fn pfn_##fn
XRP(xrCreateInstance);
XRP(xrDestroyInstance);
XRP(xrGetSystem);
XRP(xrGetSystemProperties);
XRP(xrEnumerateViewConfigurationViews);
XRP(xrCreateSession);
XRP(xrDestroySession);
XRP(xrPollEvent);
XRP(xrBeginSession);
XRP(xrEndSession);
XRP(xrRequestExitSession);
XRP(xrWaitFrame);
XRP(xrBeginFrame);
XRP(xrEndFrame);
XRP(xrCreateReferenceSpace);
XRP(xrDestroySpace);
XRP(xrLocateViews);
XRP(xrEnumerateSwapchainFormats);
XRP(xrCreateSwapchain);
XRP(xrDestroySwapchain);
XRP(xrEnumerateSwapchainImages);
XRP(xrAcquireSwapchainImage);
XRP(xrWaitSwapchainImage);
XRP(xrReleaseSwapchainImage);
XRP(xrEnumerateInstanceExtensionProperties);
XRP(xrGetOpenGLESGraphicsRequirementsKHR);
XRP(xrCreateActionSet);
XRP(xrCreateAction);
XRP(xrStringToPath);
XRP(xrSuggestInteractionProfileBindings);
XRP(xrAttachSessionActionSets);
XRP(xrCreateActionSpace);
XRP(xrSyncActions);
XRP(xrGetActionStateBoolean);
XRP(xrGetActionStateFloat);
XRP(xrGetActionStateVector2f);
XRP(xrLocateSpace);
#undef XRP

static bool load_pfn(XrInstance inst, PFN_xrVoidFunction *out, const char *name) {
    return XR_SUCCEEDED(xrGetInstanceProcAddr_fn(inst, name, out));
}

// ---------- math ----------

static void mat4_proj(XrFovf fov, float zn, float zf, float *m) {
    float l = tanf(fov.angleLeft) * zn, r = tanf(fov.angleRight) * zn;
    float t = tanf(fov.angleUp) * zn, b = tanf(fov.angleDown) * zn;
    memset(m, 0, 64);
    m[0] = 2 * zn / (r - l);
    m[5] = 2 * zn / (t - b);
    m[8] = (r + l) / (r - l);
    m[9] = (t + b) / (t - b);
    m[10] = -(zf + zn) / (zf - zn);
    m[11] = -1;
    m[14] = -2 * zf * zn / (zf - zn);
}

// pose -> view matrix (inverse)
static void mat4_view_from_pose(XrPosef p, float *m) {
    XrQuaternionf q = p.orientation;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    float R[9] = {
        1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy),
        2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx),
        2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy)};
    float tx = p.position.x, ty = p.position.y, tz = p.position.z;
    float ix = -(R[0] * tx + R[3] * ty + R[6] * tz);
    float iy = -(R[1] * tx + R[4] * ty + R[7] * tz);
    float iz = -(R[2] * tx + R[5] * ty + R[8] * tz);
    m[0] = R[0]; m[4] = R[3]; m[8] = R[6];  m[12] = ix;
    m[1] = R[1]; m[5] = R[4]; m[9] = R[7];  m[13] = iy;
    m[2] = R[2]; m[6] = R[5]; m[10] = R[8]; m[14] = iz;
    m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;
}

// pose -> model matrix
static void mat4_model_from_pose(XrPosef p, float *m) {
    XrQuaternionf q = p.orientation;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float R[9] = {
        1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
        2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
        2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)};
    m[0] = R[0]; m[4] = R[1]; m[8] = R[2];  m[12] = p.position.x;
    m[1] = R[3]; m[5] = R[4]; m[9] = R[5];  m[13] = p.position.y;
    m[2] = R[6]; m[6] = R[7]; m[10] = R[8]; m[14] = p.position.z;
    m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;
}

static void mat4_mul(float *out, const float *a, const float *b) {
    float r[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[j * 4 + i] = a[i] * b[j * 4] + a[4 + i] * b[j * 4 + 1] +
                           a[8 + i] * b[j * 4 + 2] + a[12 + i] * b[j * 4 + 3];
    memcpy(out, r, sizeof(r));
}

static void mat4_scale(float *m, float s) {
    memset(m, 0, 64);
    m[0] = s; m[5] = s; m[10] = s; m[15] = 1.0f;
}

static void mat4_rot_y(float *m, float rad) {
    memset(m, 0, 64);
    float c = cosf(rad), s = sinf(rad);
    m[0] = c; m[2] = -s; m[5] = 1; m[8] = s; m[10] = c; m[15] = 1;
}

static void mat4_translate(float *m, float x, float y, float z) {
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    m[12] = x; m[13] = y; m[14] = z;
}

// pose orientation applied to v (for controller aim rays)
static void quat_rot(XrQuaternionf q, float vx, float vy, float vz, float *out) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    // v' = v + 2*cross(q.xyz, cross(q.xyz, v) + w*v)
    float cx = y * vz - z * vy + w * vx;
    float cy = z * vx - x * vz + w * vy;
    float cz = x * vy - y * vx + w * vz;
    out[0] = vx + 2 * (y * cz - z * cy);
    out[1] = vy + 2 * (z * cx - x * cz);
    out[2] = vz + 2 * (x * cy - y * cx);
}

// ---------- gles ----------

static const char *VS =
    "attribute vec3 aPos; attribute vec3 aCol; uniform mat4 uMvp; uniform vec3 uTint; varying vec3 vCol;"
    "void main(){ vCol=aCol*uTint; gl_Position=uMvp*vec4(aPos,1.0);}";
static const char *FS =
    "precision mediump float; varying vec3 vCol; void main(){ gl_FragColor=vec4(vCol,1.0);}";

static const char *TVS =
    "attribute vec3 aPos; attribute vec2 aUV; uniform mat4 uMvp; varying vec2 vUV;"
    "void main(){ vUV=aUV; gl_Position=uMvp*vec4(aPos,1.0);}";
static const char *TFS =
    "precision mediump float; varying vec2 vUV; uniform sampler2D uTex; uniform vec4 uCol;"
    "void main(){ float a=texture2D(uTex,vUV).a; if(a<0.02) discard; gl_FragColor=vec4(uCol.rgb,uCol.a*a);}";

static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        LOGE("shader: %s", log);
    }
    return s;
}

static GLuint link_prog(const char *vs, const char *fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, compile(GL_VERTEX_SHADER, vs));
    glAttachShader(p, compile(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    return p;
}

// ---------- font ----------

#define FONT_PX 34
#define ATLAS_W 512
#define ATLAS_H 512
#define FONT_FIRST 32
#define FONT_COUNT 96

static stbtt_bakedchar g_baked[FONT_COUNT];
static GLuint g_font_tex = 0;
static bool g_font_ok = false;

static const char *g_font_paths[] = {
    "/system/fonts/Roboto-Regular.ttf",
    "/system/fonts/DroidSans.ttf",
    "/system/fonts/NotoSansMono-Regular.ttf",
    "/data/local/tmp/xr/font.ttf",
    NULL};

static void init_font(void) {
    FILE *f = NULL;
    for (int i = 0; g_font_paths[i]; i++) {
        f = fopen(g_font_paths[i], "rb");
        if (f) {
            LOGI("font: %s", g_font_paths[i]);
            break;
        }
    }
    if (!f) { LOGE("no usable ttf"); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *ttf = malloc(sz);
    if (fread(ttf, 1, sz, f) != (size_t)sz) { fclose(f); free(ttf); return; }
    fclose(f);

    unsigned char *bmp = calloc(ATLAS_W * ATLAS_H, 1);
    int res = stbtt_BakeFontBitmap(ttf, 0, FONT_PX, bmp, ATLAS_W, ATLAS_H,
                                   FONT_FIRST, FONT_COUNT, g_baked);
    free(ttf);
    if (res <= 0) { LOGE("bake failed %d", res); free(bmp); return; }

    glGenTextures(1, &g_font_tex);
    glBindTexture(GL_TEXTURE_2D, g_font_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_W, ATLAS_H, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, bmp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    free(bmp);
    g_font_ok = true;
}

// ---------- debug panel ----------

// world-locked panel straight ahead; px space mapped through kMPX
#define PANEL_Z   (-1.7f)
#define PANEL_CX  0.0f
#define PANEL_CY  0.25f
#define PANEL_W   1.54f
#define PANEL_H   0.97f
#define kMPX      (PANEL_W / 1400.0f)   // metres per panel px
#define PANEL_X0  (PANEL_CX - PANEL_W / 2)
#define PANEL_Y0  (PANEL_CY + PANEL_H / 2)

#define MAX_TEXT_CHARS 4096
static float g_textv[MAX_TEXT_CHARS * 6 * 5];
static int g_textn; // vertex count
static float g_penx, g_baseline;

static void text_reset(void) { g_textn = 0; }

static void text_str(float px, float py, const char *s) {
    if (!g_font_ok) return;
    g_penx = px;
    g_baseline = py + FONT_PX;
    for (; *s && g_textn < MAX_TEXT_CHARS * 6 - 6; s++) {
        int c = *s;
        if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) c = '?';
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(g_baked, ATLAS_W, ATLAS_H, c - FONT_FIRST,
                           &g_penx, &g_baseline, &q, 1);
        float x0 = PANEL_X0 + q.x0 * kMPX, x1 = PANEL_X0 + q.x1 * kMPX;
        float y0 = PANEL_Y0 - q.y0 * kMPX, y1 = PANEL_Y0 - q.y1 * kMPX;
        float *v = g_textv + g_textn * 5;
        float quad[6][5] = {
            {x0, y0, PANEL_Z, q.s0, q.t0}, {x1, y0, PANEL_Z, q.s1, q.t0},
            {x0, y1, PANEL_Z, q.s0, q.t1},
            {x0, y1, PANEL_Z, q.s0, q.t1}, {x1, y0, PANEL_Z, q.s1, q.t0},
            {x1, y1, PANEL_Z, q.s1, q.t1}};
        memcpy(v, quad, sizeof(quad));
        g_textn += 6;
    }
}

// ---------- geometry ----------

static GLuint vbo, ibo, cube_vbo, panel_vbo, line_vbo, text_vbo;
static int g_lines, g_tris;

static void build_scene(void) {
    static float v[4096 * 6];
    static uint16_t idx[8192];
    int nv = 0, ni = 0;
    // floor grid at y=-1.5, xz in [-10,10]
    for (int i = -10; i <= 10; i++) {
        float c = (i == 0) ? 0.9f : 0.3f;
        v[nv*6+0]=-10; v[nv*6+1]=-1.5f; v[nv*6+2]=i;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=10;  v[nv*6+1]=-1.5f; v[nv*6+2]=i;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=i;   v[nv*6+1]=-1.5f; v[nv*6+2]=-10; v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=i;   v[nv*6+1]=-1.5f; v[nv*6+2]=10;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
    }
    g_lines = nv;
    for (int i = 0; i < g_lines; i++) idx[ni++] = i;

    // marker cubes: {x,y,z,colorIdx}
    const float cubes[][4] = {
        {0, 0, -3, 0}, {-3, 0, -1, 1}, {3, 0, -1, 2}, {0, 2, -6, 3}, {0, -1.4f, 2, 4}};
    const float cols[][3] = {
        {0.9f, 0.2f, 0.2f}, {0.2f, 0.9f, 0.2f}, {0.25f, 0.35f, 0.95f},
        {0.9f, 0.85f, 0.2f}, {0.7f, 0.25f, 0.8f}};
    static const uint16_t ci36[36] = {
        0,1,2, 2,1,3, 4,6,5, 5,6,7, 0,4,1, 1,4,5,
        2,3,6, 6,3,7, 0,2,4, 4,2,6, 1,5,3, 3,5,7};
    for (int c = 0; c < 5; c++) {
        int base = nv;
        float cx = cubes[c][0], cy = cubes[c][1], cz = cubes[c][2], s = 0.25f;
        int col = (int)cubes[c][3];
        for (int fi = 0; fi < 8; fi++) {
            v[nv*6+0]=cx+((fi&1)?s:-s); v[nv*6+1]=cy+((fi&2)?s:-s); v[nv*6+2]=cz+((fi&4)?s:-s);
            v[nv*6+3]=cols[col][0]; v[nv*6+4]=cols[col][1]; v[nv*6+5]=cols[col][2];
            nv++;
        }
        for (int k = 0; k < 36; k++) idx[ni++] = base + ci36[k];
    }
    g_tris = ni - g_lines;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, nv * 6 * sizeof(float), v, GL_STATIC_DRAW);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(uint16_t), idx, GL_STATIC_DRAW);

    // unit cube, 36 non-indexed verts, pale faces; uTint colours each instance
    static float cu[36 * 6];
    static const uint8_t face_col[6][3] = {
        {230, 230, 235}, {190, 190, 200}, {215, 215, 225},
        {160, 160, 175}, {205, 205, 215}, {140, 140, 155}};
    // faces: +z -z +x -x +y -y as 2 tris each over a unit cube
    static const float fc[6][4][3] = {
        {{-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}},
        {{ 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}},
        {{ 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}},
        {{-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}},
        {{-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1}, {-1, 1,-1}},
        {{-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1}}};
    int cn = 0;
    for (int f = 0; f < 6; f++) {
        const uint8_t *cc = face_col[f];
        const int order[6] = {0, 1, 2, 0, 2, 3};
        for (int k = 0; k < 6; k++) {
            const float *p = fc[f][order[k]];
            cu[cn*6+0] = p[0]; cu[cn*6+1] = p[1]; cu[cn*6+2] = p[2];
            cu[cn*6+3] = cc[0] / 255.f; cu[cn*6+4] = cc[1] / 255.f; cu[cn*6+5] = cc[2] / 255.f;
            cn++;
        }
    }
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cu), cu, GL_STATIC_DRAW);

    // panel background quad, opaque dark
    float bg[6 * 6];
    const float bc[3] = {0.03f, 0.04f, 0.07f};
    const float bz = PANEL_Z - 0.002f;
    const float bq[4][2] = {
        {PANEL_X0, PANEL_Y0 - PANEL_H}, {PANEL_X0 + PANEL_W, PANEL_Y0 - PANEL_H},
        {PANEL_X0, PANEL_Y0}, {PANEL_X0 + PANEL_W, PANEL_Y0}};
    const int bord[6] = {0, 1, 2, 2, 1, 3};
    for (int k = 0; k < 6; k++) {
        bg[k*6+0] = bq[bord[k]][0]; bg[k*6+1] = bq[bord[k]][1]; bg[k*6+2] = bz;
        bg[k*6+3] = bc[0]; bg[k*6+4] = bc[1]; bg[k*6+5] = bc[2];
    }
    glGenBuffers(1, &panel_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, panel_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_STATIC_DRAW);

    glGenBuffers(1, &line_vbo);
    glGenBuffers(1, &text_vbo);
}

// ---------- actions ----------

struct HandAct {
    XrAction sel, menu, ax, by, trigc, sqzc, stickc;
    XrAction trigv, sqzv, stick;
    XrAction aim, grip;
    XrSpace aim_space, grip_space;
};

static XrAction mk_action(XrActionSet aset, const char *name, XrActionType type,
                        XrPath sub) {
    XrActionCreateInfo aci = {XR_TYPE_ACTION_CREATE_INFO};
    strncpy(aci.actionName, name, sizeof(aci.actionName) - 1);
    strncpy(aci.localizedActionName, name, sizeof(aci.localizedActionName) - 1);
    aci.actionType = type;
    aci.countSubactionPaths = 1;
    aci.subactionPaths = &sub;
    XrAction a = XR_NULL_HANDLE;
    pfn_xrCreateAction(aset, &aci, &a);
    return a;
}

static XrPath to_path(XrInstance inst, const char *s) {
    XrPath p = XR_NULL_PATH;
    pfn_xrStringToPath(inst, s, &p);
    return p;
}

// ---------- debug text ----------

static const char *sess_state_str(int s) {
    switch (s) {
    case XR_SESSION_STATE_IDLE: return "IDLE";
    case XR_SESSION_STATE_READY: return "READY";
    case XR_SESSION_STATE_SYNCHRONIZED: return "SYNCHRONIZED";
    case XR_SESSION_STATE_VISIBLE: return "VISIBLE";
    case XR_SESSION_STATE_FOCUSED: return "FOCUSED";
    case XR_SESSION_STATE_STOPPING: return "STOPPING";
    case XR_SESSION_STATE_LOSS_PENDING: return "LOSS_PENDING";
    case XR_SESSION_STATE_EXITING: return "EXITING";
    default: return "UNKNOWN";
    }
}

static void prop_str(const char *key, char *out, int outlen) {
    if (__system_property_get(key, out) <= 0)
        snprintf(out, outlen, "-");
}

// ---------- main ----------

void android_main(struct android_app *app) {
    app_dummy();
    JNIEnv *env;
    (*app->activity->vm)->AttachCurrentThread(app->activity->vm, &env, NULL);

    // EGL pbuffer context for the GLES binding
    EGLDisplay edpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(edpy, NULL, NULL);
    EGLint cfg_attrs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                          EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                          EGL_ALPHA_SIZE, 8, EGL_NONE};
    EGLConfig ecfg;
    EGLint ncfg = 0;
    eglChooseConfig(edpy, cfg_attrs, NULL, 0, &ncfg);
    eglChooseConfig(edpy, cfg_attrs, &ecfg, 1, &ncfg);
    EGLSurface pbuf = eglCreatePbufferSurface(edpy, ecfg,
        (EGLint[]){EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE});
    EGLContext ectx = eglCreateContext(edpy, ecfg, EGL_NO_CONTEXT,
        (EGLint[]){EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE});
    eglMakeCurrent(edpy, pbuf, pbuf, ectx);
    LOGI("egl ready");

    // Stock Khronos loader first: it discovers the system runtime
    // (MonadoOpenXR) through /system/etc/openxr/1/active_runtime.json or the
    // OpenXRRuntimeService package - the same shape apps use on Quest, where
    // no runtime-specific code lives in the app. If the loader or the system
    // runtime is missing, fall back to the bundled libopenxr_monado.so.
    bool rt_bundled = false;
    void *loader = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_GLOBAL);
    if (loader) {
        PFN_xrGetInstanceProcAddr gipa =
            (PFN_xrGetInstanceProcAddr)dlsym(loader, "xrGetInstanceProcAddr");
        // the android loader needs the app context before any other call -
        // runtime discovery goes through PackageManager on it
        PFN_xrInitializeLoaderKHR init_pfn = NULL;
        if (gipa)
            gipa(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                 (PFN_xrVoidFunction *)&init_pfn);
        XrLoaderInitInfoAndroidKHR init = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        init.applicationVM = app->activity->vm;
        init.applicationContext = app->activity->clazz;
        if (init_pfn && XR_SUCCEEDED(init_pfn((XrLoaderInitInfoBaseHeaderKHR *)&init)))
            xrGetInstanceProcAddr_fn = gipa;
    }
    if (!xrGetInstanceProcAddr_fn) {
        rt_bundled = true;
        void *rt = dlopen("libopenxr_monado.so", RTLD_NOW | RTLD_GLOBAL);
        if (!rt) { LOGE("dlopen: %s", dlerror()); return; }
        PFN_xrNegotiateLoaderRuntimeInterface negotiate =
            (PFN_xrNegotiateLoaderRuntimeInterface)dlsym(rt, "xrNegotiateLoaderRuntimeInterface");
        if (!negotiate) { LOGE("no negotiate"); return; }

        XrNegotiateLoaderInfo li = {0};
        li.structType = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
        li.structVersion = XR_LOADER_INFO_STRUCT_VERSION;
        li.structSize = sizeof(li);
        li.minInterfaceVersion = 1;
        li.maxInterfaceVersion = 1;
        li.minApiVersion = XR_API_VERSION_1_0;
        li.maxApiVersion = XR_CURRENT_API_VERSION;
        XrNegotiateRuntimeRequest rr = {0};
        rr.structType = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
        rr.structVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
        rr.structSize = sizeof(rr);
        XrResult nr = negotiate(&li, &rr);
        LOGI("negotiate -> %d iface=%u", nr, rr.runtimeInterfaceVersion);
        if (XR_FAILED(nr) || !rr.getInstanceProcAddr) return;
        xrGetInstanceProcAddr_fn = rr.getInstanceProcAddr;
    }
    LOGI("runtime path: %s", rt_bundled ? "bundled" : "system");

    load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrCreateInstance, "xrCreateInstance");
    load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrEnumerateInstanceExtensionProperties, "xrEnumerateInstanceExtensionProperties");

    // extension list: BD_controller_interaction unlocks the full pico_neo3
    // input profile (stick/squeeze/trigger value); simple_controller covers
    // the rest either way
    uint32_t ext_count = 0;
    pfn_xrEnumerateInstanceExtensionProperties(NULL, 0, &ext_count, NULL);
    XrExtensionProperties exts_avail[64];
    for (uint32_t i = 0; i < ext_count && i < 64; i++)
        exts_avail[i].type = XR_TYPE_EXTENSION_PROPERTIES;
    if (ext_count > 64) ext_count = 64;
    pfn_xrEnumerateInstanceExtensionProperties(NULL, ext_count, &ext_count, exts_avail);
    bool have_bd = false;
    for (uint32_t i = 0; i < ext_count; i++) {
        LOGI("ext: %s", exts_avail[i].extensionName);
        if (!strcmp(exts_avail[i].extensionName, "XR_BD_controller_interaction"))
            have_bd = true;
    }

    // instance
    XrInstanceCreateInfoAndroidKHR andr = {
        XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR, NULL,
        app->activity->vm, app->activity->clazz};
    const char *exts[] = {"XR_KHR_android_create_instance", "XR_KHR_opengl_es_enable",
                          "XR_BD_controller_interaction"};
    XrInstanceCreateInfo ici = {XR_TYPE_INSTANCE_CREATE_INFO};
    ici.next = &andr;
    strcpy(ici.applicationInfo.applicationName, "xrtest");
    ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    ici.enabledExtensionCount = have_bd ? 3 : 2;
    ici.enabledExtensionNames = exts;
    XrInstance inst = XR_NULL_HANDLE;
    XrResult r = pfn_xrCreateInstance(&ici, &inst);
    LOGI("xrCreateInstance -> %d (bd=%d)", r, have_bd);
    if (XR_FAILED(r) && !rt_bundled) {
        // loader is present but found no usable system runtime - try the
        // bundled copy once before giving up
        LOGE("system runtime unusable (%d), falling back to bundled", r);
        void *rt = dlopen("libopenxr_monado.so", RTLD_NOW | RTLD_GLOBAL);
        PFN_xrNegotiateLoaderRuntimeInterface negotiate = rt ?
            (PFN_xrNegotiateLoaderRuntimeInterface)dlsym(rt, "xrNegotiateLoaderRuntimeInterface") : NULL;
        XrNegotiateLoaderInfo li = {0};
        li.structType = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
        li.structVersion = XR_LOADER_INFO_STRUCT_VERSION;
        li.structSize = sizeof(li);
        li.minInterfaceVersion = 1;
        li.maxInterfaceVersion = 1;
        li.minApiVersion = XR_API_VERSION_1_0;
        li.maxApiVersion = XR_CURRENT_API_VERSION;
        XrNegotiateRuntimeRequest rr = {0};
        rr.structType = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
        rr.structVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
        rr.structSize = sizeof(rr);
        if (negotiate && XR_SUCCEEDED(negotiate(&li, &rr)) && rr.getInstanceProcAddr) {
            rt_bundled = true;
            xrGetInstanceProcAddr_fn = rr.getInstanceProcAddr;
            load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrCreateInstance, "xrCreateInstance");
            r = pfn_xrCreateInstance(&ici, &inst);
            LOGI("xrCreateInstance (bundled) -> %d", r);
        }
    }
    if (XR_FAILED(r)) return;

    // instance-level functions resolve against the real instance
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrDestroyInstance, "xrDestroyInstance");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetSystem, "xrGetSystem");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetSystemProperties, "xrGetSystemProperties");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrEnumerateViewConfigurationViews, "xrEnumerateViewConfigurationViews");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateSession, "xrCreateSession");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrDestroySession, "xrDestroySession");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrPollEvent, "xrPollEvent");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrBeginSession, "xrBeginSession");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrEndSession, "xrEndSession");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrRequestExitSession, "xrRequestExitSession");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrWaitFrame, "xrWaitFrame");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrBeginFrame, "xrBeginFrame");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrEndFrame, "xrEndFrame");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateReferenceSpace, "xrCreateReferenceSpace");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrDestroySpace, "xrDestroySpace");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrLocateViews, "xrLocateViews");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrEnumerateSwapchainFormats, "xrEnumerateSwapchainFormats");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateSwapchain, "xrCreateSwapchain");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrDestroySwapchain, "xrDestroySwapchain");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrEnumerateSwapchainImages, "xrEnumerateSwapchainImages");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrAcquireSwapchainImage, "xrAcquireSwapchainImage");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrWaitSwapchainImage, "xrWaitSwapchainImage");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrReleaseSwapchainImage, "xrReleaseSwapchainImage");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetOpenGLESGraphicsRequirementsKHR, "xrGetOpenGLESGraphicsRequirementsKHR");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateActionSet, "xrCreateActionSet");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateAction, "xrCreateAction");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrStringToPath, "xrStringToPath");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrSuggestInteractionProfileBindings, "xrSuggestInteractionProfileBindings");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrAttachSessionActionSets, "xrAttachSessionActionSets");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrCreateActionSpace, "xrCreateActionSpace");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrSyncActions, "xrSyncActions");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetActionStateBoolean, "xrGetActionStateBoolean");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetActionStateFloat, "xrGetActionStateFloat");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrGetActionStateVector2f, "xrGetActionStateVector2f");
    load_pfn(inst, (PFN_xrVoidFunction *)&pfn_xrLocateSpace, "xrLocateSpace");

    XrSystemGetInfo sgi = {XR_TYPE_SYSTEM_GET_INFO};
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId sys = 0;
    r = pfn_xrGetSystem(inst, &sgi, &sys);
    LOGI("pfn_xrGetSystem -> %d sys=%llu", r, (unsigned long long)sys);
    if (XR_FAILED(r)) return;

    XrSystemProperties sp = {XR_TYPE_SYSTEM_PROPERTIES};
    pfn_xrGetSystemProperties(inst, sys, &sp);
    LOGI("system: %s pos=%d ori=%d maxLayer=%u", sp.systemName,
         sp.trackingProperties.positionTracking, sp.trackingProperties.orientationTracking,
         sp.graphicsProperties.maxLayerCount);

    uint32_t vcount = 0;
    pfn_xrEnumerateViewConfigurationViews(inst, sys, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                      0, &vcount, NULL);
    XrViewConfigurationView vcv[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW},
                                      {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    pfn_xrEnumerateViewConfigurationViews(inst, sys, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                      2, &vcount, vcv);
    LOGI("views=%u rec=%ux%u", vcount,
         vcv[0].recommendedImageRectWidth, vcv[0].recommendedImageRectHeight);

    // required before session creation for GLES bindings
    XrGraphicsRequirementsOpenGLESKHR greq = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
    r = pfn_xrGetOpenGLESGraphicsRequirementsKHR(inst, sys, &greq);
    LOGI("gles reqs -> %d min=%llu max=%llu", r,
         (unsigned long long)greq.minApiVersionSupported,
         (unsigned long long)greq.maxApiVersionSupported);

    XrGraphicsBindingOpenGLESAndroidKHR gb = {
        XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR, NULL, edpy, ecfg, ectx};
    XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
    sci.next = &gb;
    sci.systemId = sys;
    XrSession sess = XR_NULL_HANDLE;
    r = pfn_xrCreateSession(inst, &sci, &sess);
    LOGI("pfn_xrCreateSession -> %d", r);
    if (XR_FAILED(r)) return;

    XrReferenceSpaceCreateInfo rsci = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    rsci.poseInReferenceSpace.orientation.w = 1.0f;
    XrSpace space = XR_NULL_HANDLE;
    r = pfn_xrCreateReferenceSpace(sess, &rsci, &space);
    LOGI("space -> %d", r);

    uint32_t fmt_count = 0;
    pfn_xrEnumerateSwapchainFormats(sess, 0, &fmt_count, NULL);
    int64_t fmts[32];
    if (fmt_count > 32) fmt_count = 32;
    pfn_xrEnumerateSwapchainFormats(sess, fmt_count, &fmt_count, fmts);
    int64_t fmt = fmts[0];
    for (uint32_t i = 0; i < fmt_count; i++) {
        LOGI("fmt 0x%llx", (long long)fmts[i]);
        if (fmts[i] == 0x8C43 || fmts[i] == 0x8058) fmt = fmts[i];
    }
    LOGI("using fmt 0x%llx", (long long)fmt);

    XrSwapchain sc[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSwapchainImageOpenGLESKHR *imgs[2] = {NULL, NULL};
    uint32_t img_count[2] = {0, 0};
    for (int eye = 0; eye < 2; eye++) {
        XrSwapchainCreateInfo scci = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        scci.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        scci.format = fmt;
        scci.sampleCount = vcv[eye].recommendedSwapchainSampleCount ? vcv[eye].recommendedSwapchainSampleCount : 1;
        scci.width = vcv[eye].recommendedImageRectWidth;
        scci.height = vcv[eye].recommendedImageRectHeight;
        scci.faceCount = 1;
        scci.arraySize = 1;
        scci.mipCount = 1;
        r = pfn_xrCreateSwapchain(sess, &scci, &sc[eye]);
        LOGI("swapchain eye%d -> %d (%ux%u)", eye, r, scci.width, scci.height);
        if (XR_FAILED(r)) return;
        pfn_xrEnumerateSwapchainImages(sc[eye], 0, &img_count[eye], NULL);
        imgs[eye] = malloc(sizeof(XrSwapchainImageOpenGLESKHR) * img_count[eye]);
        for (uint32_t i = 0; i < img_count[eye]; i++)
            imgs[eye][i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        pfn_xrEnumerateSwapchainImages(sc[eye], img_count[eye], &img_count[eye],
                                   (XrSwapchainImageBaseHeader *)imgs[eye]);
    }

    // actions: the whole pico_neo3 surface plus a simple_controller fallback.
    // xrSyncActions is what drives update_inputs on the controller devices.
    XrPath hand[2];
    hand[0] = to_path(inst, "/user/hand/left");
    hand[1] = to_path(inst, "/user/hand/right");

    XrActionSet aset = XR_NULL_HANDLE;
    XrActionSetCreateInfo asci = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(asci.actionSetName, "test");
    strcpy(asci.localizedActionSetName, "test");
    r = pfn_xrCreateActionSet(inst, &asci, &aset);
    LOGI("actionset -> %d", r);

    struct HandAct ha[2];
    memset(ha, 0, sizeof(ha));
    const char *hn[2] = {"l", "r"};
    for (int h = 0; h < 2; h++) {
        char nm[32];
        #define MK(field, type)                                          \
            snprintf(nm, sizeof(nm), "%s_%s", #field, hn[h]);            \
            ha[h].field = mk_action(aset, nm, type, hand[h])
        MK(sel,    XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(menu,   XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(ax,     XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(by,     XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(trigc,  XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(sqzc,   XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(stickc, XR_ACTION_TYPE_BOOLEAN_INPUT);
        MK(trigv,  XR_ACTION_TYPE_FLOAT_INPUT);
        MK(sqzv,   XR_ACTION_TYPE_FLOAT_INPUT);
        MK(stick,  XR_ACTION_TYPE_VECTOR2F_INPUT);
        MK(aim,    XR_ACTION_TYPE_POSE_INPUT);
        MK(grip,   XR_ACTION_TYPE_POSE_INPUT);
        #undef MK
    }

    // pico_neo3 bindings (only meaningful with XR_BD_controller_interaction)
    {
        XrActionSuggestedBinding b[22];
        int nb = 0;
        for (int h = 0; h < 2; h++) {
            const char *side = h ? "right" : "left";
            char p[96];
            #define BIND(act, fmt)                                        \
                snprintf(p, sizeof(p), fmt, side);                        \
                b[nb].action = ha[h].act;                                 \
                b[nb].binding = to_path(inst, p);                         \
                nb++
            BIND(aim,    "/user/hand/%s/input/aim/pose");
            BIND(grip,   "/user/hand/%s/input/grip/pose");
            BIND(trigv,  "/user/hand/%s/input/trigger/value");
            BIND(trigc,  "/user/hand/%s/input/trigger/click");
            BIND(sqzv,   "/user/hand/%s/input/squeeze/value");
            BIND(sqzc,   "/user/hand/%s/input/squeeze/click");
            BIND(stick,  "/user/hand/%s/input/thumbstick");
            BIND(stickc, "/user/hand/%s/input/thumbstick/click");
            BIND(menu,   "/user/hand/%s/input/menu/click");
            BIND(ax,     h ? "/user/hand/%s/input/a/click" : "/user/hand/%s/input/x/click");
            BIND(by,     h ? "/user/hand/%s/input/b/click" : "/user/hand/%s/input/y/click");
            #undef BIND
        }
        XrInteractionProfileSuggestedBinding sug = {
            XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        sug.interactionProfile =
            to_path(inst, "/interaction_profiles/bytedance/pico_neo3_controller");
        sug.countSuggestedBindings = nb;
        sug.suggestedBindings = b;
        r = pfn_xrSuggestInteractionProfileBindings(inst, &sug);
        LOGI("suggest pico_neo3 -> %d", r);
    }

    // simple_controller fallback: select=trigger, menu, aim+grip poses
    {
        XrActionSuggestedBinding b[8];
        int nb = 0;
        for (int h = 0; h < 2; h++) {
            const char *side = h ? "right" : "left";
            char p[96];
            #define BIND(act, fmt)                                        \
                snprintf(p, sizeof(p), fmt, side);                        \
                b[nb].action = ha[h].act;                                 \
                b[nb].binding = to_path(inst, p);                         \
                nb++
            BIND(sel,  "/user/hand/%s/input/select/click");
            BIND(menu, "/user/hand/%s/input/menu/click");
            BIND(aim,  "/user/hand/%s/input/aim/pose");
            BIND(grip, "/user/hand/%s/input/grip/pose");
            #undef BIND
        }
        XrInteractionProfileSuggestedBinding sug = {
            XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        sug.interactionProfile =
            to_path(inst, "/interaction_profiles/khr/simple_controller");
        sug.countSuggestedBindings = nb;
        sug.suggestedBindings = b;
        r = pfn_xrSuggestInteractionProfileBindings(inst, &sug);
        LOGI("suggest simple_controller -> %d", r);
    }

    XrSessionActionSetsAttachInfo att = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    att.countActionSets = 1;
    att.actionSets = &aset;
    r = pfn_xrAttachSessionActionSets(sess, &att);
    LOGI("attach -> %d", r);

    for (int h = 0; h < 2; h++) {
        XrActionSpaceCreateInfo aspci = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        aspci.action = ha[h].aim;
        aspci.subactionPath = hand[h];
        aspci.poseInActionSpace.orientation.w = 1.0f;
        pfn_xrCreateActionSpace(sess, &aspci, &ha[h].aim_space);
        aspci.action = ha[h].grip;
        pfn_xrCreateActionSpace(sess, &aspci, &ha[h].grip_space);
    }

    GLuint prog = link_prog(VS, FS);
    GLuint tprog = link_prog(TVS, TFS);
    build_scene();
    init_font();
    GLint aPos = glGetAttribLocation(prog, "aPos");
    GLint aCol = glGetAttribLocation(prog, "aCol");
    GLint uMvp = glGetUniformLocation(prog, "uMvp");
    GLint uTint = glGetUniformLocation(prog, "uTint");
    GLint tPos = glGetAttribLocation(tprog, "aPos");
    GLint tUV = glGetAttribLocation(tprog, "aUV");
    GLint tMvp = glGetUniformLocation(tprog, "uMvp");
    GLint tTex = glGetUniformLocation(tprog, "uTex");
    GLint tCol = glGetUniformLocation(tprog, "uCol");

    XrSessionState state = XR_SESSION_STATE_UNKNOWN;
    bool running = true, session_running = false;
    long frames = 0;
    long fps_frames = 0;
    struct timespec fps_t0 = {0, 0};
    float fps = 0;
    float yaw_t = 0;

    while (running && !app->destroyRequested) {
        int events;
        struct android_poll_source *src;
        while (ALooper_pollOnce(0, NULL, &events, (void **)&src) >= 0)
            if (src) src->process(app, src);

        XrEventDataBuffer ev = {XR_TYPE_EVENT_DATA_BUFFER};
        while (pfn_xrPollEvent(inst, &ev) == 0) {
            if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                XrEventDataSessionStateChanged *ss = (XrEventDataSessionStateChanged *)&ev;
                state = ss->state;
                LOGI("state -> %d", state);
                if (state == XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo bi = {XR_TYPE_SESSION_BEGIN_INFO};
                    bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    r = pfn_xrBeginSession(sess, &bi);
                    LOGI("pfn_xrBeginSession -> %d", r);
                    session_running = XR_SUCCEEDED(r);
                } else if (state == XR_SESSION_STATE_STOPPING) {
                    pfn_xrEndSession(sess);
                    session_running = false;
                } else if (state >= XR_SESSION_STATE_EXITING) {
                    running = false;
                }
            }
            ev.type = XR_TYPE_EVENT_DATA_BUFFER;
            ev.next = NULL;
        }

        // xrWaitFrame drives the session state machine; call it whenever the
        // session is running (READY+), not only once SYNCHRONIZED.
        if (!session_running || state < XR_SESSION_STATE_READY ||
            state >= XR_SESSION_STATE_STOPPING) {
            usleep(30000);
            continue;
        }

        XrFrameState fstate = {XR_TYPE_FRAME_STATE};
        XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
        if (XR_FAILED(pfn_xrWaitFrame(sess, &fwi, &fstate))) {
            usleep(5000);
            continue;
        }
        XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
        pfn_xrBeginFrame(sess, &fbi);

        XrViewLocateInfo vli = {XR_TYPE_VIEW_LOCATE_INFO};
        vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        vli.displayTime = fstate.predictedDisplayTime;
        vli.space = space;
        XrViewState vstate = {XR_TYPE_VIEW_STATE};
        XrView views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
        uint32_t found = 0;
        r = pfn_xrLocateViews(sess, &vli, &vstate, 2, &found, views);

        // sync the action set: this is what runs update_inputs on the
        // controller devices, so button state and poses stay fresh
        XrActiveActionSet active = {aset, XR_NULL_PATH};
        XrActionsSyncInfo sync = {XR_TYPE_ACTIONS_SYNC_INFO};
        sync.countActiveActionSets = 1;
        sync.activeActionSets = &active;
        pfn_xrSyncActions(sess, &sync);

        frames++;
        yaw_t += 0.02f;

        struct timespec mts;
        clock_gettime(CLOCK_MONOTONIC, &mts);
        long long mono = (long long)mts.tv_sec * 1000000000ll + mts.tv_nsec;
        if (fps_t0.tv_sec == 0) fps_t0 = mts;
        fps_frames++;
        if (mts.tv_sec != fps_t0.tv_sec) {
            fps = fps_frames / (float)(mts.tv_sec - fps_t0.tv_sec +
                                       (mts.tv_nsec - fps_t0.tv_nsec) / 1e9f);
            fps_frames = 0;
            fps_t0 = mts;
        }

        // ---- per-frame tracking state ----
        XrSpaceLocation aim_loc[2] = {{XR_TYPE_SPACE_LOCATION},
                                      {XR_TYPE_SPACE_LOCATION}};
        XrSpaceLocation grip_loc[2] = {{XR_TYPE_SPACE_LOCATION},
                                       {XR_TYPE_SPACE_LOCATION}};
        XrActionStateBoolean bs_sel[2], bs_ax[2], bs_by[2], bs_menu[2],
                             bs_trigc[2], bs_sqzc[2], bs_stickc[2];
        XrActionStateFloat fs_trig[2], fs_sqz[2];
        XrActionStateVector2f vs_stick[2];
        memset(bs_sel, 0, sizeof(bs_sel)); memset(bs_ax, 0, sizeof(bs_ax));
        memset(bs_by, 0, sizeof(bs_by)); memset(bs_menu, 0, sizeof(bs_menu));
        memset(bs_trigc, 0, sizeof(bs_trigc)); memset(bs_sqzc, 0, sizeof(bs_sqzc));
        memset(bs_stickc, 0, sizeof(bs_stickc));
        memset(fs_trig, 0, sizeof(fs_trig)); memset(fs_sqz, 0, sizeof(fs_sqz));
        memset(vs_stick, 0, sizeof(vs_stick));

        for (int h = 0; h < 2; h++) {
            XrActionStateGetInfo gi = {XR_TYPE_ACTION_STATE_GET_INFO};
            gi.subactionPath = hand[h];

            #define GETB(field, act)                                     \
                bs_##field[h].type = XR_TYPE_ACTION_STATE_BOOLEAN;       \
                gi.action = ha[h].act;                                   \
                pfn_xrGetActionStateBoolean(sess, &gi, &bs_##field[h])
            #define GETF(dst, act)                                       \
                dst[h].type = XR_TYPE_ACTION_STATE_FLOAT;                \
                gi.action = ha[h].act;                                   \
                pfn_xrGetActionStateFloat(sess, &gi, &dst[h])

            GETB(sel, sel); GETB(ax, ax); GETB(by, by); GETB(menu, menu);
            GETB(trigc, trigc); GETB(sqzc, sqzc); GETB(stickc, stickc);
            GETF(fs_trig, trigv); GETF(fs_sqz, sqzv);
            vs_stick[h].type = XR_TYPE_ACTION_STATE_VECTOR2F;
            gi.action = ha[h].stick;
            pfn_xrGetActionStateVector2f(sess, &gi, &vs_stick[h]);

            pfn_xrLocateSpace(ha[h].aim_space, space,
                              fstate.predictedDisplayTime, &aim_loc[h]);
            pfn_xrLocateSpace(ha[h].grip_space, space,
                              fstate.predictedDisplayTime, &grip_loc[h]);
            #undef GETB
            #undef GETF
        }

        if (frames % 36 == 0 && XR_SUCCEEDED(r)) {
            XrPosef *p = &views[0].pose;
            LOGI("pose t=%lld mono=%lld q=(%.5f %.5f %.5f %.5f) p=(%.5f %.5f %.5f) flags=%llx",
                 (long long)fstate.predictedDisplayTime, mono,
                 p->orientation.x, p->orientation.y, p->orientation.z, p->orientation.w,
                 p->position.x, p->position.y, p->position.z,
                 (unsigned long long)vstate.viewStateFlags);
        }

        // ---- assemble the debug panel ----
        char ln[160];
        char prop_dof[64], prop_axismap[64], prop_ctrlmap[64];
        prop_str("persist.pn2.dof", prop_dof, sizeof(prop_dof));
        prop_str("debug.pn2.axismap", prop_axismap, sizeof(prop_axismap));
        prop_str("debug.pn2.ctrlaxismap", prop_ctrlmap, sizeof(prop_ctrlmap));
        XrSpaceLocationFlags vf = vstate.viewStateFlags;
        double pred_ms = (double)(fstate.predictedDisplayTime - mono) / 1e6;

        text_reset();
        float py = 8;
        #define LINE(...) do { snprintf(ln, sizeof(ln), __VA_ARGS__); \
                               text_str(14, py, ln); py += 42; } while (0)

        LINE("xrtest  %s  rt:%s", sp.systemName,
             rt_bundled ? "bundled" : "system");
        LINE("fps %5.1f   frame %ld   sess %s", fps, frames, sess_state_str(state));
        LINE("predict %+6.1fms  views %u  fmt 0x%llx", pred_ms, found,
             (long long)fmt);
        LINE("viewflags  ori:%s%s  pos:%s%s",
             (vf & XR_VIEW_STATE_ORIENTATION_VALID_BIT) ? "V" : "-",
             (vf & XR_VIEW_STATE_ORIENTATION_TRACKED_BIT) ? "T" : "-",
             (vf & XR_VIEW_STATE_POSITION_VALID_BIT) ? "V" : "-",
             (vf & XR_VIEW_STATE_POSITION_TRACKED_BIT) ? "T" : "-");
        LINE("caps pos=%d ori=%d   dof=%s axismap=%s ctrlmap=%s",
             sp.trackingProperties.positionTracking,
             sp.trackingProperties.orientationTracking,
             prop_dof, prop_axismap, prop_ctrlmap);
        py += 6;
        LINE("head q %+.3f %+.3f %+.3f %+.3f",
             views[0].pose.orientation.x, views[0].pose.orientation.y,
             views[0].pose.orientation.z, views[0].pose.orientation.w);
        LINE("head p %+.3f %+.3f %+.3f",
             views[0].pose.position.x, views[0].pose.position.y,
             views[0].pose.position.z);
        LINE("eye fov  L %+.1f R %+.1f U %+.1f D %+.1f",
             views[0].fov.angleLeft * 57.2958f, views[0].fov.angleRight * 57.2958f,
             views[0].fov.angleUp * 57.2958f, views[0].fov.angleDown * 57.2958f);
        py += 6;
        for (int h = 0; h < 2; h++) {
            XrSpaceLocationFlags af = aim_loc[h].locationFlags;
            XrSpaceLocationFlags gf = grip_loc[h].locationFlags;
            bool tracked = (af & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) ||
                           (af & XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
            LINE("%s %s  aimF=%02llx gripF=%02llx",
                 h ? "ctrl R" : "ctrl L",
                 tracked ? "TRACKED" : "------",
                 (unsigned long long)af, (unsigned long long)gf);
            LINE("  aim p %+.3f %+.3f %+.3f",
                 aim_loc[h].pose.position.x, aim_loc[h].pose.position.y,
                 aim_loc[h].pose.position.z);
            LINE("  aim q %+.3f %+.3f %+.3f %+.3f",
                 aim_loc[h].pose.orientation.x, aim_loc[h].pose.orientation.y,
                 aim_loc[h].pose.orientation.z, aim_loc[h].pose.orientation.w);
            LINE("  grip p %+.3f %+.3f %+.3f",
                 grip_loc[h].pose.position.x, grip_loc[h].pose.position.y,
                 grip_loc[h].pose.position.z);
            LINE("  trig %s%.2f  sqz %s%.2f  pad %+.2f %+.2f%s",
                 bs_trigc[h].currentState ? "!" : "",
                 fs_trig[h].isActive ? fs_trig[h].currentState : 0.f,
                 bs_sqzc[h].currentState ? "!" : "",
                 fs_sqz[h].isActive ? fs_sqz[h].currentState : 0.f,
                 vs_stick[h].currentState.x, vs_stick[h].currentState.y,
                 bs_stickc[h].currentState ? " clk" : "");
            LINE("  btn %s=%d %s=%d menu=%d sel=%d act=%d%d%d%d",
                 h ? "a" : "x", bs_ax[h].currentState,
                 h ? "b" : "y", bs_by[h].currentState,
                 bs_menu[h].currentState, bs_sel[h].currentState,
                 bs_ax[h].isActive, fs_trig[h].isActive,
                 vs_stick[h].isActive, aim_loc[h].locationFlags != 0);
        }
        #undef LINE

        XrCompositionLayerProjectionView pviews[2];
        memset(pviews, 0, sizeof(pviews));
        bool have_views = XR_SUCCEEDED(r) && found >= 2;

        if (fstate.shouldRender && have_views) {
            for (int eye = 0; eye < 2; eye++) {
                XrSwapchainImageAcquireInfo acq = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                uint32_t img_idx = 0;
                pfn_xrAcquireSwapchainImage(sc[eye], &acq, &img_idx);
                XrSwapchainImageWaitInfo wait = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                wait.timeout = 500000000;
                pfn_xrWaitSwapchainImage(sc[eye], &wait);

                GLuint tex = imgs[eye][img_idx].image;
                GLuint fbo;
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, tex, 0);
                glViewport(0, 0, vcv[eye].recommendedImageRectWidth,
                           vcv[eye].recommendedImageRectHeight);
                glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                float proj[16], view[16], mvp[16];
                mat4_proj(views[eye].fov, 0.05f, 100.0f, proj);
                mat4_view_from_pose(views[eye].pose, view);
                mat4_mul(mvp, proj, view);

                glUseProgram(prog);
                glUniformMatrix4fv(uMvp, 1, GL_FALSE, mvp);
                glUniform3f(uTint, 1, 1, 1);
                glEnableVertexAttribArray(aPos);
                glEnableVertexAttribArray(aCol);

                // static scene: grid + marker cubes
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void *)0);
                glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void *)12);
                glDrawElements(GL_LINES, g_lines, GL_UNSIGNED_SHORT, 0);
                glDrawElements(GL_TRIANGLES, g_tris, GL_UNSIGNED_SHORT,
                               (void *)(g_lines * sizeof(uint16_t)));

                // dynamic cube draws share this helper
                glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
                glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void *)0);
                glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void *)12);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                // spinning cube above the panel: smooth spin = healthy frames
                {
                    float mm[16], sm[16], rm[16], tm[16], model[16];
                    mat4_translate(tm, 0, PANEL_Y0 + 0.32f, PANEL_Z);
                    mat4_rot_y(rm, yaw_t);
                    mat4_scale(sm, 0.055f);
                    mat4_mul(mm, tm, rm);
                    mat4_mul(model, mm, sm);
                    mat4_mul(mvp, proj, view);
                    mat4_mul(mvp, mvp, model);
                    glUniformMatrix4fv(uMvp, 1, GL_FALSE, mvp);
                    glUniform3f(uTint, 1.0f, 0.8f, 0.3f);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // controller markers at the aim pose + a forward ray
                for (int h = 0; h < 2; h++) {
                    XrSpaceLocationFlags af = aim_loc[h].locationFlags;
                    if (!(af & XR_SPACE_LOCATION_POSITION_VALID_BIT)) continue;
                    float model[16], cmvp[16], sm[16];
                    mat4_model_from_pose(aim_loc[h].pose, model);
                    mat4_scale(sm, 0.035f);
                    mat4_mul(model, model, sm);
                    mat4_mul(cmvp, proj, view);
                    mat4_mul(cmvp, cmvp, model);
                    glUniformMatrix4fv(uMvp, 1, GL_FALSE, cmvp);
                    bool hot = bs_trigc[h].currentState || bs_sel[h].currentState;
                    if (h) glUniform3f(uTint, hot ? 1.0f : 0.8f, 0.3f, hot ? 0.3f : 0.9f);
                    else   glUniform3f(uTint, 0.3f, hot ? 1.0f : 0.85f, hot ? 0.4f : 1.0f);
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // aim ray, 1.2m along the pose's -Z
                    float dir[3];
                    quat_rot(aim_loc[h].pose.orientation, 0, 0, -1, dir);
                    float lv[12];
                    memcpy(lv, &aim_loc[h].pose.position, 12);
                    lv[3] = h ? 0.9f : 0.3f; lv[4] = 0.4f; lv[5] = h ? 0.9f : 1.0f;
                    lv[6] = aim_loc[h].pose.position.x + dir[0] * 1.2f;
                    lv[7] = aim_loc[h].pose.position.y + dir[1] * 1.2f;
                    lv[8] = aim_loc[h].pose.position.z + dir[2] * 1.2f;
                    lv[9] = lv[3]; lv[10] = lv[4]; lv[11] = lv[5];
                    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(lv), lv, GL_DYNAMIC_DRAW);
                    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void *)0);
                    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void *)12);
                    mat4_mul(cmvp, proj, view);
                    glUniformMatrix4fv(uMvp, 1, GL_FALSE, cmvp);
                    glUniform3f(uTint, 1, 1, 1);
                    glDrawArrays(GL_LINES, 0, 2);
                }

                // debug panel: opaque backing, then baked-glyph text
                glBindBuffer(GL_ARRAY_BUFFER, panel_vbo);
                glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void *)0);
                glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void *)12);
                mat4_mul(mvp, proj, view);
                glUniformMatrix4fv(uMvp, 1, GL_FALSE, mvp);
                glUniform3f(uTint, 1, 1, 1);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                if (g_font_ok && g_textn > 0) {
                    glUseProgram(tprog);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
                    glBufferData(GL_ARRAY_BUFFER, g_textn * 5 * sizeof(float),
                                 g_textv, GL_DYNAMIC_DRAW);
                    glEnableVertexAttribArray(tPos);
                    glEnableVertexAttribArray(tUV);
                    glVertexAttribPointer(tPos, 3, GL_FLOAT, GL_FALSE, 20, (void *)0);
                    glVertexAttribPointer(tUV, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
                    glUniformMatrix4fv(tMvp, 1, GL_FALSE, mvp);
                    glUniform1i(tTex, 0);
                    glUniform4f(tCol, 0.65f, 0.95f, 0.75f, 1.0f);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, g_font_tex);
                    glDrawArrays(GL_TRIANGLES, 0, g_textn);
                    glDisableVertexAttribArray(tPos);
                    glDisableVertexAttribArray(tUV);
                    glDisable(GL_BLEND);
                    glUseProgram(prog);
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &fbo);

                XrSwapchainImageReleaseInfo rel = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                pfn_xrReleaseSwapchainImage(sc[eye], &rel);

                pviews[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                pviews[eye].pose = views[eye].pose;
                pviews[eye].fov = views[eye].fov;
                pviews[eye].subImage.swapchain = sc[eye];
                pviews[eye].subImage.imageRect.extent.width = vcv[eye].recommendedImageRectWidth;
                pviews[eye].subImage.imageRect.extent.height = vcv[eye].recommendedImageRectHeight;
            }
        }

        XrCompositionLayerProjection proj_layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        proj_layer.space = space;
        proj_layer.viewCount = 2;
        proj_layer.views = pviews;
        const XrCompositionLayerBaseHeader *layers[1] =
            {(const XrCompositionLayerBaseHeader *)&proj_layer};

        XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
        fei.displayTime = fstate.predictedDisplayTime;
        fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        if (fstate.shouldRender && have_views) {
            fei.layerCount = 1;
            fei.layers = layers;
        }
        pfn_xrEndFrame(sess, &fei);
    }

    LOGI("exit frames=%ld", frames);

    for (int eye = 0; eye < 2; eye++) {
        if (sc[eye] != XR_NULL_HANDLE)
            pfn_xrDestroySwapchain(sc[eye]);
        free(imgs[eye]);
    }
    for (int h = 0; h < 2; h++) {
        if (ha[h].aim_space != XR_NULL_HANDLE)
            pfn_xrDestroySpace(ha[h].aim_space);
        if (ha[h].grip_space != XR_NULL_HANDLE)
            pfn_xrDestroySpace(ha[h].grip_space);
    }
    if (space != XR_NULL_HANDLE)
        pfn_xrDestroySpace(space);
    if (sess != XR_NULL_HANDLE)
        pfn_xrDestroySession(sess);
    if (inst != XR_NULL_HANDLE) {
        r = pfn_xrDestroyInstance(inst);
        LOGI("xrDestroyInstance -> %d", r);
    }
    (*app->activity->vm)->DetachCurrentThread(app->activity->vm);
    usleep(100000); // let logd flush the destroy lines before we die
    exit(0);
}
