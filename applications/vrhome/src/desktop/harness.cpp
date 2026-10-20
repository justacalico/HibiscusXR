#include "harness.h"

#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../dock/dock.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../shell/backend.h"
#include "../set/set.h"

#include <GLFW/glfw3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <vector>

void camFwd(const DashCam& c, float out[3]) {
    const float cp = cosf(c.pitch);
    out[0] = sinf(c.yaw) * cp;
    out[1] = sinf(c.pitch);
    out[2] = -cosf(c.yaw) * cp;
}

Mat4 camView(const DashCam& c) {
    Mat4 v = multiply(rotX(-c.pitch * 180.0f / (float)M_PI),
                      rotY(c.yaw * 180.0f / (float)M_PI));
    v.m[12] = -(v.m[0]*c.pos[0] + v.m[4]*c.pos[1] + v.m[8]*c.pos[2]);
    v.m[13] = -(v.m[1]*c.pos[0] + v.m[5]*c.pos[1] + v.m[9]*c.pos[2]);
    v.m[14] = -(v.m[2]*c.pos[0] + v.m[6]*c.pos[1] + v.m[10]*c.pos[2]);
    return v;
}

void drawBackdrop(HudEngine* e, const Mat4& vp) {
    glUseProgram(e->sceneProg);
    const GLint uMVP = glGetUniformLocation(e->sceneProg, "uMVP");
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp.m);
    glDepthMask(GL_FALSE);
    glBindBuffer(GL_ARRAY_BUFFER, e->skyVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glDrawArrays(GL_TRIANGLES, 0, e->skyVerts);
    glDepthMask(GL_TRUE);
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glDrawArrays(GL_LINES, 0, e->gridVerts);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
}

long long dashNowMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static DashApp* gApp;

static void mouseMove(GLFWwindow*, double x, double y) {
    DashCam& c = gApp->cam;
    if (c.first) { c.lastX = x; c.lastY = y; c.first = false; }
    const double dx = x - c.lastX, dy = y - c.lastY;
    c.lastX = x; c.lastY = y;
    if (gApp->grabMotion && gApp->grabMotion(dx, dy)) return;
    c.yaw += (float)dx * 0.0022f;
    c.pitch -= (float)dy * 0.0022f;
    c.pitch = fmaxf(-1.45f, fminf(1.45f, c.pitch));
}

static void mouseBtn(GLFWwindow*, int button, int action, int) {
    if (gApp->grabBtn && gApp->grabBtn(button, action)) return;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        hudKey(gApp->hud, kKeyMenu,
               action == GLFW_PRESS ? kKeyDown : kKeyUp, 0);
        return;
    }
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    hudKey(gApp->hud, kKeyEnter,
           action == GLFW_PRESS ? kKeyDown : kKeyUp, 0);
}

static void scrollCb(GLFWwindow*, double, double yoff) {
    if (gApp->grabScroll && gApp->grabScroll(yoff)) return;
    // glfw's +y is wheel-up; hudScroll's positive delta goes down the list
    hudScroll(gApp->hud, (float)-yoff);
}

static void keyCb(GLFWwindow*, int key, int scancode, int action,
                  int mods) {
    if (gApp->grabKey && gApp->grabKey(key, scancode, action, mods))
        return;
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            hudKey(gApp->hud, escAction(mods & GLFW_MOD_SHIFT),
                   kKeyUp, 0);
            return;
        }
        if (key == GLFW_KEY_G) {
            gApp->hud->grid.shown = !gApp->hud->grid.shown;
            return;
        }
        if (key == GLFW_KEY_P) {
            gApp->hud->quick.shown = !gApp->hud->quick.shown;
            return;
        }
        if (key == GLFW_KEY_N) {
            gApp->hud->shade.shown = !gApp->hud->shade.shown;
            return;
        }
        if (key == GLFW_KEY_R) { wantRecenter(); return; }
    }
    if (gApp->extraKey) gApp->extraKey(key, scancode, action);
}

GLFWwindow* dashWindow(DashApp* app, const char* title, int w, int h) {
    gApp = app;
    if (!glfwInit()) return nullptr;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // EGL over GLX so dmabuf imports can land in an EGLImage
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    GLFWwindow* win = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!win) {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API,
                       GLFW_NATIVE_CONTEXT_API);
        win = glfwCreateWindow(w, h, title, nullptr, nullptr);
    }
    if (!win) { glfwTerminate(); return nullptr; }
    glfwMakeContextCurrent(win);
    // a vsynced swap waits inside EGL on a frame callback, and compositors
    // stop sending those while the window is occluded; the loop then
    // starves event dispatch until the host flags it unresponsive. Draws
    // pace themselves with dashDrawDue instead.
    glfwSwapInterval(0);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(win, mouseMove);
    glfwSetMouseButtonCallback(win, mouseBtn);
    glfwSetScrollCallback(win, scrollCb);
    glfwSetKeyCallback(win, keyCb);
    app->win = win;
    return win;
}

static EGLDisplay gNoWinDpy = EGL_NO_DISPLAY;
static EGLContext gNoWinCtx = EGL_NO_CONTEXT;
static unsigned gFbTex = 0, gFbDepth = 0;

bool dashHeadless(DashApp* app, int w, int h) {
    gApp = app;
    PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)
            eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!getPlatformDisplay)
        getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
            eglGetProcAddress("eglGetPlatformDisplay");
    if (!getPlatformDisplay) {
        LOGE("headless: eglGetPlatformDisplay missing");
        return false;
    }
    gNoWinDpy = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                   EGL_DEFAULT_DISPLAY, nullptr);
    if (gNoWinDpy == EGL_NO_DISPLAY ||
            !eglInitialize(gNoWinDpy, nullptr, nullptr)) {
        LOGE("headless: surfaceless egl unavailable (needs Mesa)");
        gNoWinDpy = EGL_NO_DISPLAY;
        return false;
    }
    eglBindAPI(EGL_OPENGL_ES_API);
    // surfaceless displays expose no configs; EGL_MESA_configless_context
    // covers EGL_NO_CONFIG_KHR
    const EGLint cfgAttr[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                              EGL_NONE};
    EGLConfig cfg = EGL_NO_CONFIG_KHR;
    EGLint n = 0;
    eglChooseConfig(gNoWinDpy, cfgAttr, &cfg, 1, &n);
    if (n < 1) cfg = EGL_NO_CONFIG_KHR;
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    gNoWinCtx = eglCreateContext(gNoWinDpy, cfg, EGL_NO_CONTEXT, ctxAttr);
    if (gNoWinCtx == EGL_NO_CONTEXT ||
            !eglMakeCurrent(gNoWinDpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                            gNoWinCtx)) {
        LOGE("headless: es2 context failed");
        return false;
    }
    glGenTextures(1, &gFbTex);
    glBindTexture(GL_TEXTURE_2D, gFbTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenRenderbuffers(1, &gFbDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gFbDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    glGenFramebuffers(1, &app->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, gFbTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, gFbDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
        LOGE("headless: framebuffer incomplete");
        return false;
    }
    app->headless = true;
    app->fbW = w;
    app->fbH = h;
    return true;
}

bool dashAlive(DashApp* app) {
    if (app->quit) return false;
    return !app->win || !glfwWindowShouldClose(app->win);
}

void dashTarget(DashApp* app, HudEngine* hud) {
    if (app->win) {
        glfwGetFramebufferSize(app->win, &hud->width, &hud->height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        hud->width = app->fbW;
        hud->height = app->fbH;
        glBindFramebuffer(GL_FRAMEBUFFER, app->fbo);
    }
}

void dashPresent(DashApp* app) {
    if (app->win) glfwSwapBuffers(app->win);
    else glFlush();
}

void dashCamTick(DashApp* app, float dt) {
    DashCam& c = app->cam;
    float f[3], r[3];
    camFwd(c, f);
    r[0] = cosf(c.yaw); r[1] = 0.0f; r[2] = sinf(c.yaw);
    const bool flying = app->win &&
                        (!app->grabbed || !app->grabbed());
    const float speed = (flying && glfwGetKey(app->win,
                         GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ?
                        2.4f : 0.8f;
    const float mv = speed * dt;
    if (flying && glfwGetKey(app->win, GLFW_KEY_W) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] += f[i] * mv;
    if (flying && glfwGetKey(app->win, GLFW_KEY_S) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] -= f[i] * mv;
    if (flying && glfwGetKey(app->win, GLFW_KEY_D) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] += r[i] * mv;
    if (flying && glfwGetKey(app->win, GLFW_KEY_A) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] -= r[i] * mv;
    if (flying && glfwGetKey(app->win, GLFW_KEY_E) == GLFW_PRESS)
        c.pos[1] += mv;
    if (flying && glfwGetKey(app->win, GLFW_KEY_Q) == GLFW_PRESS)
        c.pos[1] -= mv;

    HudEngine* hud = app->hud;
    memcpy(hud->eyePos, c.pos, sizeof(c.pos));
    memcpy(hud->aimO, c.pos, sizeof(c.pos));
    memcpy(hud->aimD, f, sizeof(f));
    hud->aimYaw = c.yaw;
    hud->aimPitch = c.pitch;
}

struct McpJob {
    std::function<void()> fn;
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
};

static std::mutex gJobMu;
static std::deque<McpJob*> gPreJobs, gPostJobs;
static bool gLive = true;

static bool runJob(bool postDraw, const std::function<void()>& fn) {
    McpJob j;
    j.fn = fn;
    {
        std::lock_guard<std::mutex> l(gJobMu);
        if (!gLive) return false;
        (postDraw ? gPostJobs : gPreJobs).push_back(&j);
    }
    std::unique_lock<std::mutex> l(j.mu);
    j.cv.wait(l, [&] { return j.done; });
    return true;
}

static void drainJobs(std::deque<McpJob*>& q, bool run) {
    std::deque<McpJob*> batch;
    {
        std::lock_guard<std::mutex> l(gJobMu);
        batch.swap(q);
    }
    for (McpJob* j : batch) {
        if (run) j->fn();
        {
            std::lock_guard<std::mutex> lj(j->mu);
            j->done = true;
        }
        j->cv.notify_one();
    }
}

void dashDrainPre() { drainJobs(gPreJobs, true); }
void dashDrainPost() { drainJobs(gPostJobs, true); }

void dashStop(DashApp* app) {
    {
        std::lock_guard<std::mutex> l(gJobMu);
        gLive = false;
    }
    drainJobs(gPreJobs, false);
    drainJobs(gPostJobs, false);
    if (app->mcpTh.joinable()) app->mcpTh.detach();
    if (app->win) {
        glfwDestroyWindow(app->win);
        app->win = nullptr;
    }
    glfwTerminate();
    if (gNoWinDpy != EGL_NO_DISPLAY) {
        if (app->fbo) glDeleteFramebuffers(1, &app->fbo);
        if (gFbTex) glDeleteTextures(1, &gFbTex);
        if (gFbDepth) glDeleteRenderbuffers(1, &gFbDepth);
        eglMakeCurrent(gNoWinDpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        eglDestroyContext(gNoWinDpy, gNoWinCtx);
        eglTerminate(gNoWinDpy);
        gNoWinDpy = EGL_NO_DISPLAY;
        gNoWinCtx = EGL_NO_CONTEXT;
    }
}

class DashMcp : public McpDash {
public:
    DashMcp(DashApp* app) : app(app) {}
    void launch(const std::string& pkg) override {
        queueLaunch(pkg.c_str());
    }
    void key(int code, int action) override {
        runJob(false, [=] { hudKey(app->hud, code, action, 0); });
    }
    void look(float yawDeg, float pitchDeg) override {
        runJob(false, [=] {
            app->cam.yaw = yawDeg * (float)M_PI / 180.0f;
            const float p = pitchDeg * (float)M_PI / 180.0f;
            app->cam.pitch = fmaxf(-1.45f, fminf(1.45f, p));
        });
    }
    void move(float x, float y, float z) override {
        runJob(false, [=] {
            app->cam.pos[0] = x; app->cam.pos[1] = y; app->cam.pos[2] = z;
        });
    }
    void recenter() override { wantRecenter(); }
    void setGrid(bool shown) override {
        runJob(false, [=] { app->hud->grid.shown = shown; });
    }
    void setKbd(bool shown) override {
        if (app->kbdSet) runJob(false, [=] { app->kbdSet(shown); });
    }
    void setQuick(bool shown) override {
        runJob(false, [=] { app->hud->quick.shown = shown; });
    }
    void setShade(bool shown) override {
        runJob(false, [=] {
            if (shown != app->hud->shade.shown) dockBellActivate(app->hud);
        });
    }
    void setSettings(bool shown) override {
        runJob(false, [=] {
            if (shown) setOpen(app->hud, SEC_WIFI);
            else setClose(app->hud);
        });
    }
    void scroll(float delta) override {
        runJob(false, [=] { hudScroll(app->hud, delta); });
    }
    bool state(McpState* out) override {
        return runJob(true, [=] {
            HudEngine* hud = app->hud;
            memcpy(out->pos, app->cam.pos, sizeof(app->cam.pos));
            out->yawDeg = app->cam.yaw * 180.0f / (float)M_PI;
            out->pitchDeg = app->cam.pitch * 180.0f / (float)M_PI;
            out->hover = hud->hover;
            out->hoverZone = hud->hoverZone;
            out->grid = hud->grid.shown;
            out->kbd = hud->kbd.shown;
            out->settings = hud->set.shown;
            out->panels.clear();
            for (const Panel& p : hud->panels) {
                McpPanelInfo pi;
                pi.displayId = p.displayId;
                pi.taskId = p.taskId;
                pi.pkg = p.pkg;
                pi.label = p.label;
                pi.minimized = p.minimized;
                pi.floating = p.floating;
                pi.yawDeg = p.yaw * 180.0f / (float)M_PI;
                pi.pitchDeg = p.pitch * 180.0f / (float)M_PI;
                pi.scale = p.scale;
                out->panels.push_back(pi);
            }
        });
    }
    bool screenshot(std::vector<unsigned char>* rgba,
                    int* w, int* h) override {
        return runJob(true, [=] {
            HudEngine* hud = app->hud;
            *w = hud->width; *h = hud->height;
            if (*w <= 0 || *h <= 0) return;
            std::vector<unsigned char> px((size_t)*w * *h * 4);
            glReadPixels(0, 0, *w, *h, GL_RGBA, GL_UNSIGNED_BYTE,
                         px.data());
            const size_t row = (size_t)*w * 4;
            rgba->resize(px.size());
            for (int y = 0; y < *h; ++y)
                memcpy(rgba->data() + (size_t)y * row,
                       px.data() + (size_t)(*h - 1 - y) * row, row);
        });
    }

private:
    DashApp* app;
};

// stdin carries one JSON-RPC message per line; EOF means the MCP client
// went away, so the dash exits with it
static void mcpStdinLoop(McpServer* srv, DashApp* app) {
    std::string line;
    while (std::getline(std::cin, line)) {
        const std::string out = srv->handleLine(line);
        if (out.empty()) continue;
        fwrite(out.data(), 1, out.size(), stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    app->quit = true;
}

void dashStartMcp(DashApp* app) {
    static DashMcp mcpDash(app);   // MCP thread outlives the caller
    static McpServer mcpSrv(&mcpDash);
    LOGI("mcp server on stdio");
    app->mcpTh = std::thread(mcpStdinLoop, &mcpSrv, app);
}
