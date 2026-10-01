#include "xrtest.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------- runtime loading ----------

// the bundled runtime's loader/runtime negotiation; shared by the
// loader-miss path and the "system runtime unusable" retry
static bool negotiate_bundled(void) {
    void *rt = dlopen("libopenxr_monado.so", RTLD_NOW | RTLD_GLOBAL);
    if (!rt) { LOGE("dlopen: %s", dlerror()); return false; }
    PFN_xrNegotiateLoaderRuntimeInterface negotiate =
        (PFN_xrNegotiateLoaderRuntimeInterface)dlsym(rt, "xrNegotiateLoaderRuntimeInterface");
    if (!negotiate) { LOGE("no negotiate"); return false; }

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
    if (XR_FAILED(nr) || !rr.getInstanceProcAddr) return false;
    xrGetInstanceProcAddr_fn = rr.getInstanceProcAddr;
    return true;
}

// Stock Khronos loader first: it discovers the system runtime
// (MonadoOpenXR) through /system/etc/openxr/1/active_runtime.json or the
// OpenXRRuntimeService package - the same shape apps use on Quest, where
// no runtime-specific code lives in the app. If the loader or the system
// runtime is missing, fall back to the bundled libopenxr_monado.so.
bool xr_open_runtime(struct android_app *app, bool *bundled) {
    *bundled = false;
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
        *bundled = true;
        if (!negotiate_bundled()) return false;
    }
    LOGI("runtime path: %s", *bundled ? "bundled" : "system");
    return true;
}

XrResult xr_open_instance(struct android_app *app, bool *bundled,
                          XrInstance *out) {
    *out = XR_NULL_HANDLE;
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
    XrResult r = pfn_xrCreateInstance(&ici, out);
    LOGI("xrCreateInstance -> %d (bd=%d)", r, have_bd);
    if (XR_FAILED(r) && !*bundled) {
        // loader is present but found no usable system runtime - try the
        // bundled copy once before giving up
        LOGE("system runtime unusable (%d), falling back to bundled", r);
        if (negotiate_bundled()) {
            *bundled = true;
            load_pfn(XR_NULL_HANDLE, (PFN_xrVoidFunction *)&pfn_xrCreateInstance, "xrCreateInstance");
            r = pfn_xrCreateInstance(&ici, out);
            LOGI("xrCreateInstance (bundled) -> %d", r);
        }
    }
    return r;
}

void xr_load_pfns(XrInstance inst) {
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
}

// ---------- swapchains ----------

bool xr_make_swapchains(XrSession sess, const XrViewConfigurationView vcv[2],
                        XrSwapchain sc[2],
                        XrSwapchainImageOpenGLESKHR *imgs[2],
                        uint32_t img_count[2], int64_t *fmt_out) {
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
    *fmt_out = fmt;

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
        XrResult r = pfn_xrCreateSwapchain(sess, &scci, &sc[eye]);
        LOGI("swapchain eye%d -> %d (%ux%u)", eye, r, scci.width, scci.height);
        if (XR_FAILED(r)) return false;
        pfn_xrEnumerateSwapchainImages(sc[eye], 0, &img_count[eye], NULL);
        imgs[eye] = malloc(sizeof(XrSwapchainImageOpenGLESKHR) * img_count[eye]);
        for (uint32_t i = 0; i < img_count[eye]; i++)
            imgs[eye][i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        pfn_xrEnumerateSwapchainImages(sc[eye], img_count[eye], &img_count[eye],
                                   (XrSwapchainImageBaseHeader *)imgs[eye]);
    }
    return true;
}

// ---------- actions ----------

// the whole pico_neo3 surface plus a simple_controller fallback.
// xrSyncActions is what drives update_inputs on the controller devices.
void xr_setup_actions(XrInstance inst, XrSession sess, struct HandAct ha[2],
                      XrPath hand[2], XrActionSet *aset_out) {
    hand[0] = to_path(inst, "/user/hand/left");
    hand[1] = to_path(inst, "/user/hand/right");

    XrActionSet aset = XR_NULL_HANDLE;
    XrActionSetCreateInfo asci = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy(asci.actionSetName, "test");
    strcpy(asci.localizedActionSetName, "test");
    XrResult r = pfn_xrCreateActionSet(inst, &asci, &aset);
    LOGI("actionset -> %d", r);

    memset(ha, 0, sizeof(struct HandAct) * 2);
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

    *aset_out = aset;
}
