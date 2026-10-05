#ifndef XRTEST_H
#define XRTEST_H

// Minimal OpenXR test app for the Pico Neo 2 Monado runtime: shared
// declarations between main.c (EGL + frame loop), setup.c (runtime,
// instance, swapchains, actions), xrfns.c (dynamically-loaded entry
// points), render.c (shaders, font, scene geometry) and math3d.c.

#include <android/log.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>

#include <stdbool.h>
#include <stdint.h>

#define LOG_TAG "xrtest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ---------- entry points (xrfns.c) ----------

extern PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr_fn;
extern bool g_have_depth_ext;

#define XRP(fn) extern PFN_##fn pfn_##fn
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

bool load_pfn(XrInstance inst, PFN_xrVoidFunction *out, const char *name);
XrAction mk_action(XrActionSet aset, const char *name, XrActionType type,
                   XrPath sub);
XrPath to_path(XrInstance inst, const char *s);
const char *sess_state_str(int s);
void prop_str(const char *key, char *out, int outlen);

// ---------- setup.c ----------

struct HandAct {
    XrAction sel, menu, ax, by, trigc, sqzc, stickc;
    XrAction trigv, sqzv, stick;
    XrAction aim, grip;
    XrSpace aim_space, grip_space;
};

// stock Khronos loader first, bundled libopenxr_monado.so on failure;
// *bundled reports which path won
bool xr_open_runtime(struct android_app *app, bool *bundled);
// instance create with the same bundled fallback; returns the XrResult of
// the last attempt
XrResult xr_open_instance(struct android_app *app, bool *bundled,
                          XrInstance *out);
// instance-level function pointers resolve against the real instance
void xr_load_pfns(XrInstance inst);
// the whole pico_neo3 action surface plus a simple_controller fallback,
// spaces attached to sess; hand_out and aset_out feed the frame loop's
// per-hand state reads and the action sync
void xr_setup_actions(XrInstance inst, XrSession sess, struct HandAct ha[2],
                      XrPath hand[2], XrActionSet *aset_out);
// one swapchain per eye in the preferred sRGB-ish format; *fmt_out takes
// the chosen format for the debug line. if depth_ext_ok is true a matching
// depth swapchain is also created per eye (for XR_KHR_composition_layer_depth).
bool xr_make_swapchains(XrSession sess, const XrViewConfigurationView vcv[2],
                        XrSwapchain sc[2],
                        XrSwapchainImageOpenGLESKHR *imgs[2],
                        uint32_t img_count[2], int64_t *fmt_out,
                        bool depth_ext_ok,
                        XrSwapchain depth_sc[2],
                        XrSwapchainImageOpenGLESKHR *depth_imgs[2],
                        uint32_t depth_img_count[2], int64_t *depth_fmt_out);

// ---------- math3d.c ----------

void mat4_proj(XrFovf fov, float zn, float zf, float *m);
void mat4_view_from_pose(XrPosef p, float *m);
void mat4_model_from_pose(XrPosef p, float *m);
void mat4_mul(float *out, const float *a, const float *b);
void mat4_scale(float *m, float s);
void mat4_rot_y(float *m, float rad);
void mat4_translate(float *m, float x, float y, float z);
void quat_rot(XrQuaternionf q, float vx, float vy, float vz, float *out);

// ---------- render.c ----------

// world-locked debug panel straight ahead; px space mapped through kMPX
#define PANEL_Z   (-1.7f)
#define PANEL_CX  0.0f
#define PANEL_CY  0.25f
#define PANEL_W   1.54f
#define PANEL_H   0.97f
#define kMPX      (PANEL_W / 1400.0f)   // metres per panel px
#define PANEL_X0  (PANEL_CX - PANEL_W / 2)
#define PANEL_Y0  (PANEL_CY + PANEL_H / 2)

#define MAX_TEXT_CHARS 4096

extern GLuint vbo, ibo, cube_vbo, panel_vbo, line_vbo, text_vbo;
extern int g_lines, g_tris;
extern GLuint g_font_tex;
extern bool g_font_ok;
extern float g_textv[];
extern int g_textn;

GLuint link_color_prog(void);
GLuint link_text_prog(void);
void build_scene(void);
void init_font(void);
void text_reset(void);
void text_str(float px, float py, const char *s);

#endif
