// Copyright 2026, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Freestanding base for the vmd guest HALs: direct syscalls (no libc,
// so the .so has zero DT_NEEDED and loads in any Android process), kmsg
// logging, the Android HAL ABI structs the modules implement, and the
// DRM uapi used to drive virtio-gpu.

#ifndef VMDHAL_H
#define VMDHAL_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;
typedef long long i64;
typedef unsigned long size_t;
typedef long ssize_t;
typedef unsigned long uintptr_t;

#define O_RDWR 02
#define O_WRONLY 01
#define O_CLOEXEC 02000000
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define CLOCK_MONOTONIC 1

#define __NR_ioctl 29
#define __NR_openat 56
#define __NR_close 57
#define __NR_write 64
#define __NR_exit 93
#define __NR_nanosleep 101
#define __NR_clock_gettime 113
#define __NR_getpid 172
#define __NR_munmap 215
#define __NR_mmap 222

static inline long sys1(long n, long a){register long x8 asm("x8")=n;register long x0 asm("x0")=a;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0):"memory");return x0;}
static inline long sys2(long n, long a, long b){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1):"memory");return x0;}
static inline long sys3(long n, long a, long b, long c){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1),"r"(x2):"memory");return x0;}
static inline long sys6(long n, long a, long b, long c, long d, long e, long f){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c,x3 asm("x3")=d,x4 asm("x4")=e,x5 asm("x5")=f;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");return x0;}

static inline int fd_open(const char *p, int fl){return (int)sys3(__NR_openat,-100,(long)p,fl);}
static inline int fd_close(int fd){return (int)sys1(__NR_close,fd);}
static inline long ioct(int fd, unsigned long req, void *arg){return sys3(__NR_ioctl,fd,req,(long)arg);}
static inline void *mm(void *a, size_t l, int pr, int fl, int f, long o){return (void*)sys6(__NR_mmap,(long)a,l,pr,fl,f,o);}
static inline int unmap(void *a, size_t l){return (int)sys2(__NR_munmap,(long)a,l);}
static inline void wr(int fd, const char *s, int n){sys3(__NR_write,fd,(long)s,n);}
static inline int sl(const char *s){int n=0;while(s[n])n++;return n;}

static int vmd_kmsg(void)
{
	static int kfd = -2;
	if (kfd == -2) {
		kfd = fd_open("/dev/kmsg", O_WRONLY|O_CLOEXEC);
	}
	return kfd;
}

static void dbg(const char *s)
{
	int kfd = vmd_kmsg();
	if (kfd >= 0) {
		wr(kfd, s, sl(s));
	}
}

static void dbgnum(u64 v)
{
	char b[24];
	int i = 23;
	b[i] = 0;
	if (!v) {
		dbg("0");
		return;
	}
	while (v) {
		b[--i] = '0' + v % 10;
		v /= 10;
	}
	dbg(b + i);
}

static __attribute__((unused)) void dbgkv(const char *k, u64 v)
{
	dbg(k);
	dbgnum(v);
}

// ---- Android HAL ABI ----------------------------------------------------

#define HARDWARE_DEVICE_TAG 0x48574454
#define HARDWARE_MODULE_TAG 0x48574D54
#define GRALLOC_HARDWARE_MODULE_ID "gralloc"
#define GRALLOC_HARDWARE_GPU0 "gpu0"
#define HWC_HARDWARE_MODULE_ID "hwcomposer"
#define HAL_MODULE_INFO_SYM HMI

struct hw_module_t;
struct hw_device_t;

typedef struct hw_module_t {
	u32 tag;
	u16 module_api_version;
	u16 hal_api_version;
	const char *id;
	const char *name;
	const char *author;
	struct hw_module_methods_t *methods;
	void *dso;
	u64 reserved[25];
} hw_module_t;

typedef struct hw_module_methods_t {
	int (*open)(const hw_module_t *module, const char *id,
	            struct hw_device_t **device);
} hw_module_methods_t;

typedef struct hw_device_t {
	u32 tag;
	u32 version;
	struct hw_module_t *module;
	u64 reserved[12];
	int (*close)(struct hw_device_t *dev);
} hw_device_t;

typedef struct native_handle {
	int version;
	int numFds;
	int numInts;
	int data[0];
} native_handle_t;

typedef const native_handle_t *buffer_handle_t;

enum {
	HAL_PIXEL_FORMAT_RGBA_8888 = 1,
	HAL_PIXEL_FORMAT_RGBX_8888 = 2,
	HAL_PIXEL_FORMAT_RGB_888 = 3,
	HAL_PIXEL_FORMAT_RGB_565 = 4,
	HAL_PIXEL_FORMAT_BGRA_8888 = 5,
	HAL_PIXEL_FORMAT_RGBA_FP16 = 0x16,
	HAL_PIXEL_FORMAT_YCbCr_420_888 = 0x23,
	HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 0x22,
};

#define GRALLOC_USAGE_SW_WRITE_MASK 0x000000F0U
#define GRALLOC_USAGE_HW_FB 0x00001000U
#define GRALLOC_USAGE_HW_COMPOSER 0x00000800U

typedef struct gralloc_module_t {
	hw_module_t common;
	int (*registerBuffer)(struct gralloc_module_t const *module, buffer_handle_t handle);
	int (*unregisterBuffer)(struct gralloc_module_t const *module, buffer_handle_t handle);
	int (*lock)(struct gralloc_module_t const *module, buffer_handle_t handle,
	            int usage, int l, int t, int w, int h, void **vaddr);
	int (*unlock)(struct gralloc_module_t const *module, buffer_handle_t handle);
	int (*perform)(struct gralloc_module_t const *module, int operation, ...);
	int (*lock_ycbcr)(struct gralloc_module_t const *module, buffer_handle_t handle,
	                int usage, int l, int t, int w, int h, void *ycbcr);
	int (*lockAsync)(struct gralloc_module_t const *module, buffer_handle_t handle,
	                 int usage, int l, int t, int w, int h, void **vaddr, int fenceFd);
	int (*unlockAsync)(struct gralloc_module_t const *module, buffer_handle_t handle,
	                   int *fenceFd);
	int (*lockAsync_ycbcr)(struct gralloc_module_t const *module, buffer_handle_t handle,
	                       int usage, int l, int t, int w, int h, void *ycbcr, int fenceFd);
	i32 (*getTransportSize)(struct gralloc_module_t const *module,
	                        buffer_handle_t handle, u32 *outNumFds, u32 *outNumInts);
	i32 (*validateBufferSize)(struct gralloc_module_t const *module,
	                          buffer_handle_t handle, u32 w, u32 h, i32 format,
	                          int usage, u32 stride);
	void *reserved_proc[1];
} gralloc_module_t;

typedef struct alloc_device_t {
	hw_device_t common;
	int (*alloc)(struct alloc_device_t *dev, int w, int h, int format, int usage,
	             buffer_handle_t *pHandle, int *pStride);
	int (*free)(struct alloc_device_t *dev, buffer_handle_t handle);
	void (*dump)(struct alloc_device_t *dev, char *buff, int buff_len);
	void *reserved_proc[7];
} alloc_device_t;

// ---- hwc1 ---------------------------------------------------------------

enum {
	HWC_FRAMEBUFFER = 0,
	HWC_OVERLAY = 1,
	HWC_BACKGROUND = 2,
	HWC_FRAMEBUFFER_TARGET = 3,
	HWC_SIDEBAND = 4,
	HWC_CURSOR_OVERLAY = 5,
};

#define HWC_EVENT_VSYNC 0

#define HWC_DISPLAY_VSYNC_PERIOD 1
#define HWC_DISPLAY_WIDTH 2
#define HWC_DISPLAY_HEIGHT 3
#define HWC_DISPLAY_DPI_X 4
#define HWC_DISPLAY_DPI_Y 5
#define HWC_DISPLAY_COLOR_TRANSFORM 6
#define HWC_DISPLAY_NO_ATTRIBUTE 0

#define HWC_DISPLAY_PRIMARY 0

typedef struct hwc_rect { int left, top, right, bottom; } hwc_rect_t;
typedef struct hwc_frect { float left, top, right, bottom; } hwc_frect_t;
typedef struct hwc_region { size_t numRects; hwc_rect_t const *rects; } hwc_region_t;
typedef struct hwc_color { u8 r, g, b, a; } hwc_color_t;

typedef struct hwc_layer_1 {
	i32 compositionType;
	u32 hints;
	u32 flags;
	union {
		hwc_color_t backgroundColor;
		struct {
			buffer_handle_t handle;
			u32 transform;
			i32 blending;
			union {
				hwc_rect_t sourceCropi;
				hwc_frect_t sourceCropf;
			};
			hwc_rect_t displayFrame;
			hwc_region_t visibleRegionScreen;
			int acquireFenceFd;
			int releaseFenceFd;
			u8 planeAlpha;
			u8 _pad[3];
			hwc_region_t surfaceDamage;
		};
	};
	u8 reserved[120 - 112];
} hwc_layer_1_t;

typedef struct hwc_display_contents_1 {
	int retireFenceFd;
	union {
		struct {
			void *dpy;
			void *sur;
		};
		struct {
			buffer_handle_t outbuf;
			int outbufAcquireFenceFd;
		};
	};
	u32 flags;
	size_t numHwLayers;
	hwc_layer_1_t hwLayers[0];
} hwc_display_contents_1_t;

typedef struct hwc_procs {
	void (*invalidate)(const struct hwc_procs *procs);
	void (*vsync)(const struct hwc_procs *procs, int disp, i64 timestamp);
	void (*hotplug)(const struct hwc_procs *procs, int disp, int connected);
} hwc_procs_t;

typedef struct hwc_composer_device_1 {
	hw_device_t common;
	int (*prepare)(struct hwc_composer_device_1 *dev,
	               size_t numDisplays, hwc_display_contents_1_t **displays);
	int (*set)(struct hwc_composer_device_1 *dev,
	           size_t numDisplays, hwc_display_contents_1_t **displays);
	int (*eventControl)(struct hwc_composer_device_1 *dev, int disp,
	                    int event, int enabled);
	int (*blank)(struct hwc_composer_device_1 *dev, int disp, int blank);
	int (*query)(struct hwc_composer_device_1 *dev, int what, int *value);
	void (*registerProcs)(struct hwc_composer_device_1 *dev,
	                      hwc_procs_t const *procs);
	void (*dump)(struct hwc_composer_device_1 *dev, char *buff, int buff_len);
	int (*getDisplayConfigs)(struct hwc_composer_device_1 *dev, int disp,
	                         u32 *configs, size_t *numConfigs);
	int (*getDisplayAttributes)(struct hwc_composer_device_1 *dev, int disp,
	                            u32 config, const u32 *attributes, i32 *values);
	int (*getActiveConfig)(struct hwc_composer_device_1 *dev, int disp);
	int (*setActiveConfig)(struct hwc_composer_device_1 *dev, int disp, int index);
	int (*setCursorPositionAsync)(struct hwc_composer_device_1 *dev, int disp,
	                              int x_pos, int y_pos);
	void *reserved_proc[1];
} hwc_composer_device_1_t;

// ---- DRM uapi -----------------------------------------------------------

#define DRM_FORMAT_XRGB8888 0x34325258

#define DRM_IOCTL_MODE_GETRESOURCES 0xC04064A0
#define DRM_IOCTL_MODE_GETCONNECTOR 0xC05064A7
#define DRM_IOCTL_MODE_SETCRTC 0xC06864A2
#define DRM_IOCTL_MODE_DIRTYFB 0xC01864B1
#define DRM_IOCTL_MODE_CREATE_DUMB 0xC02064B2
#define DRM_IOCTL_MODE_MAP_DUMB 0xC01064B3
#define DRM_IOCTL_MODE_ADDFB2 0xC06864B8

struct drm_mode_card_res {
	u64 fb_id_ptr, crtc_id_ptr, connector_id_ptr, encoder_id_ptr;
	u32 count_fbs, count_crtcs, count_connectors, count_encoders;
	u32 min_width, max_width, min_height, max_height;
};

struct drm_mode_modeinfo {
	u32 clock;
	u16 hdisplay, hsync_start, hsync_end, htotal, hskew;
	u16 vdisplay, vsync_start, vsync_end, vtotal, vscan;
	u32 vrefresh;
	u32 flags;
	u32 type;
	char name[32];
};

struct drm_mode_get_connector {
	u64 encoders_ptr, modes_ptr, props_ptr, prop_values_ptr;
	u32 count_modes, count_props, count_encoders;
	u32 encoder_id, connector_id, connector_type, connector_type_id;
	u32 connection, mm_width, mm_height, subpixel, pad;
};

struct drm_mode_create_dumb {
	u32 height, width, bpp, flags;
	u32 handle, pitch;
	u64 size;
};

struct drm_mode_fb_cmd2 {
	u32 fb_id, width, height, pixel_format, flags;
	u32 handles[4], pitches[4], offsets[4];
	u64 modifier[4];
};

struct drm_mode_map_dumb {
	u32 handle, pad;
	u64 offset;
};

struct drm_mode_crtc {
	u64 set_connectors_ptr;
	u32 count_connectors, crtc_id, fb_id;
	u32 x, y, gamma_size, mode_valid;
	struct drm_mode_modeinfo mode;
};

struct drm_mode_fb_dirty_cmd {
	u32 fb_id, flags, color, num_clips;
	u64 clips_ptr;
};

#endif
