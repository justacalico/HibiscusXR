#include "vr.h"

#include "../common/log.h"
#include "../hud/engine.h"
#include "../input/aim.h"
#include "../input/input.h"
#include "../input/keys.h"

#include <EGL/eglext.h>

#include <cmath>
#include <cstring>
#include <vector>

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

// one EGL context for the session: the XR runtime hands us swapchain
// textures inside it, so the dash's whole draw happens on this context.
// A real display first (EGL_DEFAULT_DISPLAY covers X11 and Wayland), then
// Mesa's surfaceless platform so -vr also runs headless

static bool eglPick(VrDash* v) {
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0;
    if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &major, &minor)) {
        PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)
                eglGetProcAddress("eglGetPlatformDisplayEXT");
        if (!getPlatformDisplay)
            getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
                eglGetProcAddress("eglGetPlatformDisplay");
        if (getPlatformDisplay) {
            dpy = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                     EGL_DEFAULT_DISPLAY, nullptr);
            if (dpy != EGL_NO_DISPLAY)
                eglInitialize(dpy, &major, &minor);
        }
    }
    if (dpy == EGL_NO_DISPLAY || major == 0) {
        LOGE("vr: no usable EGL display");
        return false;
    }
    v->edpy = dpy;
    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint cfgAttr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };
    EGLConfig cfgs[16];
    EGLint n = 0;
    eglChooseConfig(dpy, cfgAttr, cfgs, 16, &n);
    v->ecfg = n > 0 ? cfgs[0] : EGL_NO_CONFIG_KHR;
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    v->ectx = eglCreateContext(dpy, v->ecfg, EGL_NO_CONTEXT, ctxAttr);
    if (v->ectx == EGL_NO_CONTEXT) {
        LOGE("vr: es2 context failed");
        return false;
    }
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, v->ectx))
        return true;
    const EGLint pbAttr[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    v->esurf = eglCreatePbufferSurface(dpy, v->ecfg, pbAttr);
    v->pbuffer = v->esurf != EGL_NO_SURFACE;
    if (!v->pbuffer ||
            !eglMakeCurrent(dpy, v->esurf, v->esurf, v->ectx)) {
        LOGE("vr: no surfaceless context or pbuffer");
        return false;
    }
    return true;
}

static bool xrFail(XrResult r, const char* what) {
    if (XR_SUCCEEDED(r)) return false;
    LOGE("vr: %s failed (%d)", what, (int)r);
    return true;
}

static bool xrHasExt(const char* name) {
    uint32_t n = 0;
    if (xrEnumerateInstanceExtensionProperties(nullptr, 0, &n, nullptr) !=
            XR_SUCCESS)
        return false;
    std::vector<XrExtensionProperties> props(n,
        XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES});
    if (xrEnumerateInstanceExtensionProperties(nullptr, n, &n,
            props.data()) != XR_SUCCESS)
        return false;
    for (const auto& p : props)
        if (!strcmp(p.extensionName, name)) return true;
    return false;
}

static XrPath toPath(XrInstance inst, const char* s) {
    XrPath p = XR_NULL_PATH;
    xrStringToPath(inst, s, &p);
    return p;
}

struct VrBinds {
    const char* profile;
    const char* sel[2], *sq[2], *aux[2], *menu[2], *stick[2];
};

// per-profile path suggestions. select is the confirm click, squeeze the
// secondary click, aux the back/park key and menu the recenter key;
// anything a profile lacks is left unbound
static const VrBinds kBinds[] = {
    {"/interaction_profiles/khr/simple_controller",
        {"/user/hand/left/input/select/click",
         "/user/hand/right/input/select/click"},
        {nullptr, nullptr},
        {nullptr, nullptr},
        {"/user/hand/left/input/menu/click",
         "/user/hand/right/input/menu/click"},
        {nullptr, nullptr}},
    {"/interaction_profiles/oculus/touch_controller",
        {"/user/hand/left/input/trigger",
         "/user/hand/right/input/trigger"},
        {"/user/hand/left/input/squeeze",
         "/user/hand/right/input/squeeze"},
        {"/user/hand/left/input/x/click",
         "/user/hand/right/input/a/click"},
        {"/user/hand/left/input/menu/click",
         "/user/hand/right/input/b/click"},
        {"/user/hand/left/input/thumbstick",
         "/user/hand/right/input/thumbstick"}},
    {"/interaction_profiles/valve/index_controller",
        {"/user/hand/left/input/trigger",
         "/user/hand/right/input/trigger"},
        {"/user/hand/left/input/squeeze",
         "/user/hand/right/input/squeeze"},
        {"/user/hand/left/input/a/click",
         "/user/hand/right/input/a/click"},
        {"/user/hand/left/input/b/click",
         "/user/hand/right/input/b/click"},
        {"/user/hand/left/input/thumbstick",
         "/user/hand/right/input/thumbstick"}},
    {"/interaction_profiles/htc/vive_controller",
        {"/user/hand/left/input/trigger",
         "/user/hand/right/input/trigger"},
        {"/user/hand/left/input/squeeze",
         "/user/hand/right/input/squeeze"},
        {"/user/hand/left/input/trackpad/click",
         "/user/hand/right/input/trackpad/click"},
        {"/user/hand/left/input/menu/click",
         "/user/hand/right/input/menu/click"},
        {"/user/hand/left/input/trackpad",
         "/user/hand/right/input/trackpad"}},
    {"/interaction_profiles/microsoft/motion_controller",
        {"/user/hand/left/input/trigger",
         "/user/hand/right/input/trigger"},
        {"/user/hand/left/input/squeeze",
         "/user/hand/right/input/squeeze"},
        {"/user/hand/left/input/thumbstick/click",
         "/user/hand/right/input/thumbstick/click"},
        {"/user/hand/left/input/menu/click",
         "/user/hand/right/input/menu/click"},
        {"/user/hand/left/input/thumbstick",
         "/user/hand/right/input/thumbstick"}},
};

static bool xrActionSetup(VrDash* v) {
    const XrPath hands[2] = {
        toPath(v->inst, "/user/hand/left"),
        toPath(v->inst, "/user/hand/right"),
    };
    v->handL = hands[0];
    v->handR = hands[1];
    XrActionSetCreateInfo asi{XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(asi.actionSetName, "dash");
    strcpy(asi.localizedActionSetName, "dash");
    if (xrFail(xrCreateActionSet(v->inst, &asi, &v->aset), "action set"))
        return false;

    auto mkAction = [&](XrAction* out, const char* name,
                        XrActionType type) {
        XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};
        ai.actionType = type;
        strcpy(ai.actionName, name);
        strcpy(ai.localizedActionName, name);
        ai.countSubactionPaths = 2;
        ai.subactionPaths = hands;
        return xrCreateAction(v->aset, &ai, out) == XR_SUCCESS;
    };
    if (!mkAction(&v->aAim, "aim", XR_ACTION_TYPE_POSE_INPUT) ||
        !mkAction(&v->aSel, "select", XR_ACTION_TYPE_BOOLEAN_INPUT) ||
        !mkAction(&v->aSqueeze, "squeeze", XR_ACTION_TYPE_BOOLEAN_INPUT) ||
        !mkAction(&v->aAux, "aux", XR_ACTION_TYPE_BOOLEAN_INPUT) ||
        !mkAction(&v->aMenu, "menu", XR_ACTION_TYPE_BOOLEAN_INPUT) ||
        !mkAction(&v->aStick, "stick", XR_ACTION_TYPE_VECTOR2F_INPUT)) {
        LOGE("vr: action create failed");
        return false;
    }

    const XrAction acts[] = {v->aSel, v->aSqueeze, v->aAux, v->aMenu,
                             v->aStick};
    for (const VrBinds& b : kBinds) {
        std::vector<XrActionSuggestedBinding> binds;
        for (int h = 0; h < 2; ++h) {
            const char* paths[] = {b.sel[h], b.sq[h], b.aux[h],
                                   b.menu[h], b.stick[h]};
            for (int a = 0; a < 5; ++a) {
                if (!paths[a]) continue;
                XrPath full;
                if (xrStringToPath(v->inst, paths[a], &full) != XR_SUCCESS)
                    continue;
                binds.push_back({acts[a], full});
            }
            XrPath aimPath;
            if (xrStringToPath(v->inst,
                    h == 0 ? "/user/hand/left/input/aim/pose"
                           : "/user/hand/right/input/aim/pose",
                    &aimPath) == XR_SUCCESS)
                binds.push_back({v->aAim, aimPath});
        }
        if (binds.empty()) continue;
        XrInteractionProfileSuggestedBinding sug{
            XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        sug.interactionProfile = toPath(v->inst, b.profile);
        sug.countSuggestedBindings = (uint32_t)binds.size();
        sug.suggestedBindings = binds.data();
        xrSuggestInteractionProfileBindings(v->inst, &sug);
    }

    for (int h = 0; h < 2; ++h) {
        XrActionSpaceCreateInfo ci{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        ci.action = v->aAim;
        ci.subactionPath = hands[h];
        ci.poseInActionSpace.orientation.w = 1.0f;
        if (xrFail(xrCreateActionSpace(v->sess, &ci, &v->aimSpace[h]),
                   "aim space"))
            return false;
    }

    XrSessionActionSetsAttachInfo att{
        XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    att.countActionSets = 1;
    att.actionSets = &v->aset;
    return !xrFail(xrAttachSessionActionSets(v->sess, &att),
                   "attach action sets");
}

static bool xrSwapchain(VrDash* v) {
    uint32_t nfmt = 0;
    if (xrFail(xrEnumerateSwapchainFormats(v->sess, 0, &nfmt, nullptr),
               "swapchain formats"))
        return false;
    std::vector<int64_t> fmts(nfmt);
    if (xrFail(xrEnumerateSwapchainFormats(v->sess, nfmt, &nfmt,
                                           fmts.data()),
               "swapchain formats"))
        return false;
    int64_t fmt = nfmt ? fmts[0] : 0;
    for (int64_t want : {(int64_t)GL_SRGB8_ALPHA8, (int64_t)GL_RGBA8})
        for (int64_t f : fmts)
            if (f == want) { fmt = f; goto picked; }
picked:
    XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    ci.format = fmt;
    ci.sampleCount = 1;
    ci.width = kVrTexW;
    ci.height = kVrTexH;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (xrFail(xrCreateSwapchain(v->sess, &ci, &v->swap),
               "create swapchain"))
        return false;
    v->texW = kVrTexW;
    v->texH = kVrTexH;

    uint32_t n = 0;
    LOGI("vr: swapchain fmt %lld", (long long)fmt);
    if (xrFail(xrEnumerateSwapchainImages(v->swap, 0, &n, nullptr),
               "swapchain images"))
        return false;
    if (n > sizeof(v->fbo) / sizeof(v->fbo[0])) n =
        sizeof(v->fbo) / sizeof(v->fbo[0]);
    XrSwapchainImageOpenGLESKHR imgs[8];
    for (uint32_t i = 0; i < n; ++i)
        imgs[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
    uint32_t got = 0;
    if (xrFail(xrEnumerateSwapchainImages(v->swap, n, &got,
            (XrSwapchainImageBaseHeader*)imgs),
               "swapchain images"))
        return false;
    v->nimg = got;
    for (uint32_t i = 0; i < got; ++i) {
        glGenFramebuffers(1, &v->fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, v->fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, imgs[i].image, 0);
        glGenRenderbuffers(1, &v->depth[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, v->depth[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                              v->texW, v->texH);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, v->depth[i]);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
                GL_FRAMEBUFFER_COMPLETE) {
            LOGE("vr: swapchain fbo %u incomplete", i);
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool vrInit(VrDash* v, const char* appName) {
    // Monado's GLES binding on desktop is its own XR_MNDX_egl_enable; the
    // KHR opengl_es_enable binding is only honoured by runtimes that ship
    // it, so prefer whichever the runtime advertises
    const bool mndx = xrHasExt(XR_MNDX_EGL_ENABLE_EXTENSION_NAME);
    const bool khrGles = xrHasExt(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
    if (!mndx && !khrGles) {
        LOGE("vr: runtime lacks " XR_MNDX_EGL_ENABLE_EXTENSION_NAME
             " and " XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
        return false;
    }
    if (!xrHasExt(XR_EXTX_OVERLAY_EXTENSION_NAME)) {
        LOGE("vr: runtime lacks " XR_EXTX_OVERLAY_EXTENSION_NAME
             " (works on WiVRn/Monado; SteamVR has no OpenXR overlay)");
        return false;
    }
    if (!eglPick(v)) return false;

    const char* exts[3];
    int nexts = 0;
    if (mndx) exts[nexts++] = XR_MNDX_EGL_ENABLE_EXTENSION_NAME;
    if (khrGles) exts[nexts++] = XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
    exts[nexts++] = XR_EXTX_OVERLAY_EXTENSION_NAME;
    XrInstanceCreateInfo ici{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(ici.applicationInfo.applicationName, appName);
    strcpy(ici.applicationInfo.engineName, appName);
    ici.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 32);
    ici.enabledExtensionCount = (uint32_t)nexts;
    ici.enabledExtensionNames = exts;
    if (xrFail(xrCreateInstance(&ici, &v->inst), "create instance"))
        return false;

    XrSystemGetInfo sgi{XR_TYPE_SYSTEM_GET_INFO};
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (xrFail(xrGetSystem(v->inst, &sgi, &v->sys), "get system"))
        return false;

    // Monado rejects session create until the app has asked for the GLES
    // requirements, even when it doesn't need the answer
    PFN_xrGetOpenGLESGraphicsRequirementsKHR getReq = nullptr;
    if (khrGles &&
        xrGetInstanceProcAddr(v->inst,
            "xrGetOpenGLESGraphicsRequirementsKHR",
            (PFN_xrVoidFunction*)&getReq) == XR_SUCCESS && getReq) {
        XrGraphicsRequirementsOpenGLESKHR req{
            XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        getReq(v->inst, v->sys, &req);
    }

    XrSessionCreateInfoOverlayEXTX overlay{
        XR_TYPE_SESSION_CREATE_INFO_OVERLAY_EXTX};
    overlay.sessionLayersPlacement = 5;
    // the MNDX binding carries a getProcAddress on top of the same
    // display/config/context triple the KHR binding uses
    XrGraphicsBindingEGLMNDX mndxBind{XR_TYPE_GRAPHICS_BINDING_EGL_MNDX,
        &overlay, (PFN_xrEglGetProcAddressMNDX)eglGetProcAddress,
        v->edpy, v->ecfg, v->ectx};
    // openxr 1.0 headers only name the GLES binding's android variant;
    // newer headers renamed it ..._XR_KHR with the same numeric type and
    // an identical layout, so the plain struct is declared here
    struct {
        XrStructureType type;
        const void* next;
        EGLDisplay display;
        EGLConfig config;
        EGLContext context;
    } khrBind{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
              &overlay, v->edpy, v->ecfg, v->ectx};
    XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
    sci.next = mndx ? (const void*)&mndxBind : (const void*)&khrBind;
    sci.systemId = v->sys;
    if (xrFail(xrCreateSession(v->inst, &sci, &v->sess),
               "create overlay session"))
        return false;

    XrReferenceSpaceCreateInfo rs{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    rs.poseInReferenceSpace.orientation.w = 1.0f;
    if (xrCreateReferenceSpace(v->sess, &rs, &v->stage) != XR_SUCCESS) {
        rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        if (xrFail(xrCreateReferenceSpace(v->sess, &rs, &v->stage),
                   "reference space"))
            return false;
    }
    rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    if (xrFail(xrCreateReferenceSpace(v->sess, &rs, &v->view),
               "view space"))
        return false;

    uint32_t nmode = 0;
    if (xrEnumerateEnvironmentBlendModes(v->inst, v->sys,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &nmode,
            nullptr) == XR_SUCCESS && nmode) {
        std::vector<XrEnvironmentBlendMode> modes(nmode);
        xrEnumerateEnvironmentBlendModes(v->inst, v->sys,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, nmode, &nmode,
            modes.data());
        v->blend = modes[0];
        for (XrEnvironmentBlendMode m : modes)
            if (m == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) v->blend = m;
    }

    if (!xrActionSetup(v)) return false;
    if (!xrSwapchain(v)) return false;
    return true;
}

int vrPoll(VrDash* v) {
    XrEventDataBuffer ev{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(v->inst, &ev) == XR_SUCCESS) {
        switch (ev.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            const auto* e = (XrEventDataSessionStateChanged*)&ev;
            LOGI("vr: session state %d", (int)e->state);
            switch (e->state) {
            case XR_SESSION_STATE_READY: {
                XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};
                bi.primaryViewConfigurationType =
                    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                if (xrFail(xrBeginSession(v->sess, &bi),
                           "begin session") == false)
                    v->running = true;
                break;
            }
            case XR_SESSION_STATE_STOPPING:
                v->running = false;
                xrEndSession(v->sess);
                break;
            case XR_SESSION_STATE_EXITING:
            case XR_SESSION_STATE_LOSS_PENDING:
                v->quit = true;
                break;
            default:
                break;
            }
            break;
        }
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            v->quit = true;
            break;
        case XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX: {
            const auto* e =
                (XrEventDataMainSessionVisibilityChangedEXTX*)&ev;
            LOGI("vr: main session %s", e->visible ? "visible" : "hidden");
            break;
        }
        default:
            break;
        }
        ev = XrEventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};
    }
    if (v->quit) return -1;
    return v->running ? 1 : 0;
}

static VrPose locPose(XrSpace space, XrSpace base, XrTime t) {
    VrPose out;
    XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
    if (xrLocateSpace(space, base, t, &loc) != XR_SUCCESS) return out;
    const XrSpaceLocationFlags need =
        XR_SPACE_LOCATION_POSITION_VALID_BIT |
        XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if ((loc.locationFlags & need) != need) return out;
    const XrPosef& p = loc.pose;
    out.q[0] = p.orientation.x;
    out.q[1] = p.orientation.y;
    out.q[2] = p.orientation.z;
    out.q[3] = p.orientation.w;
    out.p[0] = p.position.x;
    out.p[1] = p.position.y;
    out.p[2] = p.position.z;
    out.ok = true;
    return out;
}

bool vrBeginFrame(VrDash* v) {
    XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};
    v->fs = XrFrameState{XR_TYPE_FRAME_STATE};
    if (xrFail(xrWaitFrame(v->sess, &wi, &v->fs), "wait frame"))
        return false;
    v->predTime = v->fs.predictedDisplayTime;
    v->head = locPose(v->view, v->stage, v->predTime);
    XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};
    xrFail(xrBeginFrame(v->sess, &bi), "begin frame");
    if (!v->placed) {
        LOGI("vr: head %s %.2f %.2f %.2f", v->head.ok ? "ok" : "bad",
             v->head.p[0], v->head.p[1], v->head.p[2]);
        vrRecenter(v);
    }
    return v->fs.shouldRender;
}

static bool boolState(XrSession sess, XrAction a, XrPath hand) {
    XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};
    gi.action = a;
    gi.subactionPath = hand;
    XrActionStateBoolean st{XR_TYPE_ACTION_STATE_BOOLEAN};
    xrGetActionStateBoolean(sess, &gi, &st);
    return st.isActive && st.currentState;
}

void vrInput(VrDash* v, HudEngine* e) {
    XrActionsSyncInfo si{XR_TYPE_ACTIONS_SYNC_INFO};
    si.countActiveActionSets = 1;
    XrActiveActionSet act{v->aset, XR_NULL_PATH};
    si.activeActionSets = &act;
    xrSyncActions(v->sess, &si);

    const XrPath hands[2] = {v->handL, v->handR};
    VrInput cur;
    VrPose aimPose[2];
    for (int w = 0; w < 2; ++w) {
        XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};
        gi.action = v->aAim;
        gi.subactionPath = hands[w];
        XrActionStatePose st{XR_TYPE_ACTION_STATE_POSE};
        xrGetActionStatePose(v->sess, &gi, &st);
        if (st.isActive) {
            aimPose[w] = locPose(v->aimSpace[w], v->stage, v->predTime);
            cur.aim[w] = aimPose[w].ok;
        }
        cur.sel[w] = boolState(v->sess, v->aSel, hands[w]);
        cur.sq[w] = boolState(v->sess, v->aSqueeze, hands[w]);
        cur.aux[w] = boolState(v->sess, v->aAux, hands[w]);
        cur.menu[w] = boolState(v->sess, v->aMenu, hands[w]);
        gi = XrActionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        gi.action = v->aStick;
        gi.subactionPath = hands[w];
        XrActionStateVector2f vs{XR_TYPE_ACTION_STATE_VECTOR2F};
        xrGetActionStateVector2f(v->sess, &gi, &vs);
        if (vs.isActive) {
            cur.stick[w][0] = vs.currentState.x;
            cur.stick[w][1] = vs.currentState.y;
        }
    }
    const VrEvents ev = vrInputTick(v->prev, cur, e->input.active);
    v->prev = cur;
    e->input.active = ev.active;
    for (int i = 0; i < ev.nKeys; ++i) e->ctrlEv.push_back(ev.keys[i]);
    if (ev.scroll != 0.0f) hudScroll(e, ev.scroll);

    // the pointer: whichever controller owns input, else the head; its
    // stage-space ray lands on the presentation quad and the hit uv
    // unprojects into the dash scene
    memset(e->eyePos, 0, sizeof(e->eyePos));
    const VrPose* src = nullptr;
    if (ev.active >= 0 && aimPose[ev.active].ok)
        src = &aimPose[ev.active];
    else if (v->head.ok)
        src = &v->head;
    float ro[3], rd[3], u = 0.5f, hv = 0.5f, t = 0.0f;
    bool hit = false;
    if (src) {
        vrRay(*src, ro, rd);
        hit = vrQuadHit(v->quad, ro, rd, &u, &hv, &t);
    }
    if (hit) {
        vrUvRay(u, hv, kVrFovY, (float)v->texW / v->texH,
                e->aimO, e->aimD);
        if (ev.active >= 0) {
            memcpy(e->ctrlPos[ev.active], e->eyePos,
                   sizeof(e->ctrlPos[0]));
            memcpy(e->ctrlDir[ev.active], e->aimD,
                   sizeof(e->ctrlDir[0]));
        }
    } else {
        memset(e->aimO, 0, sizeof(e->aimO));
        // off the quad: park the ray where nothing can be picked and drop
        // the beam for the frame - a miss isn't a pointer
        e->aimD[0] = 0.0f;
        e->aimD[1] = -1.0f;
        e->aimD[2] = 0.0f;
        e->input.active = -1;
    }
    float ay;
    if (dirYaw(e->aimD, &ay)) e->aimYaw = ay;
    e->aimPitch = dirPitch(e->aimD);
}

void vrRecenter(VrDash* v) {
    VrPose head = v->head;
    if (!head.ok) {
        // tracking may not feed an overlay session yet - fall back to a
        // standing pose so the quad still shows instead of never placing
        head.ok = true;
        head.p[1] = 1.6f;
    }
    vrQuadPlace(head, kVrQuadDist, kVrQuadW, kVrQuadH, &v->quad);
    v->placed = true;
}

bool vrTarget(VrDash* v) {
    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (xrAcquireSwapchainImage(v->swap, &ai, &idx) != XR_SUCCESS)
        return false;
    XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi.timeout = XR_INFINITE_DURATION;
    if (xrWaitSwapchainImage(v->swap, &wi) != XR_SUCCESS)
        return false;
    v->img = (int)idx;
    v->drawn = true;
    glBindFramebuffer(GL_FRAMEBUFFER, v->fbo[idx]);
    glViewport(0, 0, v->texW, v->texH);
    return true;
}

void vrEnd(VrDash* v) {
    if (v->img >= 0) {
        glFlush();
        XrSwapchainImageReleaseInfo ri{
            XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        xrReleaseSwapchainImage(v->swap, &ri);
        v->img = -1;
    }
    XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    XrSwapchainSubImage sub{0};
    const XrCompositionLayerBaseHeader* layer = nullptr;
    // the quad keeps its last released image on frames we didn't draw:
    // dropping the layer whenever shouldRender flickers would blink it
    if (v->drawn && v->placed) {
        sub.swapchain = v->swap;
        sub.imageRect.offset = {0, 0};
        sub.imageRect.extent = {v->texW, v->texH};
        sub.imageArrayIndex = 0;
        quad.layerFlags =
            XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
            XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
        quad.space = v->stage;
        quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        quad.subImage = sub;
        quad.pose.orientation.x = 0.0f;
        quad.pose.orientation.y = sinf(v->quad.yaw * 0.5f);
        quad.pose.orientation.z = 0.0f;
        quad.pose.orientation.w = cosf(v->quad.yaw * 0.5f);
        quad.pose.position.x = v->quad.p[0];
        quad.pose.position.y = v->quad.p[1];
        quad.pose.position.z = v->quad.p[2];
        quad.size.width = v->quad.w;
        quad.size.height = v->quad.h;
        layer = (const XrCompositionLayerBaseHeader*)&quad;
    }
    XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};
    ei.displayTime = v->predTime;
    ei.environmentBlendMode = v->blend;
    ei.layers = layer ? &layer : nullptr;
    ei.layerCount = layer ? 1 : 0;
    if (layer && !v->presented) {
        v->presented = true;
        LOGI("vr: quad live at %.2f %.2f %.2f yaw %.2f",
             v->quad.p[0], v->quad.p[1], v->quad.p[2], v->quad.yaw);
    }
    xrFail(xrEndFrame(v->sess, &ei), "end frame");
}

void vrShutdown(VrDash* v) {
    for (uint32_t i = 0; i < v->nimg; ++i) {
        if (v->fbo[i]) glDeleteFramebuffers(1, &v->fbo[i]);
        if (v->depth[i]) glDeleteRenderbuffers(1, &v->depth[i]);
    }
    if (v->swap != XR_NULL_HANDLE) xrDestroySwapchain(v->swap);
    for (int h = 0; h < 2; ++h)
        if (v->aimSpace[h] != XR_NULL_HANDLE)
            xrDestroySpace(v->aimSpace[h]);
    if (v->stage != XR_NULL_HANDLE) xrDestroySpace(v->stage);
    if (v->view != XR_NULL_HANDLE) xrDestroySpace(v->view);
    if (v->aset != XR_NULL_HANDLE) xrDestroyActionSet(v->aset);
    if (v->sess != XR_NULL_HANDLE) xrDestroySession(v->sess);
    if (v->inst != XR_NULL_HANDLE) xrDestroyInstance(v->inst);
    if (v->edpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(v->edpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        if (v->esurf != EGL_NO_SURFACE)
            eglDestroySurface(v->edpy, v->esurf);
        if (v->ectx != EGL_NO_CONTEXT)
            eglDestroyContext(v->edpy, v->ectx);
        eglTerminate(v->edpy);
    }
}
