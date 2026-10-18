#include "wlegl.h"

#include "dmabuf.h"
#include "../common/log.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <cstring>

static PFNEGLCREATEIMAGEKHRPROC pCreateImage;
static PFNEGLDESTROYIMAGEKHRPROC pDestroyImage;
static PFNEGLQUERYDMABUFFORMATSEXTPROC pQueryFmts;
static PFNEGLQUERYDMABUFMODIFIERSEXTPROC pQueryMods;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC pImageTex;
static int eglState = -1;   // -1 unprobed, 0 unusable, 1 ready

static EGLDisplay curDpy() { return eglGetCurrentDisplay(); }

static bool hasExt(EGLDisplay dpy, const char* name) {
    const char* exts = eglQueryString(dpy, EGL_EXTENSIONS);
    return exts && strstr(exts, name);
}

static bool probe() {
    if (eglState >= 0) return eglState == 1;
    const EGLDisplay dpy = curDpy();
    if (dpy == EGL_NO_DISPLAY) return false;   // retry once a ctx exists
    if (!hasExt(dpy, "EGL_EXT_image_dma_buf_import")) {
        eglState = 0;
        return false;
    }
    pCreateImage = (PFNEGLCREATEIMAGEKHRPROC)
        eglGetProcAddress("eglCreateImageKHR");
    pDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC)
        eglGetProcAddress("eglDestroyImageKHR");
    pImageTex = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
        eglGetProcAddress("glEGLImageTargetTexture2DOES");
    if (hasExt(dpy, "EGL_EXT_image_dma_buf_import_modifiers")) {
        pQueryFmts = (PFNEGLQUERYDMABUFFORMATSEXTPROC)
            eglGetProcAddress("eglQueryDmaBufFormatsEXT");
        pQueryMods = (PFNEGLQUERYDMABUFMODIFIERSEXTPROC)
            eglGetProcAddress("eglQueryDmaBufModifiersEXT");
    }
    eglState = pCreateImage && pDestroyImage && pImageTex ? 1 : 0;
    if (eglState == 0)
        LOGE("egl dmabuf procs missing, dmabuf global off");
    return eglState == 1;
}

bool wlEglReady() { return probe(); }

void wlEglFormats(std::vector<std::pair<uint32_t, uint64_t>>* out) {
    out->clear();
    if (!probe() || !pQueryFmts || !pQueryMods) return;
    EGLint n = 0;
    if (!pQueryFmts(curDpy(), 0, nullptr, &n) || n <= 0) return;
    std::vector<EGLint> f(n);
    if (!pQueryFmts(curDpy(), n, f.data(), &n)) return;
    for (EGLint fmt : f) {
        EGLint mn = 0;
        if (!pQueryMods(curDpy(), fmt, 0, nullptr, nullptr, &mn) ||
                mn <= 0) {
            out->emplace_back((uint32_t)fmt, DRM_FORMAT_MOD_INVALID);
            continue;
        }
        std::vector<EGLuint64KHR> m(mn);
        if (!pQueryMods(curDpy(), fmt, mn, m.data(), nullptr, &mn))
            continue;
        for (EGLint i = 0; i < mn; ++i)
            out->emplace_back((uint32_t)fmt, (uint64_t)m[i]);
    }
}

void* wlEglImport(const DmaBuf* d) {
    if (!probe() || !d || d->n <= 0) return nullptr;
    const uint64_t modInv = DRM_FORMAT_MOD_INVALID;
    EGLint at[48];
    int i = 0;
    at[i++] = EGL_WIDTH;                    at[i++] = d->w;
    at[i++] = EGL_HEIGHT;                   at[i++] = d->h;
    at[i++] = EGL_LINUX_DRM_FOURCC_EXT;     at[i++] = d->fmt;
    for (int p = 0; p < d->n && p < 4; ++p) {
        const EGLint base = EGL_DMA_BUF_PLANE0_FD_EXT + p * 3;
        at[i++] = base + 0;                 at[i++] = d->fd[p];
        at[i++] = base + 1;                 at[i++] = d->off[p];
        at[i++] = base + 2;                 at[i++] = d->stride[p];
        if (d->mod[p] != modInv) {
            const EGLint mb =
                EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT + p * 2;
            at[i++] = mb + 0;
            at[i++] = (EGLint)(d->mod[p] & 0xffffffffu);
            at[i++] = mb + 1;
            at[i++] = (EGLint)(d->mod[p] >> 32);
        }
    }
    at[i++] = EGL_NONE;
    const EGLImage img = pCreateImage(curDpy(), EGL_NO_CONTEXT,
        EGL_LINUX_DMA_BUF_EXT, nullptr, at);
    if (img == EGL_NO_IMAGE)
        LOGE("eglImage import failed: fmt %08x err %x", d->fmt,
             eglGetError());
    return img == EGL_NO_IMAGE ? nullptr : img;
}

void wlEglFree(void* img) {
    if (probe() && img) pDestroyImage(curDpy(), (EGLImage)img);
}

bool wlEglBindTex(unsigned tex, void* img) {
    if (!probe() || !img) return false;
    glBindTexture(GL_TEXTURE_2D, tex);
    pImageTex(GL_TEXTURE_2D, (GLeglImageOES)img);
    return true;
}
