// Copyright 2026, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// gralloc.vmd - ashmem gralloc0 for the vmd guest.
//
// The vendor's generic allocator@2.0 / mapper@2.0 passthrough HALs wrap
// a legacy gralloc module (hw_get_module("gralloc") -> gralloc.<prop>.so
// with ro.hardware.gralloc=vmd). This is that module: plain CPU buffers
// backed by /dev/ashmem, mmapped on demand in whatever process locks or
// registers the handle. No libc, no libhardware - it is fully
// freestanding so it can be loaded anywhere.

#include "vmdhal.h"

#define GR_MAGIC 0x03141592
#define ASHMEM_SET_SIZE 0x40087703

// handle layout: native_handle header + 1 fd + 12 ints; allocated as a
// single 64-byte anonymous map so free() is one munmap
typedef struct vmd_handle {
	native_handle_t base;
	int fd;
	int magic;
	int flags;
	int size;
	int owner;
	u64 addr;
	int pid;
	int w, h, stride, format;
	int unused1;
} vmd_handle_t;

_Static_assert(sizeof(vmd_handle_t) == 64, "vmd_handle_t size");

static int priv_validate(buffer_handle_t handle)
{
	const vmd_handle_t *h = (const vmd_handle_t *)handle;
	if (!h || h->base.version != (int)sizeof(native_handle_t) ||
	    h->base.numFds != 1 || h->base.numInts != 12 ||
	    h->magic != (int)GR_MAGIC) {
		return -22;
	}
	return 0;
}

// buffers are process-agnostic ashmem: whoever touches the handle maps
// it. base+pid record this process's mapping.
static int map_buffer(vmd_handle_t *h)
{
	int pid = (int)sys1(__NR_getpid, 0);
	if (h->addr && h->pid == pid) {
		return 0;
	}
	void *p = mm(0, (size_t)h->size, PROT_READ|PROT_WRITE, MAP_SHARED,
	             h->fd, 0);
	if (mm_err(p)) {
		return -12;
	}
	h->addr = (u64)p;
	h->pid = pid;
	return 0;
}

static int gr_alloc(alloc_device_t *dev, int w, int h, int format, int usage,
                    buffer_handle_t *pHandle, int *pStride)
{
	(void)dev;
	(void)usage;
	if (!pHandle || !pStride) {
		return -22;
	}
	int bpp;
	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_RGBX_8888:
	case HAL_PIXEL_FORMAT_BGRA_8888:
	case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
		bpp = 4;
		break;
	case HAL_PIXEL_FORMAT_RGB_888:
		bpp = 3;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		bpp = 2;
		break;
	default:
		dbgkv("gralloc: bad fmt ", (u64)format);
		return -22;
	}
	if (w <= 0 || h <= 0) {
		dbgkv("gralloc: bad dims ", (u64)(u32)w);
		return -22;
	}
	u64 stride64 = ((u64)w + 1) & ~1ULL;
	u64 size64 = (((u64)h + 1) & ~1ULL) * stride64 * (u64)bpp + 4;
	if (stride64 > 0x7fffffff || size64 > 0x7fffffff) {
		dbgkv("gralloc: dims too big ", (u64)(u32)w);
		return -22;
	}
	int stride = (int)stride64;
	int size = (int)size64;

	int fd = fd_open("/dev/ashmem", O_RDWR|O_CLOEXEC);
	if (fd < 0) {
		dbg("gralloc: ashmem open fail\n");
		return -12;
	}
	if (ioct(fd, ASHMEM_SET_SIZE, (void *)(long)size) < 0) {
		dbg("gralloc: ashmem size fail\n");
		fd_close(fd);
		return -12;
	}
	vmd_handle_t *hnd = mm(0, sizeof(*hnd), PROT_READ|PROT_WRITE,
	                       MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (mm_err(hnd)) {
		fd_close(fd);
		return -12;
	}
	hnd->base.version = sizeof(native_handle_t);
	hnd->base.numFds = 1;
	hnd->base.numInts = 12;
	hnd->fd = fd;
	hnd->magic = GR_MAGIC;
	hnd->owner = (int)sys1(__NR_getpid, 0);
	hnd->size = size;
	hnd->w = w;
	hnd->h = h;
	hnd->stride = stride;
	hnd->format = format;
	if (map_buffer(hnd) < 0) {
		fd_close(fd);
		unmap(hnd, sizeof(*hnd));
		return -12;
	}
	*pHandle = (buffer_handle_t)hnd;
	*pStride = stride;
	return 0;
}

static int gr_free(alloc_device_t *dev, buffer_handle_t handle)
{
	(void)dev;
	if (priv_validate(handle) < 0) {
		return -22;
	}
	vmd_handle_t *h = (vmd_handle_t *)handle;
	int pid = (int)sys1(__NR_getpid, 0);
	if (h->addr && h->pid == pid) {
		unmap((void *)h->addr, (size_t)h->size);
		h->addr = 0;
	}
	fd_close(h->fd);
	if (h->owner == pid) {
		unmap(h, sizeof(*h));
	}
	return 0;
}

static int gr_register(gralloc_module_t const *module, buffer_handle_t handle)
{
	(void)module;
	if (priv_validate(handle) < 0) {
		return -22;
	}
	return map_buffer((vmd_handle_t *)handle);
}

static int gr_unregister(gralloc_module_t const *module, buffer_handle_t handle)
{
	(void)module;
	if (priv_validate(handle) < 0) {
		return -22;
	}
	vmd_handle_t *h = (vmd_handle_t *)handle;
	if (h->addr && h->pid == (int)sys1(__NR_getpid, 0)) {
		unmap((void *)h->addr, (size_t)h->size);
		h->addr = 0;
	}
	return 0;
}

static int gr_lock(gralloc_module_t const *module, buffer_handle_t handle,
                   int usage, int l, int t, int w, int h, void **vaddr)
{
	(void)module;
	(void)usage; (void)l; (void)t; (void)w; (void)h;
	if (priv_validate(handle) < 0 || !vaddr) {
		return -22;
	}
	vmd_handle_t *hnd = (vmd_handle_t *)handle;
	int err = map_buffer(hnd);
	if (err < 0) {
		return err;
	}
	*vaddr = (void *)hnd->addr;
	return 0;
}

static int gr_unlock(gralloc_module_t const *module, buffer_handle_t handle)
{
	(void)module;
	if (priv_validate(handle) < 0) {
		return -22;
	}
	return 0;
}

static int gr_perform(gralloc_module_t const *module, int operation, ...)
{
	(void)module;
	dbgkv("gralloc: perform op=", (u64)operation);
	return -38;
}

static int gr_lock_ycbcr(gralloc_module_t const *module, buffer_handle_t handle,
                         int usage, int l, int t, int w, int h, void *ycbcr)
{
	(void)module; (void)handle; (void)usage; (void)l; (void)t; (void)w;
	(void)h; (void)ycbcr;
	return -22;
}

static int gr_lockAsync(gralloc_module_t const *module, buffer_handle_t handle,
                        int usage, int l, int t, int w, int h, void **vaddr,
                        int fenceFd)
{
	if (fenceFd >= 0) {
		fd_close(fenceFd);
	}
	return gr_lock(module, handle, usage, l, t, w, h, vaddr);
}

static int gr_unlockAsync(gralloc_module_t const *module, buffer_handle_t handle,
                          int *fenceFd)
{
	if (fenceFd) {
		*fenceFd = -1;
	}
	return gr_unlock(module, handle);
}

static int gr_lockAsync_ycbcr(gralloc_module_t const *module,
                              buffer_handle_t handle, int usage, int l, int t,
                              int w, int h, void *ycbcr, int fenceFd)
{
	if (fenceFd >= 0) {
		fd_close(fenceFd);
	}
	return gr_lock_ycbcr(module, handle, usage, l, t, w, h, ycbcr);
}

static i32 gr_getTransportSize(gralloc_module_t const *module,
                               buffer_handle_t handle, u32 *outNumFds,
                               u32 *outNumInts)
{
	(void)module;
	if (priv_validate(handle) < 0) {
		return -22;
	}
	*outNumFds = 1;
	*outNumInts = 12;
	return 0;
}

static i32 gr_validateBufferSize(gralloc_module_t const *module,
                                 buffer_handle_t handle, u32 w, u32 h,
                                 i32 format, int usage, u32 stride)
{
	(void)module; (void)w; (void)h; (void)format; (void)usage; (void)stride;
	return priv_validate(handle);
}

static int gr_close(hw_device_t *dev)
{
	if (dev) {
		unmap(dev, 200);
	}
	return 0;
}

static int gr_open(const hw_module_t *module, const char *name,
                   hw_device_t **device)
{
	if (!name || name[0] != 'g') {
		return -22;
	}
	alloc_device_t *ctx = mm(0, sizeof(*ctx), PROT_READ|PROT_WRITE,
	                         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (mm_err(ctx)) {
		return -12;
	}
	ctx->common.tag = HARDWARE_DEVICE_TAG;
	ctx->common.version = 0;
	ctx->common.module = (hw_module_t *)module;
	ctx->common.close = gr_close;
	ctx->alloc = gr_alloc;
	ctx->free = gr_free;
	*device = &ctx->common;
	dbg("gralloc: vmd opened\n");
	return 0;
}

static hw_module_methods_t gr_methods = { .open = gr_open };

gralloc_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = 3,
		.hal_api_version = 1,
		.id = GRALLOC_HARDWARE_MODULE_ID,
		.name = "vmd ashmem gralloc",
		.author = "Hibiscus",
		.methods = &gr_methods,
	},
	.registerBuffer = gr_register,
	.unregisterBuffer = gr_unregister,
	.lock = gr_lock,
	.unlock = gr_unlock,
	.perform = gr_perform,
	.lock_ycbcr = gr_lock_ycbcr,
	.lockAsync = gr_lockAsync,
	.unlockAsync = gr_unlockAsync,
	.lockAsync_ycbcr = gr_lockAsync_ycbcr,
	.getTransportSize = gr_getTransportSize,
	.validateBufferSize = gr_validateBufferSize,
};
