#include <stdint.h>
#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

#define MAX_SEGS   8
#define MAX_STAGES 8
#define MAX_SC     4
#define MAX_IMG    8

typedef long (*vt_fn_t)();
typedef int (*xr_fn_t)();

static uintptr_t g_base, g_end;
static uintptr_t g_exec_lo, g_exec_hi;
static uintptr_t g_seg_lo[MAX_SEGS], g_seg_hi[MAX_SEGS];
static uint32_t g_seg_flags[MAX_SEGS];
static int g_segs;

static vt_fn_t g_orig_begin;
static volatile int g_vt_patched;

static xr_fn_t g_real_acquire, g_real_enumerate, g_monado_endf;
static xr_fn_t g_real_waitf, g_real_beginf, g_real_endf, g_real_release;
static xr_fn_t g_real_create_sc, g_real_wait_img, g_real_locate_views;
static xr_fn_t g_real_create_sess, g_real_create_refspace;
static int g_tbl_done;
static uintptr_t g_session;
static void *g_egl_dpy, *g_egl_ctx;

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

static uintptr_t g_eye_sc[2];
static GLuint g_eye_sc_img[2][MAX_IMG];
static int g_eye_sc_nimg[2];
static int g_eye_sc_tried;
static uintptr_t g_ref_space;
static int g_proj_ok;

static void resolve_monado_endf(void);
static PFNGLBINDTEXTUREPROC p_glBindTexture;
static PFNGLFRAMEBUFFERTEXTURE2DPROC p_glFramebufferTexture2D;
static PFNGLTEXSTORAGE3DPROC p_glTexStorage3D;
static PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURELAYERPROC p_glFramebufferTextureLayer;
static PFNGLBLITFRAMEBUFFERPROC p_glBlitFramebuffer;
static PFNGLGETERRORPROC p_glGetError;
static PFNGLGETINTEGERVPROC p_glGetIntegerv;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC p_glGetFBAttParam;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC p_glCheckFBStatus;
static PFNGLISTEXTUREPROC p_glIsTexture;
static PFNGLGETTEXLEVELPARAMETERIVPROC p_glGetTexLevelParameter;
static PFNGLREADPIXELSPROC p_glReadPixels;
static PFNGLFLUSHPROC p_glFlush;
static PFNGLCOLORMASKPROC p_glColorMask;
static PFNGLCLEARCOLORPROC p_glClearColor;
static PFNGLCLEARPROC p_glClear;

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
    p_glFramebufferTexture2D = glsym("glFramebufferTexture2D");
    p_glIsTexture = glsym("glIsTexture");
    p_glGetTexLevelParameter = glsym("glGetTexLevelParameteriv");
    p_glReadPixels = glsym("glReadPixels");
    p_glFlush = glsym("glFlush");
    p_glColorMask = glsym("glColorMask");
    p_glClearColor = glsym("glClearColor");
    p_glClear = glsym("glClear");
    return p_glBindFramebuffer && p_glFramebufferTextureLayer && p_glBlitFramebuffer;
}

static int acquire_hook(uintptr_t swapchain, uintptr_t info, uint32_t *idx_out) {
    int r = g_real_acquire(swapchain, info, idx_out);
    LOGI("acquire sc=%p r=%d idx=%u", (void *)swapchain, r,
         idx_out ? *idx_out : 9999);
    if (r >= 0 && idx_out) {
        g_last_sc = (void *)swapchain;
        g_acq_idx = *idx_out;
    }
    return r;
}

static int g_fr_log = 6;

static int waitf_hook(uintptr_t a0, uintptr_t a1, uintptr_t a2) {
    g_session = a0;
    int r = g_real_waitf(a0, a1, a2);
    if (g_fr_log > 0) { g_fr_log--; LOGI("waitframe r=%d", r); }
    return r;
}

static int beginf_hook(uintptr_t a0, uintptr_t a1) {
    int r = g_real_beginf(a0, a1);
    if (g_fr_log > 0) { g_fr_log--; LOGI("beginframe r=%d", r); }
    return r;
}

static int create_sc_hook(uintptr_t a0, uintptr_t a1, uintptr_t a2) {
    int r = g_real_create_sc(a0, a1, a2);
    if (a1) {
        uintptr_t ci = a1;
        LOGI("create_sc r=%d sc=%p fmt=%ld %ux%u arr=%u face=%u mip=%u usage=0x%lx",
             r, a2 ? (void *)*(uintptr_t *)a2 : 0,
             (long)*(int64_t *)(ci + 32),
             *(uint32_t *)(ci + 44), *(uint32_t *)(ci + 48),
             *(uint32_t *)(ci + 56), *(uint32_t *)(ci + 52),
             *(uint32_t *)(ci + 60), *(uint64_t *)(ci + 24));
    }
    return r;
}

static int wait_img_hook(uintptr_t a0, uintptr_t a1) {
    int r = g_real_wait_img(a0, a1);
    LOGI("waitimg sc=%p r=%d", (void *)a0, r);
    return r;
}

static int create_sess_hook(uintptr_t a0, uintptr_t a1, uintptr_t a2) {
    if (a1) {
        uintptr_t n = *(volatile uintptr_t *)(a1 + 8);
        while (n) {
            if (*(volatile uint32_t *)n == 1000024001) {
                g_egl_dpy = (void *)*(volatile uintptr_t *)(n + 16);
                g_egl_ctx = (void *)*(volatile uintptr_t *)(n + 32);
                LOGI("binding ctx=%p dpy=%p", g_egl_ctx, g_egl_dpy);
                break;
            }
            n = *(volatile uintptr_t *)(n + 8);
        }
    }
    int r = g_real_create_sess(a0, a1, a2);
    LOGI("create_session r=%d", r);
    return r;
}

static int sc_slot(uintptr_t h) {
    for (int i = 0; i < g_sc_n; i++)
        if (g_sc[i] == (void *)h) return i;
    return -1;
}

static int endf_hook(uintptr_t a0, uintptr_t a1) {
    static int ef_budget = 12;
    static uint64_t last_sig;
    static int ef_hb;
    if (++ef_hb % 240 == 1) LOGE("endf hb n=%d", ef_hb);
    if (a1) {
        uint32_t nlayers = *(volatile uint32_t *)(a1 + 28);
        uintptr_t layers = *(volatile uintptr_t *)(a1 + 32);
        uint64_t sig = nlayers;
        for (uint32_t i = 0; i < nlayers && i < 4 && layers; i++) {
            uintptr_t lp = *(volatile uintptr_t *)(layers + 8 * i);
            if (!lp) continue;
            sig = sig * 131 + *(volatile uint32_t *)lp;
            for (int off = 24; off <= 128; off += 8)
                sig = sig * 131 + sc_slot(*(volatile uintptr_t *)(lp + off)) + 1;
        }
        if (sig != last_sig) ef_budget = 60;
        if (ef_budget > 0) {
            ef_budget--;
            last_sig = sig;
            LOGI("endframe info nlayers=%u layers=%p", nlayers, (void *)layers);
            for (uint32_t i = 0; i < nlayers && i < 4 && layers; i++) {
            uintptr_t lp = *(volatile uintptr_t *)(layers + 8 * i);
            if (!lp) continue;
            uint32_t ltype = *(volatile uint32_t *)lp;
            int flags = (int)*(volatile uint64_t *)(lp + 16);
            int found = 0;
            for (int off = 24; off <= 128; off += 8) {
                uintptr_t v = *(volatile uintptr_t *)(lp + off);
                int s = sc_slot(v);
                if (s >= 0) {
                    int x = *(volatile int *)(lp + off + 8);
                    int y = *(volatile int *)(lp + off + 12);
                    int w = *(volatile int *)(lp + off + 16);
                    int h = *(volatile int *)(lp + off + 20);
                    int arr = *(volatile int *)(lp + off + 24);
                    LOGI("  layer%u type=%u flags=%d sc%d @off%d rect=%d,%d %dx%d arr=%d",
                         i, ltype, flags, s, off, x, y, w, h, arr);
                    found = 1;
                }
            }
            if (!found) LOGI("  layer%u type=%u flags=%d noswap", i, ltype, flags);
            }
        }
        uint32_t nl0 = *(volatile uint32_t *)(a1 + 28);
        uintptr_t l0 = *(volatile uintptr_t *)(a1 + 32);
        if (!g_ref_space && nl0 && l0) {
            uintptr_t lp0 = *(volatile uintptr_t *)l0;
            if (lp0) g_ref_space = *(volatile uintptr_t *)(lp0 + 24);
        }
    }
    if (g_proj_ok && g_eye_sc[0] && g_eye_sc[1] && g_real_locate_views
        && g_ref_space) {
        char vli[40], vs[24], views[2 * 64];
        memset(vli, 0, sizeof vli);
        memset(vs, 0, sizeof vs);
        memset(views, 0, sizeof views);
        *(uint32_t *)vli = 6;                /* XR_TYPE_VIEW_LOCATE_INFO */
        *(uint32_t *)(vli + 16) = 2;         /* PRIMARY_STEREO */
        *(int64_t *)(vli + 24) = *(volatile int64_t *)(a1 + 16);
        *(uint64_t *)(vli + 32) = g_ref_space;
        *(uint32_t *)vs = 11;                /* XR_TYPE_VIEW_STATE */
        *(uint32_t *)views = 7;              /* XR_TYPE_VIEW */
        *(uint32_t *)(views + 64) = 7;
        uint32_t vcount = 0;
        int lr = g_real_locate_views(g_session, (uintptr_t)vli, (uintptr_t)vs,
                                     2, (uintptr_t)&vcount, (uintptr_t)views);
        static int lv_logged;
        if (!lv_logged) {
            lv_logged = 1;
            LOGI("locate_views r=%d count=%u flags=%lx", lr, vcount,
                 (long)*(uint64_t *)(vs + 16));
            for (int i = 0; i < 2; i++) {
                float *f = (float *)(views + 64 * i + 44);
                float *q = (float *)(views + 64 * i + 16);
                float *p = (float *)(views + 64 * i + 32);
                LOGI("view%d fov=%f,%f,%f,%f quat=%f,%f,%f,%f pos=%f,%f,%f",
                     i, f[0], f[1], f[2], f[3], q[0], q[1], q[2], q[3],
                     p[0], p[1], p[2]);
            }
        }
        static char pl[48], pv[2 * 96];
        memset(pl, 0, sizeof pl);
        memset(pv, 0, sizeof pv);
        *(uint32_t *)pl = 35;                /* COMPOSITION_LAYER_PROJECTION */
        *(uint64_t *)(pl + 24) = g_ref_space;
        *(uint32_t *)(pl + 32) = 2;
        *(uint64_t *)(pl + 40) = (uintptr_t)pv;
        for (int i = 0; i < 2; i++) {
            char *v = pv + 96 * i;
            *(uint32_t *)v = 48;             /* COMPOSITION_LAYER_PROJECTION_VIEW */
            if (lr == 0 && vcount >= 2)
                memcpy(v + 16, views + 64 * i + 16, 44);
            else {
                *(float *)(v + 16 + 12) = 1.0f;      /* identity quat w */
                *(float *)(v + 44 + 0) = -0.9f;
                *(float *)(v + 44 + 4) = 0.9f;
                *(float *)(v + 44 + 8) = 0.9f;
                *(float *)(v + 44 + 12) = -0.9f;
            }
            *(uint64_t *)(v + 64) = g_eye_sc[i];
            *(int *)(v + 72) = 0;
            *(int *)(v + 76) = 0;
            *(int *)(v + 80) = g_eye_w;
            *(int *)(v + 84) = g_eye_h;
            *(int *)(v + 88) = 0;
        }
        char fei[40];
        uintptr_t larr[8];
        memcpy(fei, (const void *)a1, sizeof fei);
        uint32_t n_out = 1;
        larr[0] = (uintptr_t)pl;
        uint32_t nl = *(volatile uint32_t *)(a1 + 28);
        uintptr_t ol = *(volatile uintptr_t *)(a1 + 32);
        for (uint32_t i = 0; i < nl && i < 4 && ol; i++) {
            uintptr_t lp = *(volatile uintptr_t *)(ol + 8 * i);
            if (!lp) continue;
            if (*(volatile uint32_t *)lp == 36) continue;
            larr[n_out++] = lp;
        }
        *(uint32_t *)(fei + 28) = n_out;
        *(uint64_t *)(fei + 32) = (uintptr_t)larr;
        resolve_monado_endf();
        xr_fn_t ef = g_monado_endf ? g_monado_endf : g_real_endf;
        int r = ef(a0, (uintptr_t)fei);
        static int proj_log = 30;
        if (proj_log > 0 || r != 0) {
            if (proj_log > 0) proj_log--;
            LOGI("endframe+proj r=%d nlayers=%u ef=%s", r, n_out,
                 ef == g_monado_endf ? "oxr" : "ldr");
        }
        return r;
    }
    int r = g_real_endf(a0, a1);
    if (g_fr_log > 0) { g_fr_log--; LOGI("endframe r=%d", r); }
    return r;
}

static int release_hook(uintptr_t a0, uintptr_t a1) {
    int r = g_real_release(a0, a1);
    LOGI("release sc=%p r=%d", (void *)a0, r);
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

static int in_exec(const void *p) {
    return (uintptr_t)p >= g_exec_lo && (uintptr_t)p < g_exec_hi;
}

static int is_stub_ret1(const void *p) {
    uint32_t a, b;
    memcpy(&a, p, 4);
    memcpy(&b, (const char *)p + 4, 4);
    return (a == 0x52800020 || a == 0xd2800020) &&
           (b == 0xd65f03c0 || b == 0xd65f03ff);
}

static uintptr_t g_acq_cand[8], g_enu_cand[8], g_wf_cand[8], g_bf_cand[8],
    g_ef_cand[8], g_rel_cand[8], g_csc_cand[8], g_wi_cand[8], g_cse_cand[8];
static int g_acq_n, g_enu_n, g_wf_n, g_bf_n, g_ef_n, g_rel_n, g_csc_n, g_wi_n,
    g_cse_n;

static void add_cand(uintptr_t *arr, int *n, uintptr_t v) {
    if (!v) return;
    for (int i = 0; i < *n; i++)
        if (arr[i] == v) return;
    if (*n < 8) arr[(*n)++] = v;
}

static void tbl_init(void) {
    if (g_tbl_done) return;
    static const char *mods[] = {
        "libopenxr_loader.so", "libopenxr_loader_generic.so", "libopenxr_monado.so"
    };
    for (int m = 0; m < 3; m++) {
        void *lh = dlopen(mods[m], RTLD_NOW | RTLD_NOLOAD);
        if (!lh) continue;
        add_cand(g_acq_cand, &g_acq_n, (uintptr_t)dlsym(lh, "xrAcquireSwapchainImage"));
        add_cand(g_enu_cand, &g_enu_n, (uintptr_t)dlsym(lh, "xrEnumerateSwapchainImages"));
        add_cand(g_wf_cand, &g_wf_n, (uintptr_t)dlsym(lh, "xrWaitFrame"));
        add_cand(g_bf_cand, &g_bf_n, (uintptr_t)dlsym(lh, "xrBeginFrame"));
        add_cand(g_ef_cand, &g_ef_n, (uintptr_t)dlsym(lh, "xrEndFrame"));
        add_cand(g_rel_cand, &g_rel_n, (uintptr_t)dlsym(lh, "xrReleaseSwapchainImage"));
        add_cand(g_csc_cand, &g_csc_n, (uintptr_t)dlsym(lh, "xrCreateSwapchain"));
        add_cand(g_wi_cand, &g_wi_n, (uintptr_t)dlsym(lh, "xrWaitSwapchainImage"));
        add_cand(g_cse_cand, &g_cse_n, (uintptr_t)dlsym(lh, "xrCreateSession"));
        if (!g_real_locate_views)
            g_real_locate_views = (xr_fn_t)dlsym(lh, "xrLocateViews");
        if (!g_real_create_refspace)
            g_real_create_refspace = (xr_fn_t)dlsym(lh, "xrCreateReferenceSpace");
        if (!g_monado_endf && strstr(mods[m], "monado"))
            g_monado_endf = (xr_fn_t)dlsym(lh, "xrEndFrame");
    }
    if (!g_acq_n || !g_enu_n) return;
    for (int s = 0; s < g_segs; s++) {
        if (!(g_seg_flags[s] & PF_W)) continue;
        uintptr_t *p = (uintptr_t *)g_seg_lo[s];
        uintptr_t *e = (uintptr_t *)g_seg_hi[s];
        for (; p < e; p++) {
            uintptr_t v = *p;
            for (int c = 0; c < g_acq_n; c++)
                if (v == g_acq_cand[c]) {
                    g_real_acquire = (xr_fn_t)v;
                    void *h = acquire_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_enu_n; c++)
                if (v == g_enu_cand[c]) {
                    g_real_enumerate = (xr_fn_t)v;
                    void *h = enumerate_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_wf_n; c++)
                if (v == g_wf_cand[c]) {
                    g_real_waitf = (xr_fn_t)v;
                    void *h = waitf_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_bf_n; c++)
                if (v == g_bf_cand[c]) {
                    g_real_beginf = (xr_fn_t)v;
                    void *h = beginf_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_ef_n; c++)
                if (v == g_ef_cand[c]) {
                    g_real_endf = (xr_fn_t)v;
                    void *h = endf_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_rel_n; c++)
                if (v == g_rel_cand[c]) {
                    g_real_release = (xr_fn_t)v;
                    void *h = release_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_csc_n; c++)
                if (v == g_csc_cand[c]) {
                    g_real_create_sc = (xr_fn_t)v;
                    void *h = create_sc_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_wi_n; c++)
                if (v == g_wi_cand[c]) {
                    g_real_wait_img = (xr_fn_t)v;
                    void *h = wait_img_hook;
                    patch_write(p, &h, sizeof h);
                }
            for (int c = 0; c < g_cse_n; c++)
                if (v == g_cse_cand[c]) {
                    g_real_create_sess = (xr_fn_t)v;
                    void *h = create_sess_hook;
                    patch_write(p, &h, sizeof h);
                }
        }
    }
    if (g_real_acquire && g_real_enumerate) {
        g_tbl_done = 1;
        LOGI("fntbl acquire=%p enumerate=%p waitf=%p beginf=%p endf=%p rel=%p",
             (void *)g_real_acquire, (void *)g_real_enumerate,
             (void *)g_real_waitf, (void *)g_real_beginf,
             (void *)g_real_endf, (void *)g_real_release);
    }
}

static long begin_hook(uintptr_t thisptr, int frame, int a2, void *a3, void *a4, void *a5) {
    tbl_init();
    if (!g_session) {
        void **vt = *(void ***)thisptr;
        if (in_exec(vt[VT_WAIT]))
            ((vt_fn_t)vt[VT_WAIT])(thisptr, frame, 0, 0, 0);
    }
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

static void diag_once(const char *fmt, ...);
static GLenum tex_target(GLuint t);

static GLuint pick_src(void) {
    GLint fbo = 0, type = 0, name = 0, fbor = 0;
    p_glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &fbor);
    for (int i = 0; i < 2; i++) {
        GLint fb = i ? fbor : fbo;
        if (!fb || !p_glGetFBAttParam) continue;
        type = 0; name = 0;
        p_glGetFBAttParam(i ? GL_READ_FRAMEBUFFER : GL_FRAMEBUFFER,
                          GL_COLOR_ATTACHMENT0,
                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        if (type == GL_TEXTURE)
            p_glGetFBAttParam(i ? GL_READ_FRAMEBUFFER : GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
        if (name) {
            diag_once("pick_src fbo=%d attach tex=%u", fb, name);
            return (GLuint)name;
        }
    }
    if (g_eye_stages) return g_eye_tex[g_stage_ctr++ % g_eye_stages];
    return 0;
}

static void learn_dims(GLuint t) {
    if (!p_glGetTexLevelParameter) return;
    GLenum tgt = tex_target(t);
    GLint prev = 0;
    p_glGetIntegerv(tgt == GL_TEXTURE_2D_ARRAY ? GL_TEXTURE_BINDING_2D_ARRAY
                                             : GL_TEXTURE_BINDING_2D, &prev);
    p_glBindTexture(tgt, t);
    GLint w = 0, h = 0;
    p_glGetTexLevelParameter(tgt, 0, GL_TEXTURE_WIDTH, &w);
    p_glGetTexLevelParameter(tgt, 0, GL_TEXTURE_HEIGHT, &h);
    p_glBindTexture(tgt, prev);
    if (w > 0 && h > 0) { g_eye_w = w; g_eye_h = h; }
}

static GLuint g_tex_name[64];
static GLenum g_tex_tgt[64];
static int g_tex_n;

static GLenum tex_target(GLuint t) {
    for (int i = 0; i < g_tex_n; i++)
        if (g_tex_name[i] == t) return g_tex_tgt[i];
    GLenum tgt = GL_TEXTURE_2D;
    if (p_glBindTexture) {
        p_glBindTexture(GL_TEXTURE_2D_ARRAY, t);
        if (p_glGetError && p_glGetError() == 0)
            tgt = GL_TEXTURE_2D_ARRAY;
        else
            p_glBindTexture(GL_TEXTURE_2D, t);
    }
    if (g_tex_n < 64) {
        g_tex_name[g_tex_n] = t;
        g_tex_tgt[g_tex_n++] = tgt;
    }
    return tgt;
}

static int attach_tex(GLenum fb, GLuint tex, int layer) {
    if (tex_target(tex) == GL_TEXTURE_2D_ARRAY) {
        p_glFramebufferTextureLayer(fb, GL_COLOR_ATTACHMENT0, tex, 0, layer);
    } else {
        if (layer > 0) return 0;
        p_glFramebufferTexture2D(fb, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    }
    return 1;
}

static GLuint g_dst_img[16];
static GLenum g_dst_tgt[16];
static int g_dst_n;
static int g_dst_scanned;

static int is_eye_tex(GLuint t) {
    for (int i = 0; i < g_eye_stages; i++)
        if (g_eye_tex[i] == t) return 1;
    return 0;
}

static void scan_dst(GLuint exclude) {
    if (g_dst_scanned > 0 || !p_glIsTexture || !p_glGetTexLevelParameter) return;
    g_dst_scanned++;
    GLint prev_tex = 0;
    p_glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &prev_tex);
    int ntex = 0, narr = 0;
    for (GLuint t = 1; t < 2048; t++) {
        if (!p_glIsTexture(t)) continue;
        ntex++;
        if (t == exclude || is_eye_tex(t)) continue;
        GLenum tgt = tex_target(t);
        if (tgt != GL_TEXTURE_2D_ARRAY && tgt != GL_TEXTURE_2D) continue;
        if (tgt == GL_TEXTURE_2D_ARRAY) narr++;
        GLint prev2 = 0;
        p_glGetIntegerv(tgt == GL_TEXTURE_2D_ARRAY ? GL_TEXTURE_BINDING_2D_ARRAY
                                                 : GL_TEXTURE_BINDING_2D, &prev2);
        p_glBindTexture(tgt, t);
        GLint w = 0, h = 0;
        p_glGetTexLevelParameter(tgt, 0, GL_TEXTURE_WIDTH, &w);
        p_glGetTexLevelParameter(tgt, 0, GL_TEXTURE_HEIGHT, &h);
        p_glBindTexture(tgt, prev2);
        if (w == g_eye_w && h == g_eye_h && g_dst_n < 16) {
            g_dst_img[g_dst_n] = t;
            g_dst_tgt[g_dst_n++] = tgt;
        }
        if (ntex < 400 && (w > 0))
            LOGE("  tex %u tgt=0x%x %dx%d", t, tgt, w, h);
    }
    p_glBindTexture(GL_TEXTURE_2D_ARRAY, prev_tex);
    LOGE("dst scan: %d texes, %d arrays, %d candidates (%dx%d)",
         ntex, narr, g_dst_n, g_eye_w, g_eye_h);
    if (g_dst_n) g_dst_scanned = -1;
    else g_dst_scanned = 0;
}

static int g_diag = 24;

static int blit_layer(GLuint src, int sl, GLuint dst, int dl) {
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo[0]);
    if (tex_target(src) == GL_TEXTURE_2D_ARRAY)
        p_glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src, 0, sl);
    else
        p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src, 0);
    GLenum e1 = p_glGetError();
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_fbo[1]);
    if (dl < 0)
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);
    else
        p_glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst, 0, dl);
    GLenum e2 = p_glGetError();
    GLenum cs_r = p_glCheckFBStatus ? p_glCheckFBStatus(GL_READ_FRAMEBUFFER) : 0;
    GLenum cs_d = p_glCheckFBStatus ? p_glCheckFBStatus(GL_DRAW_FRAMEBUFFER) : 0;
    if (g_diag > 0) {
        g_diag--;
        LOGE("blit src=%u/%d dst=%u/%d ae=%x/%x cs=%x/%x", src, sl, dst, dl, e1, e2, cs_r, cs_d);
    }
    if (e1 || e2) return 0;
    if (cs_r != GL_FRAMEBUFFER_COMPLETE || cs_d != GL_FRAMEBUFFER_COMPLETE) return 0;
    p_glBlitFramebuffer(0, 0, g_eye_w, g_eye_h, 0, 0, g_eye_w, g_eye_h,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    GLenum e3 = p_glGetError();
    if (e3 && g_diag > 0) { g_diag--; LOGE("blit err=%x", e3); }
    return e3 == 0;
}

static void diag_once(const char *fmt, ...) {
    static int n = 40;
    if (n <= 0) return;
    n--;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    LOGE("%s", buf);
}

static void ensure_refspace(void) {
    if (g_ref_space || !g_session || !g_real_create_refspace) return;
    char ci[48];
    memset(ci, 0, sizeof ci);
    *(uint32_t *)(ci + 0) = 37;
    *(uint32_t *)(ci + 16) = 2;
    *(float *)(ci + 32) = 1.0f;
    uintptr_t sp = 0;
    int r = g_real_create_refspace(g_session, (uintptr_t)ci, (uintptr_t)&sp);
    LOGI("create_refspace r=%d sp=%p", r, (void *)sp);
    if (r == 0) g_ref_space = sp;
}

static void build_proj_views(char *pl, char *pv, int lr, uint32_t vcount,
                             const char *views) {
    memset(pl, 0, 48);
    memset(pv, 0, 2 * 96);
    *(uint32_t *)pl = 35;
    *(uint64_t *)(pl + 24) = g_ref_space;
    *(uint32_t *)(pl + 32) = 2;
    *(uint64_t *)(pl + 40) = (uintptr_t)pv;
    for (int i = 0; i < 2; i++) {
        char *v = pv + 96 * i;
        *(uint32_t *)v = 48;
        if (lr == 0 && vcount >= 2)
            memcpy(v + 16, views + 64 * i + 16, 44);
        else {
            *(float *)(v + 16 + 12) = 1.0f;
            *(float *)(v + 44 + 0) = -0.9f;
            *(float *)(v + 44 + 4) = 0.9f;
            *(float *)(v + 44 + 8) = 0.9f;
            *(float *)(v + 44 + 12) = -0.9f;
        }
        *(uint64_t *)(v + 64) = g_eye_sc[i];
        *(int *)(v + 72) = 0;
        *(int *)(v + 76) = 0;
        *(int *)(v + 80) = g_eye_w;
        *(int *)(v + 84) = g_eye_h;
        *(int *)(v + 88) = 0;
    }
}

static void submit_frame(void) {
    static int sf_log = 30;
    if (!g_session || !g_real_waitf || !g_real_beginf || !g_eye_sc[0]
        || !g_eye_sc[1])
        return;
    ensure_refspace();
    if (!g_ref_space) return;
    char wi[16], fs[40], bi[16];
    memset(wi, 0, sizeof wi);
    memset(fs, 0, sizeof fs);
    memset(bi, 0, sizeof bi);
    *(uint32_t *)wi = 33;
    *(uint32_t *)fs = 44;
    int r = g_real_waitf(g_session, (uintptr_t)wi, (uintptr_t)fs);
    if (r != 0) {
        if (sf_log > 0) { sf_log--; LOGE("self waitf r=%d", r); }
        return;
    }
    *(uint32_t *)bi = 46;
    r = g_real_beginf(g_session, (uintptr_t)bi);
    if (r != 0 && r != 9) {
        if (sf_log > 0) { sf_log--; LOGE("self beginf r=%d", r); }
        return;
    }
    int64_t dt = *(volatile int64_t *)(fs + 16);
    char vli[40], vs[24], views[2 * 64];
    memset(vli, 0, sizeof vli);
    memset(vs, 0, sizeof vs);
    memset(views, 0, sizeof views);
    *(uint32_t *)vli = 6;
    *(uint32_t *)(vli + 16) = 2;
    *(int64_t *)(vli + 24) = dt;
    *(uint64_t *)(vli + 32) = g_ref_space;
    *(uint32_t *)vs = 11;
    *(uint32_t *)views = 7;
    *(uint32_t *)(views + 64) = 7;
    uint32_t vcount = 0;
    int lr = g_real_locate_views
        ? g_real_locate_views(g_session, (uintptr_t)vli, (uintptr_t)vs, 2,
                              (uintptr_t)&vcount, (uintptr_t)views)
        : -1;
    char pl[48], pv[2 * 96];
    build_proj_views(pl, pv, lr, vcount, views);
    char fei[40];
    uintptr_t larr[1];
    memset(fei, 0, sizeof fei);
    *(uint32_t *)fei = 12;
    *(int64_t *)(fei + 16) = dt;
    *(uint32_t *)(fei + 24) = 1;
    *(uint32_t *)(fei + 28) = 1;
    *(uint64_t *)(fei + 32) = (uintptr_t)larr;
    larr[0] = (uintptr_t)pl;
    resolve_monado_endf();
    xr_fn_t ef = g_monado_endf ? g_monado_endf : g_real_endf;
    r = ef(g_session, (uintptr_t)fei);
    if (sf_log > 0 || r != 0) {
        if (sf_log > 0) sf_log--;
        LOGE("self endf r=%d", r);
    }
}

static int ensure_eye_sc(void) {
    if (g_eye_sc[0] && g_eye_sc[1]) return 1;
    if (g_eye_sc_tried || !g_session || !g_real_create_sc || !g_real_enumerate
        || !g_real_acquire || !g_real_wait_img || !g_real_release)
        return 0;
    g_eye_sc_tried = 1;
    for (int eye = 0; eye < 2; eye++) {
        char ci[64];
        memset(ci, 0, sizeof ci);
        *(uint32_t *)(ci + 0) = 9;               /* SWAPCHAIN_CREATE_INFO */
        *(uint64_t *)(ci + 24) = 0x21;           /* COLOR_ATTACHMENT | SAMPLED */
        *(int64_t *)(ci + 32) = 35907;           /* GL_SRGB8_ALPHA8 */
        *(uint32_t *)(ci + 40) = 1;
        *(uint32_t *)(ci + 44) = g_eye_w;
        *(uint32_t *)(ci + 48) = g_eye_h;
        *(uint32_t *)(ci + 52) = 1;
        *(uint32_t *)(ci + 56) = 1;              /* one layer: plain 2D */
        *(uint32_t *)(ci + 60) = 1;
        uintptr_t sc = 0;
        int r = g_real_create_sc(g_session, (uintptr_t)ci, (uintptr_t)&sc);
        LOGI("eye%d swapchain create r=%d sc=%p %dx%d", eye, r,
             (void *)sc, g_eye_w, g_eye_h);
        if (r != 0 || !sc) return 0;
        char imgs[24 * MAX_IMG];
        memset(imgs, 0, sizeof imgs);
        for (int i = 0; i < MAX_IMG; i++)
            *(uint32_t *)(imgs + 24 * i) = 1000024002;
        uint32_t cnt = 0;
        r = g_real_enumerate(sc, MAX_IMG, (uintptr_t)&cnt, (uintptr_t)imgs);
        if (r != 0 || !cnt) {
            LOGE("eye%d swapchain enum r=%d cnt=%u", eye, r, cnt);
            return 0;
        }
        g_eye_sc[eye] = sc;
        g_eye_sc_nimg[eye] = cnt > MAX_IMG ? MAX_IMG : (int)cnt;
        for (int i = 0; i < g_eye_sc_nimg[eye]; i++) {
            g_eye_sc_img[eye][i] = *(uint32_t *)(imgs + 16 + 24 * i);
            LOGI("eye%d sc img%d tex=%u", eye, i, g_eye_sc_img[eye][i]);
        }
    }
    return 1;
}

static int blit_into_eye_sc(GLuint src) {
    if (!g_fbo_made) { g_fbo_made = 1; p_glGenFramebuffers(2, g_fbo); }
    GLint prev_r = 0, prev_d = 0;
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_r);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_d);
    EGLDisplay pd = eglGetCurrentDisplay();
    EGLContext pc = eglGetCurrentContext();
    EGLSurface prd = eglGetCurrentSurface(EGL_READ);
    EGLSurface pwd = eglGetCurrentSurface(EGL_DRAW);
    int swapped = 0;
    if (g_egl_dpy && g_egl_ctx && g_egl_ctx != (void *)pc)
        swapped = eglMakeCurrent(g_egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                 g_egl_ctx);
    int s1 = tex_target(src) == GL_TEXTURE_2D_ARRAY ? 1 : 0;
    int ok = 0;
    for (int eye = 0; eye < 2; eye++) {
        char ai[16], wi[24], ri[16];
        memset(ai, 0, sizeof ai);
        memset(wi, 0, sizeof wi);
        memset(ri, 0, sizeof ri);
        *(uint32_t *)ai = 55;
        *(uint32_t *)wi = 56;
        *(int64_t *)(wi + 16) = 100000000LL;
        *(uint32_t *)ri = 57;
        uint32_t idx = ~0u;
        int r = g_real_acquire(g_eye_sc[eye], (uintptr_t)ai, (uintptr_t)&idx);
        if (r == 0)
            r = g_real_wait_img(g_eye_sc[eye], (uintptr_t)wi);
        if (r != 0 || idx >= (uint32_t)g_eye_sc_nimg[eye]) {
            diag_once("eye%d sc acquire/wait r=%d idx=%u", eye, r, idx);
            continue;
        }
        GLuint dst = g_eye_sc_img[eye][idx];
        static int dim_logged;
        if (!dim_logged && p_glBindTexture && p_glGetTexLevelParameter) {
            dim_logged = 1;
            GLint pt = 0, w = -1, h = -1, ifmt = -1;
            p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &pt);
            p_glBindTexture(GL_TEXTURE_2D, dst);
            p_glGetTexLevelParameter(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
            p_glGetTexLevelParameter(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
            p_glGetTexLevelParameter(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &ifmt);
            p_glBindTexture(GL_TEXTURE_2D, pt);
            LOGE("dst tex %u resolves w=%d h=%d ifmt=%x istex=%d", dst, w, h, ifmt,
                 p_glIsTexture ? p_glIsTexture(dst) : -1);
        }
        ok += blit_layer(src, eye ? s1 : 0, dst, -1);
        if (p_glColorMask && p_glClearColor && p_glClear) {
            p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_fbo[1]);
            p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);
            p_glColorMask(0, 0, 0, 1);
            p_glClearColor(0, 0, 0, 1);
            p_glClear(GL_COLOR_BUFFER_BIT);
            p_glColorMask(1, 1, 1, 1);
        }
        static int dump_n;
        if (p_glReadPixels && dump_n < 400 && ++dump_n == 400) {
            uint8_t *buf = malloc((size_t)g_eye_w * g_eye_h * 4);
            if (buf) {
                p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo[1]);
                p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);
                ((void (*)(GLenum, GLint))glsym("glPixelStorei"))(
                    GL_PACK_ALIGNMENT, 1);
                p_glReadPixels(0, 0, g_eye_w, g_eye_h, GL_RGBA,
                               GL_UNSIGNED_BYTE, buf);
                {
                    long asum = 0;
                    for (size_t i = 3; i < (size_t)g_eye_w * g_eye_h * 4; i += 401)
                        asum += buf[i];
                    LOGE("dst alpha avg=%ld", asum / ((g_eye_w * g_eye_h * 4) / 401));
                }
                int nw = g_eye_w / 4, nh = g_eye_h / 4;
                FILE *f = fopen(
                    "/data/data/com.owlchemylabs.jobsimulator/files/eye0.ppm",
                    "wb");
                if (f) {
                    fprintf(f, "P6\n%d %d\n255\n", nw, nh);
                    for (int y = 0; y < nh; y++)
                        for (int x = 0; x < nw; x++) {
                            const uint8_t *p =
                                buf + 4 * ((size_t)(4 * y) * g_eye_w + 4 * x);
                            fwrite(p, 1, 3, f);
                        }
                    fclose(f);
                    LOGE("dumped eye0.ppm %dx%d", nw, nh);
                } else {
                    LOGE("eye0.ppm fopen failed errno=%d", errno);
                }
                free(buf);
            }
        }
        if (p_glFlush) p_glFlush();
        r = g_real_release(g_eye_sc[eye], (uintptr_t)ri);
        if (g_diag > 0) {
            g_diag--;
            LOGE("eye%d sc blit idx=%u rel=%d", eye, idx, r);
        }
    }
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_d);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_r);
    if (ok >= 2) g_proj_ok = 1;
    return ok;
}

static long endeye_hook(uintptr_t thisptr, long a1, long a2, long a3) {
    static int ee_hb;
    if (++ee_hb % 240 == 1) LOGE("endeye hb n=%d", ee_hb);
    if (!gl_init()) { diag_once("endeye: gl_init failed"); return 1; }
    GLuint src = pick_src();
    if (!src) { diag_once("endeye: no src"); return 1; }
    learn_dims(src);
    if (!g_eye_stages) { g_eye_tex[0] = src; g_eye_stages = 1; }
    static int ctx_logged;
    if (!ctx_logged) {
        ctx_logged = 1;
        LOGE("render ctx=%p dpy=%p (binding=%p/%p)",
             (void *)eglGetCurrentContext(), (void *)eglGetCurrentDisplay(),
             g_egl_ctx, g_egl_dpy);
    }
    if (ensure_eye_sc()) {
        blit_into_eye_sc(src);
        submit_frame();
        return 1;
    }
    int sc = -1;
    for (int i = 0; i < g_sc_n; i++)
        if (g_sc[i] == g_last_sc) sc = i;
    if (sc < 0 && g_sc_n) sc = g_sc_n - 1;
    if (sc < 0 || !g_sc_nimg[sc]) { diag_once("endeye: no swapchain images"); return 1; }
    uint32_t idx = g_acq_idx == ~0u ? 0 : g_acq_idx % g_sc_nimg[sc];
    GLuint dst = g_sc_img[sc][idx];
    if (!dst) { diag_once("endeye: dst image 0"); return 1; }
    static int dst_info_done;
    if (!dst_info_done && p_glIsTexture && p_glGetTexLevelParameter) {
        dst_info_done = 1;
        for (int i = 0; i < g_sc_n; i++)
            for (int j = 0; j < g_sc_nimg[i]; j++) {
                GLuint t = g_sc_img[i][j];
                GLenum tg = tex_target(t);
                GLint prev = 0, w = 0, h = 0;
                GLenum b = tg == GL_TEXTURE_2D_ARRAY ? GL_TEXTURE_BINDING_2D_ARRAY
                                                   : GL_TEXTURE_BINDING_2D;
                p_glGetIntegerv(b, &prev);
                p_glBindTexture(tg, t);
                p_glGetTexLevelParameter(tg, 0, GL_TEXTURE_WIDTH, &w);
                p_glGetTexLevelParameter(tg, 0, GL_TEXTURE_HEIGHT, &h);
                GLint iw = 0;
                p_glGetTexLevelParameter(tg, 0, GL_TEXTURE_INTERNAL_FORMAT, &iw);
                p_glBindTexture(tg, prev);
                LOGE("sc%d img%d tex=%u tgt=0x%x %dx%d fmt=0x%x",
                     i, j, t, tg, w, h, iw);
            }
    }
    if (!g_fbo_made) { g_fbo_made = 1; p_glGenFramebuffers(2, g_fbo); }
    GLint prev_r = 0, prev_d = 0;
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_r);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_d);
    GLenum tgt_s = tex_target(src);
    GLenum tgt_d = tex_target(dst);
    int layers = tgt_d == GL_TEXTURE_2D_ARRAY ? 2 : 1;
    int ok = 0;
    for (int s = 0; s < layers; s++)
        ok += blit_layer(src, s, dst, tgt_d == GL_TEXTURE_2D_ARRAY ? s : -1);
    if (g_diag > 0) {
        g_diag--;
        LOGE("blit src=%u(0x%x) dst=%u(0x%x) idx=%u ok=%d", src, tgt_s, dst, tgt_d, idx, ok);
    }
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_d);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_r);
    return 1;
}

static uintptr_t g_loader_base, g_monado_base;
static int find_named_cb(struct dl_phdr_info *i, size_t sz, void *d) {
    const char **names = (const char **)d;
    if (!i->dlpi_name) return 0;
    if (strstr(i->dlpi_name, "libopenxr_loader.so")) g_loader_base = i->dlpi_addr;
    if (strstr(i->dlpi_name, "libopenxr_monado.so")) {
        g_monado_base = i->dlpi_addr;
        for (int j = 0; j < i->dlpi_phnum; j++) {
            const ElfW(Phdr) *p = &i->dlpi_phdr[j];
            if (p->p_type == PT_LOAD && (p->p_flags & PF_X))
                names[0] = (const char *)(uintptr_t)(i->dlpi_addr + p->p_vaddr + p->p_memsz);
        }
    }
    return 0;
}

static void resolve_monado_endf(void) {
    if (g_monado_endf) return;
    const char *hi = 0;
    dl_iterate_phdr(find_named_cb, &hi);
    if (!g_loader_base || !g_monado_base) return;
    uintptr_t cand = *(volatile uintptr_t *)(g_loader_base + 0x297f0);
    if (cand > g_monado_base && cand < (uintptr_t)hi)
        g_monado_endf = (xr_fn_t)cand;
    uint8_t flag = *(volatile uint8_t *)(g_loader_base + 0x29366);
    LOGI("monado endf=%p flag29366=%u", (void *)g_monado_endf, flag);
}

static int find_plugin_cb(struct dl_phdr_info *i, size_t sz, void *d) {
    if (i->dlpi_name && strstr(i->dlpi_name, "libOVRPlugin.so")) {
        uintptr_t *r = (uintptr_t *)d;
        r[0] = i->dlpi_addr;
        for (int j = 0; j < i->dlpi_phnum && g_segs < MAX_SEGS; j++) {
            const ElfW(Phdr) *p = &i->dlpi_phdr[j];
            if (p->p_type != PT_LOAD) continue;
            g_seg_lo[g_segs] = i->dlpi_addr + p->p_vaddr;
            g_seg_hi[g_segs] = i->dlpi_addr + p->p_vaddr + p->p_memsz;
            g_seg_flags[g_segs] = p->p_flags;
            g_segs++;
            if (p->p_flags & PF_X) {
                g_exec_lo = i->dlpi_addr + p->p_vaddr;
                g_exec_hi = i->dlpi_addr + p->p_vaddr + p->p_memsz;
            }
            if (i->dlpi_addr + p->p_vaddr + p->p_memsz > r[1])
                r[1] = i->dlpi_addr + p->p_vaddr + p->p_memsz;
        }
        if (i->dlpi_phdr) {
            for (int j = 0; j < i->dlpi_phnum; j++)
                if (i->dlpi_phdr[j].p_type == PT_DYNAMIC)
                    r[2] = i->dlpi_addr + i->dlpi_phdr[j].p_vaddr;
        }
    }
    return 0;
}

static uintptr_t find_export(uintptr_t dyn, const char *name) {
    if (!dyn) return 0;
    ElfW(Sym) *symtab = 0;
    const char *strtab = 0;
    uint32_t nchain = 0;
    for (ElfW(Dyn) *e = (ElfW(Dyn) *)dyn; e->d_tag != DT_NULL; e++) {
        if (e->d_tag == DT_SYMTAB) symtab = (ElfW(Sym) *)(g_base + e->d_un.d_ptr);
        else if (e->d_tag == DT_STRTAB) strtab = (const char *)(g_base + e->d_un.d_ptr);
        else if (e->d_tag == DT_HASH)
            nchain = ((uint32_t *)(g_base + e->d_un.d_ptr))[1];
        else if (e->d_tag == DT_GNU_HASH) {
            uint32_t *h = (uint32_t *)(g_base + e->d_un.d_ptr);
            uint32_t nbucket = h[0], symndx = h[1], bloomw = h[2];
            const uint32_t *buckets = h + 4 + bloomw * 2;
            const uint32_t *chain = buckets + nbucket;
            uint32_t maxb = 0;
            for (uint32_t b = 0; b < nbucket; b++)
                if (buckets[b] > maxb) maxb = buckets[b];
            if (maxb >= symndx) {
                uint32_t idx = maxb;
                while (!(chain[idx - symndx] & 1)) idx++;
                nchain = idx + 1;
            }
        }
    }
    if (!symtab || !strtab || !nchain) return 0;
    for (uint32_t k = 0; k < nchain; k++)
        if (strcmp(strtab + symtab[k].st_name, name) == 0)
            return g_base + symtab[k].st_value;
    return 0;
}

static int64_t adrp_imm(uint32_t w) {
    int64_t imm = (((w >> 5) & 0x7ffff) << 2) | ((w >> 29) & 3);
    if (imm & (1 << 20)) imm -= 1 << 21;
    return imm << 12;
}

static uintptr_t decode_global(uintptr_t fn) {
    const uint32_t *ins = (const uint32_t *)fn;
    uintptr_t page = 0;
    int reg = -1;
    for (int i = 0; i < 64; i++) {
        uint32_t w = ins[i];
        if ((w & 0x9f000000) == 0x90000000) {
            page = (fn + i * 4 & ~0xfffUL) + adrp_imm(w);
            reg = w & 31;
            continue;
        }
        if (reg < 0) continue;
        if ((w & 0xffc00000) == 0xf9400000 && ((w >> 5) & 31) == reg)
            return page + (((w >> 10) & 0xfff) << 3);
        if ((w & 0x1f) == reg && ((w & 0xff000000) == 0x91000000 ||
            (w & 0xff000000) == 0xd1000000))
            reg = -1;
    }
    return 0;
}

static int in_libdata(const void *p) {
    return (uintptr_t)p >= g_base && (uintptr_t)p < g_end;
}

static int try_patch_vtable(void **vt) {
    if (!in_exec(vt[VT_WAIT]) || !in_exec(vt[VT_BEGIN]) || !in_exec(vt[VT_SETEYE]))
        return 0;
    if (!in_exec(vt[VT_ENDEYE]) || !is_stub_ret1(vt[VT_ENDEYE]))
        return 0;
    g_orig_begin = (vt_fn_t)vt[VT_BEGIN];
    __sync_synchronize();
    void *p = begin_hook;
    patch_write(&vt[VT_BEGIN], &p, sizeof p);
    p = seteyetex_hook;
    patch_write(&vt[VT_SETEYE], &p, sizeof p);
    p = endeye_hook;
    patch_write(&vt[VT_ENDEYE], &p, sizeof p);
    g_vt_patched = 1;
    return 1;
}

static void *poll_obj(void *arg) {
    uintptr_t global = (uintptr_t)arg;
    for (int i = 0; i < 30000 && !g_vt_patched; i++) {
        uintptr_t holder = *(volatile uintptr_t *)global;
        if (holder && in_libdata((void *)holder)) {
            uintptr_t obj = *(volatile uintptr_t *)holder;
            void **vt = obj ? *(void ***)obj : 0;
            if (!vt || !in_libdata(vt)) {
                obj = holder;
                vt = *(void ***)obj;
            }
            if (vt && in_libdata(vt) && in_exec(vt[VT_BEGIN]) &&
                try_patch_vtable(vt)) {
                LOGI("vtable %p patched (obj=%p)", vt, (void *)obj);
                break;
            }
        }
        usleep(2000);
    }
    if (!g_vt_patched) { LOGE("api object never appeared, shim inactive"); return 0; }
    for (int i = 0; i < 15000 && !g_tbl_done; i++) {
        tbl_init();
        usleep(2000);
    }
    if (!g_tbl_done) LOGE("xr fn table never populated");
    return 0;
}

__attribute__((constructor))
static void hsvr_ovrp_init(void) {
    uintptr_t r[3] = {0, 0, 0};
    dl_iterate_phdr(find_plugin_cb, r);
    g_base = r[0];
    g_end = r[1];
    if (!g_base || !g_exec_lo) { LOGE("libOVRPlugin not found"); return; }

    uintptr_t begin = find_export(r[2], "ovrp_BeginFrame");
    if (!begin || !in_exec((void *)begin)) {
        LOGE("ovrp_BeginFrame export not found");
        return;
    }
    uintptr_t global = decode_global(begin);
    if (!global || !in_libdata((void *)global)) {
        LOGE("api object global not decoded");
        return;
    }
    pthread_t t;
    if (pthread_create(&t, 0, poll_obj, (void *)global) == 0)
        pthread_detach(t);
    else
        LOGE("poll thread failed");
    LOGI("installed: base=%p begin=%p global=%p", (void *)g_base,
         (void *)begin, (void *)global);
}
