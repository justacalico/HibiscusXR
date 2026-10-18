#define VRHOME_DESKTOP 1

#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../panels/panel.h"
#include "../render/resources.h"
#include "../shell/backend.h"

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

class PreviewBackend : public DashBackend {
public:
    bool externalPanelTextures() override { return false; }

    int createPanel(Panel* p, unsigned tex, int w, int h, int) override {
        p->displayId = nextDisp++;
        p->taskId = p->displayId;
        p->st = (void*)(intptr_t)p->displayId;
        makePanelTex(tex);
        return p->displayId;
    }

    void updatePanel(Panel* p) override {
        p->stMat[0] = p->stMat[5] = p->stMat[10] = p->stMat[15] = 1.0f;
    }

    bool createKbdSurface(Kbd* k, unsigned tex, int w, int h, int) override {
        k->st = this;
        makeKbdTex(tex, w, h);
        return true;
    }
    void updateKbd(Kbd* k) override {
        k->stMat[0] = k->stMat[5] = k->stMat[10] = k->stMat[15] = 1.0f;
    }
    KbdState kbdState() override {
        return {kbdShown, kbdShown ? 9000 : -1, false};
    }
    void sendKbdHide() override { kbdShown = false; }
    void toggleKbd() { kbdShown = !kbdShown; }

    bool takePins(std::vector<std::string>* out) override {
        if (pinsSent) return false;
        pinsSent = true;
        *out = {"preview.browser", "preview.files", "preview.settings"};
        return true;
    }
    int appsVersion() override { return 1; }
    void launcherApps(std::vector<GridItem>* out) override {
        static const char* names[] = {
            "Browser", "Files", "Settings", "Store", "Notes",
            "Camera", "Music", "Video", "Terminal", "Maps",
            "Calendar", "Photos", "Mail", "Chat", "Games",
        };
        out->clear();
        for (const char* n : names) {
            GridItem it;
            it.pkg = std::string("preview.") + n;
            it.label = n;
            out->push_back(it);
        }
    }
    int notifVersion() override { return 1; }
    void notifs(std::vector<NotifItem>* out) override {
        NotifItem n;
        n.key = "preview";
        n.pkg = "preview.messages";
        n.title = "Preview notification";
        n.text = "This card uses the production dash renderer.";
        n.postMs = 0;
        n.clearable = true;
        out->assign(1, n);
    }
    void dismissNotif(const std::string&) override {}
    int uiStringsVersion() override { return 1; }
    void uiStrings(std::vector<std::string>* out) override {
        out->assign(1, "Library");
    }
    void sysStatus(int out[3]) override { out[0] = 1; out[1] = 87; out[2] = 0; }
    std::string appLabel(const std::string& pkg) override {
        const size_t dot = pkg.rfind('.');
        return dot == std::string::npos ? pkg : pkg.substr(dot + 1);
    }
    bool iconPixels(const std::string& pkg, IconPixels* out) override {
        unsigned h = 0;
        for (char c : pkg) h = h * 31 + (unsigned char)c;
        const unsigned char cols[][3] = {
            {76, 105, 162}, {94, 72, 150}, {54, 124, 136},
            {118, 82, 92}, {64, 132, 92}, {132, 112, 64},
        };
        const unsigned char* c = cols[h % 6];
        out->w = out->h = 96;
        out->rgba.resize(96 * 96 * 4);
        for (int y = 0; y < 96; ++y)
            for (int x = 0; x < 96; ++x) {
                unsigned char* p = out->rgba.data() + (y * 96 + x) * 4;
                const bool dot = (x - 48) * (x - 48) +
                                 (y - 48) * (y - 48) < 18 * 18;
                p[0] = dot ? 245 : c[0];
                p[1] = dot ? 247 : c[1];
                p[2] = dot ? 255 : c[2];
                p[3] = 255;
            }
        return true;
    }
    void launchPackageOn(const std::string& pkg, int displayId) override {
        LOGI("preview launch %s on display %d", pkg.c_str(), displayId);
    }
    void injectTouch(int displayId, float x, float y, int action) override {
        LOGI("touch disp=%d %.0f,%.0f action=%d", displayId, x, y, action);
    }

private:
    int nextDisp = 100;
    bool pinsSent = false;
    bool kbdShown = false;

    static void texParams(unsigned tex) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    static void makePanelTex(unsigned tex) {
        const int w = 640, h = 360;
        std::vector<unsigned char> px(w * h * 4);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                unsigned char* p = px.data() + (y * w + x) * 4;
                const bool line = x % 64 < 2 || y % 64 < 2;
                p[0] = line ? 64 : (unsigned char)(24 + x / 12);
                p[1] = line ? 88 : (unsigned char)(42 + y / 8);
                p[2] = line ? 110 : 72;
                p[3] = 255;
            }
        texParams(tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px.data());
    }

    static void makeKbdTex(unsigned tex, int w, int h) {
        std::vector<unsigned char> px(w * h * 4, 230);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                unsigned char* p = px.data() + (y * w + x) * 4;
                const bool key = x % 90 > 8 && y % 80 > 8;
                p[0] = p[1] = p[2] = key ? 54 : 28;
                p[3] = 255;
            }
        texParams(tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px.data());
    }
};

struct Cam {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    double lastX = 0.0f, lastY = 0.0f;
    bool first = true;
};

static void fwd(const Cam& c, float out[3]) {
    const float cp = cosf(c.pitch);
    out[0] = sinf(c.yaw) * cp;
    out[1] = sinf(c.pitch);
    out[2] = -cosf(c.yaw) * cp;
}

static Mat4 camView(const Cam& c) {
    Mat4 v = multiply(rotX(-c.pitch * 180.0f / (float)M_PI),
                      rotY(c.yaw * 180.0f / (float)M_PI));
    v.m[12] = -(v.m[0]*c.pos[0] + v.m[4]*c.pos[1] + v.m[8]*c.pos[2]);
    v.m[13] = -(v.m[1]*c.pos[0] + v.m[5]*c.pos[1] + v.m[9]*c.pos[2]);
    v.m[14] = -(v.m[2]*c.pos[0] + v.m[6]*c.pos[1] + v.m[10]*c.pos[2]);
    return v;
}

static void drawBackdrop(HudEngine* e, const Mat4& vp) {
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

static PreviewBackend* gBackend;
static HudEngine* gHud;
static Cam gCam;
static bool gConfirm = false;

static void mouseMove(GLFWwindow*, double x, double y) {
    if (gCam.first) { gCam.lastX = x; gCam.lastY = y; gCam.first = false; }
    gCam.yaw += (float)(x - gCam.lastX) * 0.0022f;
    gCam.pitch -= (float)(y - gCam.lastY) * 0.0022f;
    gCam.pitch = fmaxf(-1.45f, fminf(1.45f, gCam.pitch));
    gCam.lastX = x; gCam.lastY = y;
}

static void mouseBtn(GLFWwindow*, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    gConfirm = action == GLFW_PRESS;
    hudKey(gHud, kKeyEnter, gConfirm ? kKeyDown : kKeyUp, 0);
}

static void keyCb(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) hudKey(gHud, kKeyBack, kKeyUp, 0);
    else if (key == GLFW_KEY_G) gHud->grid.shown = !gHud->grid.shown;
    else if (key == GLFW_KEY_K) gBackend->toggleKbd();
    else if (key == GLFW_KEY_R) wantRecenter();
}

static long long nowMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* win = glfwCreateWindow(1440, 810, "Hibiscus dash preview",
                                       nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(win, mouseMove);
    glfwSetMouseButtonCallback(win, mouseBtn);
    glfwSetKeyCallback(win, keyCb);

    HudEngine hud;
    PreviewBackend backend;
    gHud = &hud; gBackend = &backend;
    hud.backend = &backend;
    hud.haveQuat = true;
    hud.dockAnchored = false;
    hud.summonMs = nowMs();
    if (initGlResources(&hud) != 0) return 1;
    queueLaunch("preview.browser");
    queueLaunch("preview.files");
    queueLaunch("preview.settings");

    long long last = nowMs();
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        const long long now = nowMs();
        const float dt = (now - last) / 1000.0f;
        last = now;
        float f[3], r[3];
        fwd(gCam, f);
        r[0] = cosf(gCam.yaw); r[1] = 0.0f; r[2] = sinf(gCam.yaw);
        const float speed = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                            ? 2.4f : 0.8f;
        const float mv = speed * dt;
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
            for (int i = 0; i < 3; ++i) gCam.pos[i] += f[i] * mv;
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
            for (int i = 0; i < 3; ++i) gCam.pos[i] -= f[i] * mv;
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
            for (int i = 0; i < 3; ++i) gCam.pos[i] += r[i] * mv;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
            for (int i = 0; i < 3; ++i) gCam.pos[i] -= r[i] * mv;
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) gCam.pos[1] += mv;
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) gCam.pos[1] -= mv;

        memcpy(hud.eyePos, gCam.pos, sizeof(gCam.pos));
        memcpy(hud.aimO, gCam.pos, sizeof(gCam.pos));
        memcpy(hud.aimD, f, sizeof(f));
        hud.aimYaw = gCam.yaw;
        hud.aimPitch = gCam.pitch;
        const Mat4 view = camView(gCam);
        tickDash(&hud, view, now);

        glfwGetFramebufferSize(win, &hud.width, &hud.height);
        glViewport(0, 0, hud.width, hud.height);
        glClearColor(0.02f, 0.025f, 0.035f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const Mat4 proj = perspective(70.0f,
            hud.height ? (float)hud.width / hud.height : 1.0f,
            0.05f, 100.0f);
        const Mat4 vp = multiply(proj, view);
        drawBackdrop(&hud, vp);
        drawDashScene(&hud, vp);
        glfwSwapBuffers(win);
    }
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
