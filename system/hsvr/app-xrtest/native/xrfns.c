#include "xrtest.h"

#include <stdio.h>
#include <string.h>
#include <sys/system_properties.h>

PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr_fn;

#define XRP(fn) PFN_##fn pfn_##fn
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

bool load_pfn(XrInstance inst, PFN_xrVoidFunction *out, const char *name) {
    return XR_SUCCEEDED(xrGetInstanceProcAddr_fn(inst, name, out));
}

XrAction mk_action(XrActionSet aset, const char *name, XrActionType type,
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

XrPath to_path(XrInstance inst, const char *s) {
    XrPath p = XR_NULL_PATH;
    pfn_xrStringToPath(inst, s, &p);
    return p;
}

const char *sess_state_str(int s) {
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

void prop_str(const char *key, char *out, int outlen) {
    if (__system_property_get(key, out) <= 0)
        snprintf(out, outlen, "-");
}
