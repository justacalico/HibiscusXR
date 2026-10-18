#define VRHOME_DESKTOP 1

#include "harness.h"

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

#include <cstdint>
#include <cstring>
#include <string>
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
    void kbdShow(bool on) { kbdShown = on; }

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
    void quickStatus(QsStatus* out) override {
        out->toggles = qsMask | (1u << QS_MIC) | (1u << QS_CENTER);
        out->batt = 87;
        out->volume = qsVol;
        out->brightness = qsBri;
        if (out->toggles & (1u << QS_WIFI)) out->ssid = "PreviewNet";
        if (out->toggles & (1u << QS_BLUETOOTH)) out->btName = "Preview Pods";
    }
    void quickToggle(int id, bool on) override {
        if (on) qsMask |= (1u << id); else qsMask &= ~(1u << id);
        LOGI("preview qs toggle %d -> %d", id, on ? 1 : 0);
    }
    void quickSetSlider(int slider, float v) override {
        if (slider == 0) qsBri = v; else qsVol = v;
    }
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
    unsigned qsMask = 1u << QS_WIFI;
    float qsVol = 0.6f, qsBri = 0.8f;

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
                const bool line = x % 64 < 1 || y % 64 < 1;
                p[0] = line ? 52 : (unsigned char)(30 + x / 14);
                p[1] = line ? 62 : (unsigned char)(50 + y / 9);
                p[2] = line ? 78 : 84;
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

int main(int argc, char** argv) {
    bool mcp = false;
    for (int i = 1; i < argc; ++i)
        mcp |= !strcmp(argv[i], "-mcp");

    DashApp app;
    if (!dashWindow(&app, "Hibiscus dash preview", 1440, 810)) return 1;

    HudEngine hud;
    PreviewBackend backend;
    app.hud = &hud;
    app.kbdSet = [&backend](bool on) { backend.kbdShow(on); };
    app.extraKey = [&backend](int key, int, int action) {
        if (key == GLFW_KEY_K && action == GLFW_PRESS)
            backend.kbdShow(!backend.kbdState().shown);
    };
    hud.backend = &backend;
    hud.haveQuat = true;
    hud.dockAnchored = false;
    hud.summonMs = dashNowMs();
    if (initGlResources(&hud) != 0) return 1;
    queueLaunch("preview.browser");
    queueLaunch("preview.files");
    queueLaunch("preview.settings");

    if (mcp) dashStartMcp(&app);

    long long last = dashNowMs();
    while (!glfwWindowShouldClose(app.win)) {
        glfwPollEvents();
        dashDrainPre();
        const long long now = dashNowMs();
        const float dt = (now - last) / 1000.0f;
        last = now;
        dashCamTick(&app, dt);
        const Mat4 view = camView(app.cam);
        tickDash(&hud, view, now);

        glfwGetFramebufferSize(app.win, &hud.width, &hud.height);
        glViewport(0, 0, hud.width, hud.height);
        glClearColor(0.02f, 0.025f, 0.035f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const Mat4 proj = perspective(70.0f,
            hud.height ? (float)hud.width / hud.height : 1.0f,
            0.05f, 100.0f);
        const Mat4 vp = multiply(proj, view);
        drawBackdrop(&hud, vp);
        drawDashScene(&hud, vp);
        dashDrainPost();
        glfwSwapBuffers(app.win);
    }
    dashStop(&app);
    return 0;
}
