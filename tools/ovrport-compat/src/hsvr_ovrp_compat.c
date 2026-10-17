#include <stdint.h>
#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <sys/mman.h>

#define TAG "hsvrovrp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define VT_WAIT   44
#define VT_BEGIN  45
#define VT_SETEYE 180
#define VT_ENDEYE 189

#define VT_VADDR      0x3e1d60
#define FNTBL_VADDR   0x3f6000
#define FNTBL_ENTRIES 0x1000

#define MAX_STAGES 8
#define MAX_SC     4
#define MAX_IMG    8

typedef long (*vt_fn_t)();
typedef int (*xr_fn_t)();

static uintptr_t g_base, g_end;
static vt_fn_t g_orig_begin;

static xr_fn_t g_real_acquire, g_real_enumerate;
static int g_tbl_done;

static GLuint g_eye_tex[MAX_STAGES];
static int g_eye_stages;
static int g_eye_w = 1024, g_eye_h = 1024;

static void *g_sc[MAX_SC];
static uint32_t g_sc_img[MAX_SC][MAX_IMG];
static int g_sc_nimg[MAX_SC];
static int g_sc_n;
static void *g_last_sc;
static uint32_t g_acq_idx = ~0u;
static int g_stage_ctr;

static PFNGLBINDTEXTUREPROC p_glBindTexture;
static PFNGLTEXSTORAGE3DPROC p_glTexStorage3D;
static PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURELAYERPROC p_glFramebufferTextureLayer;
static PFNGLBLITFRAMEBUFFERPROC p_glBlitFramebuffer;
static PFNGLGETERRORPROC p_glGetError;
static PFNGLGETINTEGERVPROC p_glGetIntegerv;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC p_glGetFBAttParam;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC p_glCheckFBStatus;

static GLuint g_fbo[2];
static int g_fbo_made;

static void *glsym(const char *n) {
    void *p = (void *)eglGetProcAddress(n);
    if (!p) p = dlsym(RTLD_DEFAULT, n);
    return p;
}

static int gl_init(void) {
    if (p_glBindTexture) return 1;
    p_glBindTexture = glsym("glBindTexture");
    p_glTexStorage3D = glsym("glTexStorage3D");
    p_glGenFramebuffers = glsym("glGenFramebuffers");
    p_glBindFramebuffer = glsym("glBindFramebuffer");
    p_glFramebufferTextureLayer = glsym("glFramebufferTextureLayer");
    p_glBlitFramebuffer = glsym("glBlitFramebuffer");
    p_glGetError = glsym("glGetError");
    p_glGetIntegerv = glsym("glGetIntegerv");
    p_glGetFBAttParam = glsym("glGetFramebufferAttachmentParameteriv");
    p_glCheckFBStatus = glsym("glCheckFramebufferStatus");
    return p_glBindFramebuffer && p_glFramebufferTextureLayer && p_glBlitFramebuffer;
}

static int acquire_hook(uintptr_t swapchain, uintptr_t info, uint32_t *idx_out) {
    int r = g_real_acquire(swapchain, info, idx_out);
    if (r >= 0 && idx_out) {
        g_last_sc = (void *)swapchain;
        g_acq_idx = *idx_out;
    }
    return r;
}

static int enumerate_hook(uintptr_t swapchain, uint32_t cap, uint32_t *count_out, uintptr_t images) {
    int r = g_real_enumerate(swapchain, cap, count_out, images);
    if (r >= 0 && count_out && images && *count_out) {
        int slot = -1;
        for (int i = 0; i < g_sc_n; i++)
            if (g_sc[i] == (void *)swapchain) slot = i;
        if (slot < 0 && g_sc_n < MAX_SC) slot = g_sc_n++;
        if (slot >= 0) {
            uint32_t n = *count_out > MAX_IMG ? MAX_IMG : *count_out;
            g_sc[slot] = (void *)swapchain;
            g_sc_nimg[slot] = n;
            for (uint32_t i = 0; i < n; i++)
                g_sc_img[slot][i] = *(uint32_t *)(images + 16 + 24 * i);
            LOGI("swapchain %p images=%d", (void *)swapchain, (int)n);
        }
    }
    return r;
}

static void patch_write(void *addr, const void *data, size_t len) {
    mprotect((void *)((uintptr_t)addr & ~0xfffUL), 0x2000, PROT_READ | PROT_WRITE);
    memcpy(addr, data, len);
}

static void tbl_init(void) {
    if (g_tbl_done) return;
    g_tbl_done = 1;
    void *lh = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lh) lh = dlopen("libopenxr_loader.so", RTLD_NOW);
    if (!lh) { LOGE("openxr loader not loaded"); return; }
    void *acq = dlsym(lh, "xrAcquireSwapchainImage");
    void *enu = dlsym(lh, "xrEnumerateSwapchainImages");
    uintptr_t *tbl = (uintptr_t *)(g_base + FNTBL_VADDR);
    for (int i = 0; i < FNTBL_ENTRIES; i++) {
        if (acq && tbl[i] == (uintptr_t)acq) {
            g_real_acquire = (xr_fn_t)tbl[i];
            void *p = acquire_hook;
            patch_write(&tbl[i], &p, sizeof p);
        }
        if (enu && tbl[i] == (uintptr_t)enu) {
            g_real_enumerate = (xr_fn_t)tbl[i];
            void *p = enumerate_hook;
            patch_write(&tbl[i], &p, sizeof p);
        }
    }
    LOGI("fntbl acquire=%p enumerate=%p", (void *)g_real_acquire, (void *)g_real_enumerate);
}

static long begin_hook(uintptr_t thisptr, int frame, int a2, void *a3, void *a4, void *a5) {
    tbl_init();
    void **vt = *(void ***)thisptr;
    ((vt_fn_t)vt[VT_WAIT])(thisptr, frame, 0, 0, 0);
    return g_orig_begin(thisptr, frame, a2, a3, a4, a5);
}

static long seteyetex_hook(uintptr_t thisptr, int eye, int stage, uintptr_t handle,
                           int h, int w, int fmt, long a7, long out) {
    if (!handle) return 0;
    gl_init();
    if (stage >= 0 && stage < MAX_STAGES && handle < 0x100000) {
        g_eye_tex[stage] = (GLuint)handle;
        if (stage + 1 > g_eye_stages) g_eye_stages = stage + 1;
        if (w > 0) g_eye_w = w;
        if (h > 0) g_eye_h = h;
    }
    if (p_glBindTexture && p_glTexStorage3D) {
        p_glBindTexture(GL_TEXTURE_2D_ARRAY, (GLuint)handle);
        p_glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16F, g_eye_w, g_eye_h, 2);
        GLenum e = p_glGetError ? p_glGetError() : 0;
        if (e) LOGE("seteyetex tex=%u stage=%d glerr=0x%x", (unsigned)handle, stage, e);
        else LOGI("seteyetex stage=%d tex=%u %dx%d", stage, (unsigned)handle, g_eye_w, g_eye_h);
    }
    return 1;
}

static GLuint pick_src(void) {
    GLint fbo = 0, type = 0, name = 0;
    p_glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    if (fbo && p_glGetFBAttParam) {
        p_glGetFBAttParam(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        if (type == GL_TEXTURE)
            p_glGetFBAttParam(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
        if (name) return (GLuint)name;
    }
    if (g_eye_stages) return g_eye_tex[g_stage_ctr++ % g_eye_stages];
    return 0;
}

static long endeye_hook(uintptr_t thisptr, long a1, long a2, long a3) {
    if (!gl_init() || !g_last_sc || g_acq_idx == ~0u) return 1;
    int sc = -1;
    for (int i = 0; i < g_sc_n; i++)
        if (g_sc[i] == g_last_sc) sc = i;
    if (sc < 0 || g_acq_idx >= (uint32_t)g_sc_nimg[sc]) return 1;
    GLuint dst = g_sc_img[sc][g_acq_idx];
    GLuint src = pick_src();
    if (!src || !dst) return 1;
    if (!g_fbo_made) { g_fbo_made = 1; p_glGenFramebuffers(2, g_fbo); }
    GLint prev_r = 0, prev_d = 0;
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_r);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_d);
    for (int s = 0; s < 2; s++) {
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo[0]);
        p_glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src, 0, s);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_fbo[1]);
        p_glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst, 0, s);
        if (p_glCheckFBStatus &&
            (p_glCheckFBStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ||
             p_glCheckFBStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE))
            continue;
        p_glBlitFramebuffer(0, 0, g_eye_w, g_eye_h, 0, 0, g_eye_w, g_eye_h,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_d);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_r);
    GLenum e = p_glGetError ? p_glGetError() : 0;
    if (e) LOGE("endeye blit src=%u dst=%u acq=%u err=0x%x", src, dst, g_acq_idx, e);
    else LOGI("endeye blit src=%u dst=%u acq=%u", src, dst, g_acq_idx);
    return 1;
}

static int in_lib(const void *p) {
    return (uintptr_t)p >= g_base && (uintptr_t)p < g_end;
}

static int find_plugin_cb(struct dl_phdr_info *i, size_t sz, void *d) {
    if (i->dlpi_name && strstr(i->dlpi_name, "libOVRPlugin.so")) {
        uintptr_t *r = (uintptr_t *)d;
        r[0] = i->dlpi_addr;
        for (int j = 0; j < i->dlpi_phnum; j++) {
            const ElfW(Phdr) *p = &i->dlpi_phdr[j];
            if (p->p_type == PT_LOAD && p->p_vaddr + p->p_memsz > r[1])
                r[1] = p->p_vaddr + p->p_memsz;
        }
        r[1] += i->dlpi_addr;
    }
    return 0;
}

__attribute__((constructor))
static void hsvr_ovrp_init(void) {
    uintptr_t r[2] = {0, 0};
    dl_iterate_phdr(find_plugin_cb, r);
    g_base = r[0];
    g_end = r[1];
    if (!g_base) { LOGE("libOVRPlugin not found"); return; }

    void **vt = (void **)(g_base + VT_VADDR);
    if (!in_lib(vt[VT_WAIT]) || !in_lib(vt[VT_BEGIN]) || !in_lib(vt[VT_SETEYE])) {
        LOGE("unsupported libOVRPlugin build, vtable mismatch");
        return;
    }

    g_orig_begin = (vt_fn_t)vt[VT_BEGIN];
    void *p = begin_hook;
    patch_write(&vt[VT_BEGIN], &p, sizeof p);
    p = seteyetex_hook;
    patch_write(&vt[VT_SETEYE], &p, sizeof p);
    if (*(uint64_t *)vt[VT_ENDEYE] == 0xd65f03ff52800020UL) {
        p = endeye_hook;
        patch_write(&vt[VT_ENDEYE], &p, sizeof p);
    } else {
        LOGI("vt[%d] is not the stub, leaving EndEye2 alone", VT_ENDEYE);
    }
    LOGI("installed: base=%p orig_begin=%p", (void *)g_base, (void *)g_orig_begin);
}
