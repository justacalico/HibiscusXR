// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
//
// OpenXR app mode: registers against whatever runtime is active on the
// host (WiVRn, Monado), feeds the real HMD's head pose into the VM, and
// submits the guest framebuffer to the swapchains so the headset shows
// the same image it would get on the device.

#include "vmd.hpp"

#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <X11/Xlib.h>

#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
#include <cstring>
#include <vector>


static bool
xr_ok(XrResult r, const char *what)
{
	if (XR_SUCCEEDED(r)) {
		return true;
	}
	char name[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(XR_NULL_HANDLE, r, name);
	fprintf(stderr, "vmd xr: %s: %s\n", what, name);
	return false;
}

// pull the FBConfig matching the context's own fbconfig id
static GLXFBConfig
current_fbconfig(Display *dpy, GLXContext ctx)
{
	int id = 0;
	glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &id);
	int n = 0;
	GLXFBConfig *cfgs = glXGetFBConfigs(dpy, DefaultScreen(dpy), &n);
	GLXFBConfig found = nullptr;
	for (int i = 0; i < n; i++) {
		int cid = -1;
		glXGetFBConfigAttrib(dpy, cfgs[i], GLX_FBCONFIG_ID, &cid);
		if (cid == id) {
			found = cfgs[i];
			break;
		}
	}
	XFree(cfgs);
	return found;
}

int
vmd_xr_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop)
{
	if (!glfwInit()) {
		fprintf(stderr, "vmd xr: glfwInit failed\n");
		return 1;
	}
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow *win = glfwCreateWindow(64, 64, "vmd", nullptr, nullptr);
	if (!win) {
		fprintf(stderr, "vmd xr: window failed\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(win);

	XrInstanceCreateInfo ici = {XR_TYPE_INSTANCE_CREATE_INFO};
	snprintf(ici.applicationInfo.applicationName,
	         sizeof(ici.applicationInfo.applicationName), "vmd");
	ici.applicationInfo.applicationVersion = 1;
	ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	const char *exts[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
	ici.enabledExtensionNames = exts;
	ici.enabledExtensionCount = 1;

	XrInstance inst;
	if (!xr_ok(xrCreateInstance(&ici, &inst), "xrCreateInstance")) {
		fprintf(stderr, "vmd xr: no OpenXR runtime registered "
		        "(WiVRn/Monado not running?)\n");
		glfwDestroyWindow(win);
		glfwTerminate();
		return 1;
	}

	XrSystemGetInfo sgi = {XR_TYPE_SYSTEM_GET_INFO};
	sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemId sysid;
	if (!xr_ok(xrGetSystem(inst, &sgi, &sysid), "xrGetSystem")) {
		xrDestroyInstance(inst);
		return 1;
	}

	PFN_xrGetOpenGLGraphicsRequirementsKHR getReq = nullptr;
	xrGetInstanceProcAddr(inst, "xrGetOpenGLGraphicsRequirementsKHR",
	                      (PFN_xrVoidFunction *)&getReq);
	XrGraphicsRequirementsOpenGLKHR req = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
	if (getReq) {
		getReq(inst, sysid, &req);
	}

	Display *dpy = glfwGetX11Display();
	GLXContext glxctx = glfwGetGLXContext(win);
	XrGraphicsBindingOpenGLXlibKHR binding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
	binding.xDisplay = dpy;
	binding.glxContext = glxctx;
	binding.glxFBConfig = current_fbconfig(dpy, glxctx);
	binding.glxDrawable = glXGetCurrentDrawable();
	{
		int vid = 0;
		glXGetFBConfigAttrib(dpy, binding.glxFBConfig, GLX_VISUAL_ID, &vid);
		binding.visualid = vid;
	}

	XrSessionCreateInfo sci = {XR_TYPE_SESSION_CREATE_INFO};
	sci.systemId = sysid;
	sci.next = &binding;
	XrSession sess;
	if (!xr_ok(xrCreateSession(inst, &sci, &sess), "xrCreateSession")) {
		xrDestroyInstance(inst);
		return 1;
	}

	XrReferenceSpaceCreateInfo rci = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	rci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	rci.poseInReferenceSpace.orientation.w = 1.0f;
	XrSpace space;
	xrCreateReferenceSpace(sess, &rci, &space);

	uint32_t nviews = 0;
	XrViewConfigurationType vcfg = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	xrEnumerateViewConfigurationViews(inst, sysid, vcfg, 0, &nviews, nullptr);
	std::vector<XrViewConfigurationView> views(nviews, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	xrEnumerateViewConfigurationViews(inst, sysid, vcfg, nviews, &nviews, views.data());
	if (nviews == 0) {
		fprintf(stderr, "vmd xr: runtime reports no views\n");
		return 1;
	}

	struct eye {
		XrSwapchain sc;
		std::vector<XrSwapchainImageOpenGLKHR> imgs;
		uint32_t w, h;
	};
	std::vector<eye> eyes(nviews);
	for (uint32_t i = 0; i < nviews; i++) {
		XrSwapchainCreateInfo sci2 = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		sci2.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		sci2.format = GL_RGBA8;
		sci2.width = views[i].recommendedImageRectWidth;
		sci2.height = views[i].recommendedImageRectHeight;
		sci2.sampleCount = 1;
		sci2.faceCount = 1;
		sci2.arraySize = 1;
		sci2.mipCount = 1;
		if (!xr_ok(xrCreateSwapchain(sess, &sci2, &eyes[i].sc), "xrCreateSwapchain")) {
			return 1;
		}
		uint32_t nimg = 0;
		xrEnumerateSwapchainImages(eyes[i].sc, 0, &nimg, nullptr);
		eyes[i].imgs.resize(nimg, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
		xrEnumerateSwapchainImages(eyes[i].sc, nimg, &nimg,
		                           (XrSwapchainImageBaseHeader *)eyes[i].imgs.data());
		eyes[i].w = sci2.width;
		eyes[i].h = sci2.height;
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	std::vector<XrView> xviews(nviews, {XR_TYPE_VIEW});
	bool running = false;
	while (!stop->load()) {
		XrEventDataBuffer ev = {XR_TYPE_EVENT_DATA_BUFFER};
		while (xrPollEvent(inst, &ev) == XR_SUCCESS) {
			if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
				auto *sc = (XrEventDataSessionStateChanged *)&ev;
				if (sc->state == XR_SESSION_STATE_READY) {
					XrSessionBeginInfo bi = {XR_TYPE_SESSION_BEGIN_INFO};
					bi.primaryViewConfigurationType = vcfg;
					xrBeginSession(sess, &bi);
					running = true;
				} else if (sc->state == XR_SESSION_STATE_STOPPING) {
					xrEndSession(sess);
					running = false;
				} else if (sc->state == XR_SESSION_STATE_EXITING) {
					running = false;
					stop->store(true);
				}
			}
			ev = {XR_TYPE_EVENT_DATA_BUFFER};
		}
		if (!running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		XrFrameWaitInfo fwi = {XR_TYPE_FRAME_WAIT_INFO};
		XrFrameState fs = {XR_TYPE_FRAME_STATE};
		xrWaitFrame(sess, &fwi, &fs);
		XrFrameBeginInfo fbi = {XR_TYPE_FRAME_BEGIN_INFO};
		xrBeginFrame(sess, &fbi);

		XrViewState vstate = {XR_TYPE_VIEW_STATE};
		XrViewLocateInfo vli = {XR_TYPE_VIEW_LOCATE_INFO};
		vli.viewConfigurationType = vcfg;
		vli.displayTime = fs.predictedDisplayTime;
		vli.space = space;
		uint32_t nloc = 0;
		xrLocateViews(sess, &vli, &vstate, nviews, &nloc, xviews.data());

		// head pose = average of the two views
		if (nloc >= 1) {
			std::lock_guard<std::mutex> g(state->lock);
			auto &p0 = xviews[0].pose;
			state->quat[0] = p0.orientation.x;
			state->quat[1] = p0.orientation.y;
			state->quat[2] = p0.orientation.z;
			state->quat[3] = p0.orientation.w;
			state->pos[0] = p0.position.x;
			state->pos[1] = p0.position.y;
			state->pos[2] = p0.position.z;
			state->position_valid =
			    (vstate.viewStateFlags & XR_VIEW_STATE_POSITION_TRACKED_BIT) != 0;
		}

		int fw = 0, fh = 0;
		std::vector<uint8_t> fb;
		if (vm && vmd_vm_framebuffer(vm, &fw, &fh, &fb)) {
			glBindTexture(GL_TEXTURE_2D, tex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fw, fh, 0, GL_RGB,
			             GL_UNSIGNED_BYTE, fb.data());
		}

		std::vector<XrCompositionLayerProjectionView> pviews(nviews);
		for (uint32_t i = 0; i < nviews; i++) {
			uint32_t idx = 0;
			xrAcquireSwapchainImage(eyes[i].sc, nullptr, &idx);
			XrSwapchainImageWaitInfo wi = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
			wi.timeout = XR_INFINITE_DURATION;
			xrWaitSwapchainImage(eyes[i].sc, &wi);

			// guest panel is side-by-side stereo: each eye gets its half
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			GLuint img = eyes[i].imgs[idx].image;
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			                       GL_TEXTURE_2D, img, 0);
			glViewport(0, 0, eyes[i].w, eyes[i].h);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			if (fw > 0) {
				glEnable(GL_TEXTURE_2D);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				float u0 = i == 0 ? 0.0f : 0.5f;
				float u1 = i == 0 ? 0.5f : 1.0f;
				glBegin(GL_QUADS);
				glTexCoord2f(u0, 1); glVertex2f(-1, -1);
				glTexCoord2f(u1, 1); glVertex2f(1, -1);
				glTexCoord2f(u1, 0); glVertex2f(1, 1);
				glTexCoord2f(u0, 0); glVertex2f(-1, 1);
				glEnd();
				glDisable(GL_TEXTURE_2D);
			}
			xrReleaseSwapchainImage(eyes[i].sc, nullptr);

			pviews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
			pviews[i].pose = xviews[i].pose;
			pviews[i].fov = xviews[i].fov;
			pviews[i].subImage.swapchain = eyes[i].sc;
			pviews[i].subImage.imageRect.offset = {0, 0};
			pviews[i].subImage.imageRect.extent = {(int)eyes[i].w, (int)eyes[i].h};
		}

		XrCompositionLayerProjection proj = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
		proj.space = space;
		proj.viewCount = nviews;
		proj.views = pviews.data();
		const XrCompositionLayerBaseHeader *layers[] = {
		    (const XrCompositionLayerBaseHeader *)&proj};
		XrFrameEndInfo fei = {XR_TYPE_FRAME_END_INFO};
		fei.displayTime = fs.predictedDisplayTime;
		fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		fei.layerCount = 1;
		fei.layers = layers;
		xrEndFrame(sess, &fei);
	}

	xrDestroySpace(space);
	for (auto &e : eyes) xrDestroySwapchain(e.sc);
	xrDestroySession(sess);
	xrDestroyInstance(inst);
	glfwDestroyWindow(win);
	glfwTerminate();
	return 0;
}
