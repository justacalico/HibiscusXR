// Minimal OpenXR test app for the Pico Neo 2 Monado runtime.
// Loads libopenxr_monado.so directly (in-process), renders a simple stereo
// scene with GLES2, logs head pose for verification.

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

static void mat4_mul(float *out, const float *a, const float *b) {
    float r[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[j * 4 + i] = a[i] * b[j * 4] + a[4 + i] * b[j * 4 + 1] +
                           a[8 + i] * b[j * 4 + 2] + a[12 + i] * b[j * 4 + 3];
    memcpy(out, r, sizeof(r));
}

// ---------- gles ----------

static const char *VS =
    "attribute vec3 aPos; attribute vec3 aCol; uniform mat4 uMvp; varying vec3 vCol;"
    "void main(){ vCol=aCol; gl_Position=uMvp*vec4(aPos,1.0);}";
static const char *FS =
    "precision mediump float; varying vec3 vCol; void main(){ gl_FragColor=vec4(vCol,1.0);}";

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

static GLuint vbo, ibo;
static int g_lines, g_tris, idx_count;

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
    idx_count = ni;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, nv * 6 * sizeof(float), v, GL_STATIC_DRAW);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(uint16_t), idx, GL_STATIC_DRAW);
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

    // load the in-process runtime
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

    load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrCreateInstance, "xrCreateInstance");
    load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrEnumerateInstanceExtensionProperties, "xrEnumerateInstanceExtensionProperties");

    // instance
    XrInstanceCreateInfoAndroidKHR andr = {
        XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR, NULL,
        app->activity->vm, app->activity->clazz};
    const char *exts[] = {"XR_KHR_android_create_instance", "XR_KHR_opengl_es_enable"};
    XrInstanceCreateInfo ici = {XR_TYPE_INSTANCE_CREATE_INFO};
    ici.next = &andr;
    strcpy(ici.applicationInfo.applicationName, "xrtest");
    ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    ici.enabledExtensionCount = 2;
    ici.enabledExtensionNames = exts;
    XrInstance inst = XR_NULL_HANDLE;
    XrResult r = pfn_xrCreateInstance(&ici, &inst);
    LOGI("xrCreateInstance -> %d", r);
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

    GLuint prog = glCreateProgram();
    glAttachShader(prog, compile(GL_VERTEX_SHADER, VS));
    glAttachShader(prog, compile(GL_FRAGMENT_SHADER, FS));
    glLinkProgram(prog);
    build_scene();
    GLint aPos = glGetAttribLocation(prog, "aPos");
    GLint aCol = glGetAttribLocation(prog, "aCol");
    GLint uMvp = glGetUniformLocation(prog, "uMvp");

    XrSessionState state = XR_SESSION_STATE_UNKNOWN;
    bool running = true, session_running = false;
    long frames = 0;

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

        frames++;
        if (frames % 36 == 0 && XR_SUCCEEDED(r)) {
            struct timespec mts;
            clock_gettime(CLOCK_MONOTONIC, &mts);
            long long mono = (long long)mts.tv_sec * 1000000000ll + mts.tv_nsec;
            XrPosef *p = &views[0].pose;
            LOGI("pose t=%lld mono=%lld q=(%.5f %.5f %.5f %.5f) p=(%.5f %.5f %.5f) flags=%llx",
                 (long long)fstate.predictedDisplayTime, mono,
                 p->orientation.x, p->orientation.y, p->orientation.z, p->orientation.w,
                 p->position.x, p->position.y, p->position.z,
                 (unsigned long long)vstate.viewStateFlags);
        }

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
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glEnableVertexAttribArray(aPos);
                glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void *)0);
                glEnableVertexAttribArray(aCol);
                glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void *)12);
                glDrawElements(GL_LINES, g_lines, GL_UNSIGNED_SHORT, 0);
                glDrawElements(GL_TRIANGLES, g_tris, GL_UNSIGNED_SHORT,
                               (void *)(g_lines * sizeof(uint16_t)));
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
    (*app->activity->vm)->DetachCurrentThread(app->activity->vm);
}
