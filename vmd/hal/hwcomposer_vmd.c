// Copyright 2026, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// hwcomposer.vmd - hwc1 hwcomposer for the vmd guest.
//
// The vendor's generic composer@2.1 passthrough wraps a legacy hwc1
// module (hw_get_module("hwcomposer") -> hwcomposer.<prop>.so with
// ro.hardware.composer/hwcomposer=vmd). This one drives virtio-gpu
// directly: one dumb KMS buffer as the scanout, and set() blits the
// client-composed framebuffer target into it from the layer's ashmem
// fd (the fd rides in the native handle, so no libhardware/gralloc
// linkage is needed here either).

#include "vmdhal.h"

typedef struct vmd_priv_handle {
	native_handle_t base;
	int fd;
	int magic;
	int flags;
	int size;
	int unused0;
	u64 addr;
	int pid;
	int w, h, stride, format;
	int unused1;
} vmd_priv_handle_t;

typedef struct vmd_display {
	hwc_composer_device_1_t dev;
	int drm_fd;
	u32 conn, crtc, fb_id;
	struct drm_mode_modeinfo mode;
	u8 *fb;
	u64 fb_size;
	u32 pitch;
	const hwc_procs_t *procs;
	int vsync_on;
	int blanked;
} vmd_display_t;

static vmd_display_t g;

static void fire_vsync(void)
{
	if (!g.vsync_on || !g.procs || !g.procs->vsync) {
		return;
	}
	long ts[2] = {0, 0};
	sys2(__NR_clock_gettime, CLOCK_MONOTONIC, (long)ts);
	g.procs->vsync(g.procs, 0, ts[0] * 1000000000LL + ts[1]);
}

// copy a layer's buffer (ashmem fd, 32bpp) into the scanout at dx,dy
static void blit_target(hwc_layer_1_t *layer, int dx, int dy)
{
	vmd_priv_handle_t *hnd = (vmd_priv_handle_t *)layer->handle;
	if (!hnd || hnd->fd < 0 || !hnd->size) {
		return;
	}
	u8 *src = mm(0, (size_t)hnd->size, PROT_READ|PROT_WRITE, MAP_SHARED,
	             hnd->fd, 0);
	if (src == (u8 *)-1) {
		dbg("hwc: target map fail\n");
		return;
	}
	int w = hnd->w ? hnd->w : g.mode.hdisplay;
	int h = hnd->h ? hnd->h : g.mode.vdisplay;
	int stride = hnd->stride ? hnd->stride : w;
	if (dx < 0) dx = 0;
	if (dy < 0) dy = 0;
	if (h + dy > g.mode.vdisplay) h = g.mode.vdisplay - dy;
	if (w + dx > g.mode.hdisplay) w = g.mode.hdisplay - dx;
	for (int y = 0; y < h; y++) {
		u8 *dst = g.fb + (u64)(y + dy) * g.pitch + (u64)dx * 4;
		const u8 *s = src + (u64)y * (u64)stride * 4;
		for (int x = 0; x < w * 4; x++) {
			dst[x] = s[x];
		}
	}
	unmap(src, (size_t)hnd->size);
}

static void flush_dirty(void)
{
	struct drm_mode_fb_dirty_cmd d = {0};
	d.fb_id = g.fb_id;
	ioct(g.drm_fd, DRM_IOCTL_MODE_DIRTYFB, &d);
}

static int hwc_prepare(struct hwc_composer_device_1 *dev, size_t numDisplays,
                       hwc_display_contents_1_t **displays)
{
	(void)dev; (void)numDisplays;
	if (!displays || !displays[0]) {
		return 0;
	}
	hwc_display_contents_1_t *dpy = displays[0];
	for (size_t i = 0; i + 1 < dpy->numHwLayers; i++) {
		dpy->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
	}
	return 0;
}

static int hwc_set(struct hwc_composer_device_1 *dev, size_t numDisplays,
                   hwc_display_contents_1_t **displays)
{
	(void)dev; (void)numDisplays;
	if (!displays || !displays[0]) {
		return 0;
	}
	hwc_display_contents_1_t *dpy = displays[0];
	dpy->retireFenceFd = -1;
	for (size_t i = 0; i < dpy->numHwLayers; i++) {
		hwc_layer_1_t *l = &dpy->hwLayers[i];
		if (l->acquireFenceFd >= 0) {
			fd_close(l->acquireFenceFd);
		}
		if (l->compositionType == HWC_FRAMEBUFFER ||
		    l->compositionType == HWC_OVERLAY) {
			l->releaseFenceFd = -1;
		}
	}
	if (!g.blanked && g.fb && dpy->numHwLayers > 0) {
		hwc_layer_1_t *target = &dpy->hwLayers[dpy->numHwLayers - 1];
		if (target->compositionType == HWC_FRAMEBUFFER_TARGET &&
		    target->handle) {
			blit_target(target, 0, 0);
		}
		for (size_t i = 0; i + 1 < dpy->numHwLayers; i++) {
			hwc_layer_1_t *l = &dpy->hwLayers[i];
			if (l->handle && l->compositionType == HWC_OVERLAY) {
				blit_target(l, l->displayFrame.left,
				            l->displayFrame.top);
			}
		}
		static int logged;
		if (!logged) {
			logged = 1;
			dbgkv("hwc: first frame w=", g.mode.hdisplay);
			dbgkv("hwc: first frame h=", g.mode.vdisplay);
		}
		flush_dirty();
	}
	fire_vsync();
	return 0;
}

static int hwc_event(struct hwc_composer_device_1 *dev, int disp, int event,
                     int enabled)
{
	(void)dev; (void)disp;
	if (event != HWC_EVENT_VSYNC) {
		return -22;
	}
	g.vsync_on = enabled;
	fire_vsync();
	return 0;
}

static int hwc_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
	(void)dev; (void)disp;
	g.blanked = blank;
	return 0;
}

static int hwc_query(struct hwc_composer_device_1 *dev, int what, int *value)
{
	(void)dev; (void)what; (void)value;
	return -22;
}

static void hwc_regprocs(struct hwc_composer_device_1 *dev,
                         hwc_procs_t const *procs)
{
	(void)dev;
	g.procs = procs;
}

static void hwc_dump(struct hwc_composer_device_1 *dev, char *buff, int len)
{
	(void)dev; (void)buff; (void)len;
}

static int hwc_getconfigs(struct hwc_composer_device_1 *dev, int disp,
                          u32 *configs, size_t *numConfigs)
{
	(void)dev;
	if (disp != HWC_DISPLAY_PRIMARY) {
		return -22;
	}
	if (*numConfigs && configs) {
		configs[0] = 0;
	}
	*numConfigs = 1;
	return 0;
}

static int hwc_getattrs(struct hwc_composer_device_1 *dev, int disp, u32 config,
                        const u32 *attributes, i32 *values)
{
	(void)dev; (void)config;
	if (disp != HWC_DISPLAY_PRIMARY) {
		return -22;
	}
	for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
		switch (attributes[i]) {
		case HWC_DISPLAY_VSYNC_PERIOD:
			values[i] = 16666666;
			break;
		case HWC_DISPLAY_WIDTH:
			values[i] = g.mode.hdisplay;
			break;
		case HWC_DISPLAY_HEIGHT:
			values[i] = g.mode.vdisplay;
			break;
		case HWC_DISPLAY_DPI_X:
		case HWC_DISPLAY_DPI_Y:
			values[i] = 160000;
			break;
		default:
			values[i] = 0;
			break;
		}
	}
	return 0;
}

static int hwc_getactive(struct hwc_composer_device_1 *dev, int disp)
{
	(void)dev;
	return disp == HWC_DISPLAY_PRIMARY ? 0 : -22;
}

static int hwc_setactive(struct hwc_composer_device_1 *dev, int disp, int index)
{
	(void)dev; (void)index;
	return disp == HWC_DISPLAY_PRIMARY ? 0 : -22;
}

static int hwc_cursor(struct hwc_composer_device_1 *dev, int disp, int x, int y)
{
	(void)dev; (void)disp; (void)x; (void)y;
	return 0;
}

static int hwc_close(hw_device_t *dev)
{
	(void)dev;
	return 0;
}

static int drm_setup(void)
{
	g.drm_fd = fd_open("/dev/dri/card0", O_RDWR|O_CLOEXEC);
	if (g.drm_fd < 0) {
		dbg("hwc: open card0 fail\n");
		return -1;
	}
	struct drm_mode_card_res res = {0};
	ioct(g.drm_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
	u32 conns[32], crtcs[32], encs[32];
	res.connector_id_ptr = (u64)conns;
	res.crtc_id_ptr = (u64)crtcs;
	res.encoder_id_ptr = (u64)encs;
	ioct(g.drm_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
	for (u32 i = 0; i < res.count_connectors; i++) {
		struct drm_mode_get_connector c = {0};
		c.connector_id = conns[i];
		ioct(g.drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &c);
		struct drm_mode_modeinfo modes[64];
		u32 en[16];
		c.modes_ptr = (u64)modes;
		c.encoders_ptr = (u64)en;
		ioct(g.drm_fd, DRM_IOCTL_MODE_GETCONNECTOR, &c);
		if (c.connection == 1 && c.count_modes) {
			g.conn = conns[i];
			g.mode = modes[0];
			break;
		}
	}
	if (!g.conn) {
		dbg("hwc: no connected connector\n");
		return -1;
	}
	g.crtc = crtcs[0];

	struct drm_mode_create_dumb cd = {0};
	cd.width = g.mode.hdisplay;
	cd.height = g.mode.vdisplay;
	cd.bpp = 32;
	if (ioct(g.drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd)) {
		dbg("hwc: dumb fail\n");
		return -1;
	}
	struct drm_mode_fb_cmd2 a = {0};
	a.width = cd.width;
	a.height = cd.height;
	a.pixel_format = DRM_FORMAT_XRGB8888;
	a.handles[0] = cd.handle;
	a.pitches[0] = cd.pitch;
	if (ioct(g.drm_fd, DRM_IOCTL_MODE_ADDFB2, &a)) {
		dbg("hwc: addfb fail\n");
		return -1;
	}
	struct drm_mode_map_dumb md = {0};
	md.handle = cd.handle;
	if (ioct(g.drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &md)) {
		dbg("hwc: map fail\n");
		return -1;
	}
	g.fb = mm(0, cd.size, PROT_READ|PROT_WRITE, MAP_SHARED, g.drm_fd,
	          md.offset);
	if (g.fb == (u8 *)-1) {
		dbg("hwc: mmap fail\n");
		return -1;
	}
	g.fb_id = a.fb_id;
	g.fb_size = cd.size;
	g.pitch = cd.pitch;

	struct drm_mode_crtc s = {0};
	s.crtc_id = g.crtc;
	s.fb_id = g.fb_id;
	s.set_connectors_ptr = (u64)&g.conn;
	s.count_connectors = 1;
	s.mode = g.mode;
	s.mode_valid = 1;
	if (ioct(g.drm_fd, DRM_IOCTL_MODE_SETCRTC, &s)) {
		dbg("hwc: setcrtc fail\n");
		return -1;
	}
	return 0;
}

static int hwc_open(const hw_module_t *module, const char *name,
                    hw_device_t **device)
{
	(void)name;
	g.dev.common.tag = HARDWARE_DEVICE_TAG;
	g.dev.common.version = 0x01030001;
	g.dev.common.module = (hw_module_t *)module;
	g.dev.common.close = hwc_close;
	g.dev.prepare = hwc_prepare;
	g.dev.set = hwc_set;
	g.dev.eventControl = hwc_event;
	g.dev.blank = hwc_blank;
	g.dev.query = hwc_query;
	g.dev.registerProcs = hwc_regprocs;
	g.dev.dump = hwc_dump;
	g.dev.getDisplayConfigs = hwc_getconfigs;
	g.dev.getDisplayAttributes = hwc_getattrs;
	g.dev.getActiveConfig = hwc_getactive;
	g.dev.setActiveConfig = hwc_setactive;
	g.dev.setCursorPositionAsync = hwc_cursor;
	if (drm_setup() < 0) {
		return -1;
	}
	*device = &g.dev.common;
	dbg("hwc: hwcomposer.vmd opened\n");
	return 0;
}

static hw_module_methods_t hwc_methods = { .open = hwc_open };

hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.module_api_version = 1,
	.hal_api_version = 1,
	.id = HWC_HARDWARE_MODULE_ID,
	.name = "vmd drm hwcomposer",
	.author = "Hibiscus",
	.methods = &hwc_methods,
};
