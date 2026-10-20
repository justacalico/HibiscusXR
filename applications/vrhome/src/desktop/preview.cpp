#define VRHOME_DESKTOP 1

#include "harness.h"
#include "cmdline.h"

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
#include "../set/set.h"

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
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

    bool resizePanel(Panel*, int, int, int) override { return true; }

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
        out->toggles = qsMask | (1u << QS_MIC);
        out->net = (out->toggles & (1u << QS_WIFI)) ? DNET_WIFI
                                                  : DNET_NONE;
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

    // the settings card's fixture: a believable in-memory rig so the
    // preview exercises every row kind - scans fill the wifi/bt lists,
    // writes land on the fixture and the version bumps
    int settingsVersion() override { return setVer; }
    void settingsStatus(SetSnap* out) override {
        int t = 0;
        if (setWifiOn) t |= 1u << TG_WIFI;
        if (setBtOn) t |= 1u << TG_BT;
        if (setMicOn) t |= 1u << TG_MIC;
        if (setDof6) t |= 1u << TG_DOF6;
        if (setDbg) t |= 1u << TG_DEBUG_HUD;
        out->toggles = t;
        out->flags = (setWifiScan ? SFL_WIFI_SCAN : 0) |
                     (setBtScan ? SFL_BT_SCAN : 0) |
                     (setCtrlScan ? SFL_CTRL_SCAN : 0);
        out->sld[SL_VOL] = setVol;
        out->sld[SL_BRI] = setBri;
        out->sld[SL_IPD] = setIpd;
        out->txt[TX_SSID] = setWifiOn ? setSsid : "";
        out->txt[TX_THEME] = setTheme;
        out->txt[TX_ENV] = setEnv;
        out->txt[TX_LANG] = setLang;
        out->txt[TX_MODEL] = "Preview HMD";
        out->txt[TX_ANDROID] = "10";
        out->txt[TX_HBVER] = "preview";
        out->wifi = setWifi;
        out->bt = setBt;
        out->imes = setImes;
        out->envs = setEnvs;
        out->ctrl[0] = setCtrl[0];
        out->ctrl[1] = setCtrl[1];
    }
    void settingsSection(int sec) override {
        setSec = sec;
        if (sec == SEC_WIFI) settingsScan(SEC_WIFI);
        else if (sec == SEC_BT) settingsScan(SEC_BT);
    }
    void settingsClose() override { setSec = -1; }
    void settingsToggle(int id, bool on) override {
        switch (id) {
        case TG_WIFI: setWifiOn = on;
            if (on) settingsScan(SEC_WIFI);
            else { setSsid = ""; setWifi.clear(); }
            break;
        case TG_BT: setBtOn = on;
            if (on) settingsScan(SEC_BT);
            else setBt.clear();
            break;
        case TG_MIC: setMicOn = on; break;
        case TG_DOF6: setDof6 = on; break;
        case TG_DEBUG_HUD: setDbg = on; break;
        default: break;
        }
        ++setVer;
    }
    void settingsSlider(int id, float v) override {
        if (id == SL_VOL) setVol = v;
        else if (id == SL_BRI) setBri = v;
        else if (id == SL_IPD) setIpd = v;
        ++setVer;
    }
    void settingsText(int id, const std::string& v) override {
        if (id == TX_THEME) setTheme = v;
        else if (id == TX_ENV) setEnv = v;
        else if (id == TX_LANG) setLang = v;
        ++setVer;
    }
    void settingsAction(int id) override {
        if (id == ACT_CTRL_UNBIND) {
            setCtrl[0].state = setCtrl[1].state = 0;
            setCtrl[0].mac = setCtrl[1].mac = "";
        }
        ++setVer;
    }
    void settingsScan(int sec) override {
        if (sec == SEC_WIFI && setWifiOn) {
            setWifiScan = true;
            setWifi.clear();
            SetWifi a; a.ssid = "PreviewNet"; a.sec = WSEC_WPA;
            a.level = 3; a.conn = true; a.saved = 0;
            SetWifi b; b.ssid = "CafeGuest"; b.sec = WSEC_OPEN;
            b.level = 1;
            SetWifi c2; c2.ssid = "Office-5G"; c2.sec = WSEC_WPA;
            c2.level = 4;
            setWifi.push_back(a); setWifi.push_back(b);
            setWifi.push_back(c2);
            setWifiScan = false;
        } else if (sec == SEC_BT && setBtOn) {
            setBtScan = true;
            setBt.clear();
            SetBtDev a; a.name = "Preview Pods";
            a.addr = "AA:BB:CC:DD:EE:01"; a.bonded = true; a.conn = true;
            SetBtDev b; b.name = "KB-380"; b.addr = "AA:BB:CC:DD:EE:02";
            setBt.push_back(a); setBt.push_back(b);
            setBtScan = false;
        }
        ++setVer;
    }
    void settingsToggleScan() override {
        setCtrlScan = !setCtrlScan;
        setCtrl[0].state = setCtrlScan ? 2 : (setCtrl[0].mac.empty() ? 0 : 1);
        setCtrl[1].state = setCtrlScan ? 2 : (setCtrl[1].mac.empty() ? 0 : 1);
        ++setVer;
    }
    void wifiJoin(const std::string& ssid, int sec,
                  const std::string&) override {
        for (size_t i = 0; i < setWifi.size(); ++i) {
            setWifi[i].conn = setWifi[i].ssid == ssid;
            if (setWifi[i].conn) setWifi[i].saved = (int)i;
        }
        setSsid = ssid;
        ++setVer;
    }
    void wifiForget(int netId) override {
        for (auto& n : setWifi)
            if (n.saved == netId) n.saved = -1;
        if (netId < (int)setWifi.size() && setWifi[netId].ssid == setSsid) {
            for (auto& n : setWifi) if (n.ssid == setSsid) n.conn = false;
            setSsid = "";
        }
        ++setVer;
    }
    void btPair(const std::string& addr) override {
        for (auto& d : setBt)
            if (d.addr == addr) { d.bonded = true; d.conn = true; }
        ++setVer;
    }
    void btUnpair(const std::string& addr) override {
        for (auto& d : setBt)
            if (d.addr == addr) { d.bonded = false; d.conn = false; }
        ++setVer;
    }
    void imePick(const std::string& id) override {
        for (auto& i : setImes) i.active = i.id == id;
        ++setVer;
    }
    void envDelete(const std::string& id) override {
        for (size_t i = 0; i < setEnvs.size(); ++i)
            if (setEnvs[i].id == id) {
                setEnvs.erase(setEnvs.begin() + i);
                break;
            }
        ++setVer;
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

    int setVer = 1;
    int setSec = -1;
    bool setWifiOn = true, setBtOn = true, setMicOn = true;
    bool setDof6 = false, setDbg = false;
    bool setWifiScan = false, setBtScan = false, setCtrlScan = false;
    float setVol = 0.6f, setBri = 0.8f, setIpd = 63.5f;
    std::string setSsid = "PreviewNet", setTheme = "dark";
    std::string setEnv = "builtin", setLang = "en";
    std::vector<SetWifi> setWifi;
    std::vector<SetBtDev> setBt;
    std::vector<SetImeOpt> setImes;
    std::vector<SetEnvOpt> setEnvs;
    SetCtrl setCtrl[2];

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
    const bool headless = argFlag(argc, argv, "-headless");
    const bool mcp = headless || argFlag(argc, argv, "-mcp");

    DashApp app;
    if (headless) {
        if (!dashHeadless(&app, 1440, 810)) return 1;
    } else if (!dashWindow(&app, "Hibiscus dash preview", 1440, 810))
        return 1;

    HudEngine hud;
    PreviewBackend backend;
    app.hud = &hud;
    app.kbdSet = [&backend](bool on) { backend.kbdShow(on); };
    app.extraKey = [&hud, &backend](int key, int, int action) {
        if (key == GLFW_KEY_K && action == GLFW_PRESS)
            backend.kbdShow(!backend.kbdState().shown);
        if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            if (hud.set.shown) setClose(&hud);
            else setOpen(&hud, SEC_WIFI);
        }
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
    long long lastDraw = 0;
    while (dashAlive(&app)) {
        if (app.headless)
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        else
            glfwWaitEventsTimeout(0.012);
        dashDrainPre();
        const long long now = dashNowMs();
        const float dt = (now - last) / 1000.0f;
        last = now;
        dashCamTick(&app, dt);
        const Mat4 view = camView(app.cam);
        tickDash(&hud, view, now);

        if (dashDrawDue(now, &lastDraw, 14)) {
            dashTarget(&app, &hud);
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
            dashPresent(&app);
        }
    }
    dashStop(&app);
    return 0;
}
