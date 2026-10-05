// Minimal OpenXR test app for the Pico Neo 2 Monado runtime.
// Loads libopenxr_monado.so directly (in-process), renders a simple stereo
// scene with GLES2, and draws a world-locked debug panel with live tracking
// state: head pose, view flags, per-controller poses/buttons/stick, fps.

#include "xrtest.h"

#include <GLES2/gl2ext.h>

// packed depth-stencil attachment enum isn't exposed by the GLES2 headers
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif

#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

    bool rt_bundled = false;
    if (!xr_open_runtime(app, &rt_bundled)) return;

    XrInstance inst = XR_NULL_HANDLE;
    XrResult r = xr_open_instance(app, &rt_bundled, &inst);
    if (XR_FAILED(r)) return;

    // instance-level functions resolve against the real instance
    xr_load_pfns(inst);

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

    XrSwapchain sc[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSwapchainImageOpenGLESKHR *imgs[2] = {NULL, NULL};
    uint32_t img_count[2] = {0, 0};
    int64_t fmt = 0;
    XrSwapchain depth_sc[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSwapchainImageOpenGLESKHR *depth_imgs[2] = {NULL, NULL};
    uint32_t depth_img_count[2] = {0, 0};
    int64_t depth_fmt = 0;
    if (!xr_make_swapchains(sess, vcv, sc, imgs, img_count, &fmt,
                            g_have_depth_ext, depth_sc, depth_imgs,
                            depth_img_count, &depth_fmt)) return;

    struct HandAct ha[2];
    XrPath hand[2];
    XrActionSet aset = XR_NULL_HANDLE;
    xr_setup_actions(inst, sess, ha, hand, &aset);

    GLuint prog = link_color_prog();
    GLuint tprog = link_text_prog();
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

    // Streaming-latency simulation: render the 3D scene with a pose from
    // N milliseconds in the past while still submitting the current frame
    // with the runtime's predicted display pose. This reproduces the
    // positional reprojection judder seen in ALVR/WiVRn, where the world is
    // rendered on the host with a stale prediction. Toggle with:
    //   adb shell setprop debug.xrtest.latency_ms 50
    //   adb shell setprop debug.xrtest.depth 1
    char latency_prop[32] = {0}, depth_prop[8] = {0};
    prop_str("debug.xrtest.latency_ms", latency_prop, sizeof(latency_prop));
    prop_str("debug.xrtest.depth", depth_prop, sizeof(depth_prop));
    int64_t render_latency_ns = (int64_t)(strtof(latency_prop, NULL) * 1e6f);
    bool submit_depth = (atoi(depth_prop) != 0) && g_have_depth_ext && depth_fmt != 0;
    LOGI("streaming sim latency=%lldns depth=%d", (long long)render_latency_ns,
         submit_depth);

#define VIEW_HISTORY_SIZE 256
    struct ViewHistory {
        int64_t display_time;
        XrView views[2];
    } view_hist[VIEW_HISTORY_SIZE];
    int view_hist_wr = 0;
    int view_hist_count = 0;

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

        // Save current views in the ring so we can render with a stale pose
        // when simulating streaming latency (ALVR/WiVRn render on the host
        // with an older prediction).
        if (XR_SUCCEEDED(r) && found >= 2) {
            view_hist[view_hist_wr].display_time = fstate.predictedDisplayTime;
            view_hist[view_hist_wr].views[0] = views[0];
            view_hist[view_hist_wr].views[1] = views[1];
            view_hist_wr = (view_hist_wr + 1) % VIEW_HISTORY_SIZE;
            if (view_hist_count < VIEW_HISTORY_SIZE) view_hist_count++;
        }

        // Pick the render pose. With latency simulation we render as if the
        // scene was produced on the PC at render_latency_ns before display,
        // which is what makes the world swim when the runtime reprojects the
        // flat color image without per-pixel depth.
        XrView render_views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
        render_views[0] = views[0];
        render_views[1] = views[1];
        int64_t target_time = fstate.predictedDisplayTime - render_latency_ns;
        if (render_latency_ns > 0 && view_hist_count > 1) {
            int best = -1;
            int64_t best_dt = INT64_MAX;
            for (int i = 0; i < view_hist_count; i++) {
                int64_t dt = llabs(view_hist[i].display_time - target_time);
                if (dt < best_dt) {
                    best_dt = dt;
                    best = i;
                }
            }
            if (best >= 0) {
                render_views[0] = view_hist[best].views[0];
                render_views[1] = view_hist[best].views[1];
            }
        }

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
            LOGI("fps %.1f frames=%ld", fps, frames);
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
        LINE("stream sim  latency %3.0fms  depth %s (%s)",
             render_latency_ns / 1e6f,
             submit_depth ? "ON" : "off",
             g_have_depth_ext ? (depth_fmt ? "ext+fmt" : "ext") : "no-ext");
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
        XrCompositionLayerDepthInfoKHR depth_info[2];
        memset(depth_info, 0, sizeof(depth_info));
        bool have_views = XR_SUCCEEDED(r) && found >= 2;

        GLenum depth_attachment = GL_DEPTH_ATTACHMENT;
        if (depth_fmt == 0x88F0) depth_attachment = GL_DEPTH_STENCIL_ATTACHMENT;

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

                // attach per-pixel depth so the compositor can do positional
                // reprojection/timewarp instead of orientation-only.
                uint32_t depth_idx = 0;
                if (submit_depth && depth_sc[eye] != XR_NULL_HANDLE) {
                    pfn_xrAcquireSwapchainImage(depth_sc[eye], &acq, &depth_idx);
                    pfn_xrWaitSwapchainImage(depth_sc[eye], &wait);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, depth_attachment,
                                           GL_TEXTURE_2D,
                                           depth_imgs[eye][depth_idx].image, 0);
                }

                glViewport(0, 0, vcv[eye].recommendedImageRectWidth,
                           vcv[eye].recommendedImageRectHeight);
                glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                // render using the (possibly stale) render_views, but submit
                // the fresh views to the compositor. This is what ALVR/WiVRn
                // do: the host renders with an old predicted pose, then the
                // headset reprojects the result to the current pose.
                float proj[16], view[16], mvp[16];
                mat4_proj(render_views[eye].fov, 0.05f, 100.0f, proj);
                mat4_view_from_pose(render_views[eye].pose, view);
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
                    mat4_mul(cmvp, cmvp, model);
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
                if (submit_depth && depth_sc[eye] != XR_NULL_HANDLE)
                    pfn_xrReleaseSwapchainImage(depth_sc[eye], &rel);

                pviews[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                pviews[eye].pose = views[eye].pose;
                pviews[eye].fov = views[eye].fov;
                pviews[eye].subImage.swapchain = sc[eye];
                pviews[eye].subImage.imageRect.extent.width = vcv[eye].recommendedImageRectWidth;
                pviews[eye].subImage.imageRect.extent.height = vcv[eye].recommendedImageRectHeight;

                if (submit_depth && depth_sc[eye] != XR_NULL_HANDLE) {
                    depth_info[eye].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
                    depth_info[eye].next = NULL;
                    depth_info[eye].subImage.swapchain = depth_sc[eye];
                    depth_info[eye].subImage.imageRect = pviews[eye].subImage.imageRect;
                    depth_info[eye].minDepth = 0.0f;
                    depth_info[eye].maxDepth = 1.0f;
                    depth_info[eye].nearZ = 0.05f;
                    depth_info[eye].farZ = 100.0f;
                    pviews[eye].next = &depth_info[eye];
                }
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
        if (depth_sc[eye] != XR_NULL_HANDLE)
            pfn_xrDestroySwapchain(depth_sc[eye]);
        free(imgs[eye]);
        free(depth_imgs[eye]);
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
