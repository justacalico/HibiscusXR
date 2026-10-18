#include "harness.h"

#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../shell/backend.h"

#include <GLFW/glfw3.h>
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
    c.yaw += (float)(x - c.lastX) * 0.0022f;
    c.pitch -= (float)(y - c.lastY) * 0.0022f;
    c.pitch = fmaxf(-1.45f, fminf(1.45f, c.pitch));
    c.lastX = x; c.lastY = y;
}

static void mouseBtn(GLFWwindow*, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    hudKey(gApp->hud, kKeyEnter,
           action == GLFW_PRESS ? kKeyDown : kKeyUp, 0);
}

static void scrollCb(GLFWwindow*, double, double yoff) {
    // glfw's +y is wheel-up; hudScroll's positive delta goes down the list
    hudScroll(gApp->hud, (float)-yoff);
}

static void keyCb(GLFWwindow*, int key, int scancode, int action,
                  int mods) {
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
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(win, mouseMove);
    glfwSetMouseButtonCallback(win, mouseBtn);
    glfwSetScrollCallback(win, scrollCb);
    glfwSetKeyCallback(win, keyCb);
    app->win = win;
    return win;
}

void dashCamTick(DashApp* app, float dt) {
    DashCam& c = app->cam;
    float f[3], r[3];
    camFwd(c, f);
    r[0] = cosf(c.yaw); r[1] = 0.0f; r[2] = sinf(c.yaw);
    const float speed = (glfwGetKey(app->win, GLFW_KEY_LEFT_SHIFT)
                         == GLFW_PRESS) ? 2.4f : 0.8f;
    const float mv = speed * dt;
    if (glfwGetKey(app->win, GLFW_KEY_W) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] += f[i] * mv;
    if (glfwGetKey(app->win, GLFW_KEY_S) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] -= f[i] * mv;
    if (glfwGetKey(app->win, GLFW_KEY_D) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] += r[i] * mv;
    if (glfwGetKey(app->win, GLFW_KEY_A) == GLFW_PRESS)
        for (int i = 0; i < 3; ++i) c.pos[i] -= r[i] * mv;
    if (glfwGetKey(app->win, GLFW_KEY_E) == GLFW_PRESS) c.pos[1] += mv;
    if (glfwGetKey(app->win, GLFW_KEY_Q) == GLFW_PRESS) c.pos[1] -= mv;

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
            out->grid = hud->grid.shown;
            out->kbd = hud->kbd.shown;
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
// went away, so the window closes with it
static void mcpStdinLoop(McpServer* srv, GLFWwindow* win) {
    std::string line;
    while (std::getline(std::cin, line)) {
        const std::string out = srv->handleLine(line);
        if (out.empty()) continue;
        fwrite(out.data(), 1, out.size(), stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    glfwSetWindowShouldClose(win, GLFW_TRUE);
}

void dashStartMcp(DashApp* app) {
    static DashMcp mcpDash(app);   // MCP thread outlives the caller
    static McpServer mcpSrv(&mcpDash);
    LOGI("mcp server on stdio");
    app->mcpTh = std::thread(mcpStdinLoop, &mcpSrv, app->win);
}
