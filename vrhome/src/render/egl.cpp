#include "egl.h"

#include "shaders.h"
#include "scene_geo.h"
#include "../engine.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../text/font.h"

#include <android/native_window.h>
#include <vector>

static bool initEyeTargets(Engine* e) {
    const int ew = e->width / 2, eh = e->height;
    for (int i = 0; i < 2; ++i) {
        Eye& y = e->eye[i];
        if (y.fbo) { glDeleteFramebuffers(1, &y.fbo); glDeleteTextures(1, &y.tex);
                     glDeleteRenderbuffers(1, &y.depth); }
        y.w = ew; y.h = eh;
        glGenTextures(1, &y.tex); glBindTexture(GL_TEXTURE_2D, y.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ew, eh, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenRenderbuffers(1, &y.depth); glBindRenderbuffer(GL_RENDERBUFFER, y.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, ew, eh);
        glGenFramebuffers(1, &y.fbo); glBindFramebuffer(GL_FRAMEBUFFER, y.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, y.tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, y.depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("eye %d FBO incomplete", i); return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

// one-time EGL setup
int initEglContext(Engine* e) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, nullptr, nullptr);
    EGLConfig config; EGLint numConfigs = 0;
    eglChooseConfig(dpy, attribs, &config, 1, &numConfigs);
    if (numConfigs < 1) { LOGE("no EGL config"); return -1; }
    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctxAttribs);
    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface pb = eglCreatePbufferSurface(dpy, config, pbAttribs);
    e->display = dpy; e->eglConfig = config; e->context = ctx; e->pbuffer = pb;
    if (eglMakeCurrent(dpy, pb, pb, ctx) == EGL_FALSE) {
        LOGE("pbuffer current failed"); return -1;
    }
    return 0;
}

int initWindow(Engine* e, ANativeWindow* win) {
    if (e->display == EGL_NO_DISPLAY && initEglContext(e) != 0) return -1;
    EGLint format = 0;
    eglGetConfigAttrib(e->display, e->eglConfig, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(win, 0, 0, format);
    e->surface = eglCreateWindowSurface(e->display, e->eglConfig,
                                      win, nullptr);
    if (eglMakeCurrent(e->display, e->surface, e->surface, e->context)
            == EGL_FALSE) {
        LOGE("eglMakeCurrent failed"); return -1;
    }
    eglQuerySurface(e->display, e->surface, EGL_WIDTH, &e->width);
    eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &e->height);
    LOGI("surface %dx%d", e->width, e->height);
    if (!e->glInit) {
        e->glInit = true;
        LOGI("renderer %s", glGetString(GL_RENDERER));

        e->sceneProg = linkProg(kSceneVS, kSceneFS);
        e->warpProg  = linkProg(kWarpVS,  kWarpFS);
        e->textProg  = linkProg(kTextVS,  kTextFS);
        e->floatProg = linkProg(kFloatVS, kFloatFS);
        e->shapeProg = linkProg(kShapeVS, kShapeFS);
        e->holdProg  = linkProg(kShapeVS, kHoldFS);
        e->iconProg  = linkProg(kFloatVS, kIconFS);
        if (!e->sceneProg || !e->warpProg || !e->textProg || !e->floatProg ||
                !e->shapeProg || !e->holdProg || !e->iconProg)
            return -1;

        if (!loadFont(e)) LOGE("font load failed, HUD text disabled");
        glGenBuffers(1, &e->textVbo);
        glGenBuffers(1, &e->panelVbo);
        glGenBuffers(1, &e->quadVbo);
        glGenBuffers(1, &e->gridVbo);
        glGenBuffers(1, &e->skyVbo);
        const float quad[] = {-1,-1, 1,-1, -1,1,  1,-1, 1,1, -1,1};
        glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        std::vector<float> verts;
        e->gridVerts = buildGrid(verts);
        glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                     verts.data(), GL_STATIC_DRAW);
        e->skyVerts = buildSky(verts);
        glBindBuffer(GL_ARRAY_BUFFER, e->skyVbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                     verts.data(), GL_STATIC_DRAW);

        glEnable(GL_DEPTH_TEST);
    }
    // a surface can come back at a new size (the window was relaid out
    // after a rotation flap): the eye targets must track it or both eyes
    // keep the old dims and draw squashed into one side of the panel
    if (e->eye[0].w != e->width / 2 || e->eye[0].h != e->height) {
        if (!initEyeTargets(e)) return -1;
    }
    e->ready = true;
    return 0;
}

// covered or window gone: keep the context current on the pbuffer so panel
// textures and adoption keep working
void termWindow(Engine* e) {
    if (e->display != EGL_NO_DISPLAY)
        eglMakeCurrent(e->display, e->pbuffer, e->pbuffer, e->context);
    if (e->surface != EGL_NO_SURFACE)
        eglDestroySurface(e->display, e->surface);
    e->surface = EGL_NO_SURFACE; e->ready = false;
}

void termDisplay(Engine* e) {
    if (e->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(e->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (e->context != EGL_NO_CONTEXT) eglDestroyContext(e->display, e->context);
        if (e->surface != EGL_NO_SURFACE) eglDestroySurface(e->display, e->surface);
        if (e->pbuffer != EGL_NO_SURFACE) eglDestroySurface(e->display, e->pbuffer);
        eglTerminate(e->display);
    }
    e->display = EGL_NO_DISPLAY; e->context = EGL_NO_CONTEXT;
    e->surface = EGL_NO_SURFACE; e->pbuffer = EGL_NO_SURFACE; e->ready = false;
}
