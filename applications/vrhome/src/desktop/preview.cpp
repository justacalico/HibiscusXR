#define VRHOME_DESKTOP 1

#include "harness.h"
#include "cmdline.h"
#include "vr.h"

#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../hud/dash.h"
#include "../hud/engine.h"
#include "../input/input.h"
#include "../input/keys.h"
#include "../panels/panel.h"
#include "../render/icons.h"
#include "../render/iconatlas.h"
#include "../render/resources.h"
#include "../shell/backend.h"
#include "../set/set.h"
#include "../../third_party/stb_truetype.h"

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
        makePanelTex(tex, p->displayId);
        return p->displayId;
    }

    bool resizePanel(Panel*, int, int, int) override { return true; }

    void updatePanel(Panel* p) override {
        p->stMat[0] = p->stMat[5] = p->stMat[10] = p->stMat[15] = 1.0f;
    }

    bool createKbdSurface(Kbd* k, unsigned tex, int w, int h, int) override {
        k->st = this;
        makeKbdTex(tex, w, h, eng);
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
    bool takeGroups(std::vector<LibGroup>* out) override {
        if (groupsSent) return false;
        groupsSent = true;
        *out = groups;
        return true;
    }
    void setGroups(const std::vector<LibGroup>& g) override {
        groups = g;
    }
    int appsVersion() override { return 1; }
    void launcherApps(std::vector<GridItem>* out) override {
        static const char* names[] = {
            "Browser", "Files", "Calculator", "Store", "Notes",
            "Camera", "Music", "Video", "Terminal", "Maps",
            "Calendar", "Photos", "Mail", "Chat", "Games",
            "Weather", "Clock", "Books", "Radio", "Podcasts",
            "Recorder", "Gallery", "Contacts", "Compass",
            "Flashlight", "Editor",
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
        out->xp = setXp;
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
    void xpToggle(const std::string& id, bool on) override {
        for (auto& f : setXp)
            if (f.id == id) { f.on = on; break; }
        ++setVer;
    }

    std::string appLabel(const std::string& pkg) override {
        const size_t dot = pkg.rfind('.');
        std::string s =
            dot == std::string::npos ? pkg : pkg.substr(dot + 1);
        if (!s.empty()) s[0] = (char)toupper((unsigned char)s[0]);
        return s;
    }
    bool iconPixels(const std::string& pkg, IconPixels* out) override {
        // per-app launcher art: a tinted tile with the app's atlas glyph -
        // the same language real icon packs speak, not a debug blob
        static const struct { const char* app; int glyph; } kMap[] = {
            {"browser", GI_LANGUAGE},    {"files", GI_FOLDER},
            {"calculator", GI_CALCULATOR},{"store", GI_BAG},
            {"notes", GI_EDIT},          {"camera", GI_CAMERA},
            {"music", GI_MUSIC_NOTE},    {"video", GI_PLAY},
            {"terminal", GI_TERMINAL},   {"maps", GI_PLACE},
            {"calendar", GI_CALENDAR},   {"photos", GI_IMAGE},
            {"mail", GI_MAIL},           {"chat", GI_CHAT},
            {"games", GI_STADIA_CONTROLLER},
            {"weather", GI_SUNNY},       {"clock", GI_SCHEDULE},
            {"books", GI_BOOK},          {"radio", GI_VOLUME_UP},
            {"podcasts", GI_MIC},        {"recorder", GI_MIC},
            {"gallery", GI_LANDSCAPE},   {"contacts", GI_PERSON},
            {"compass", GI_EXPLORE},     {"flashlight", GI_FLASHLIGHT},
            {"editor", GI_EDIT},         {"settings", GI_SETTINGS},
            {"messages", GI_CHAT},
        };
        const size_t dot = pkg.rfind('.');
        std::string app = dot == std::string::npos
            ? pkg : pkg.substr(dot + 1);
        for (char& c2 : app) c2 = (char)tolower((unsigned char)c2);
        int glyph = -1;
        for (const auto& m : kMap)
            if (app == m.app) { glyph = m.glyph; break; }
        unsigned h = 0;
        for (char c : pkg) h = h * 31 + (unsigned char)c;
        const unsigned char cols[][3] = {
            {66, 133, 244}, {148, 96, 220}, {30, 150, 160},
            {210, 80, 88},  {56, 160, 100}, {225, 150, 55},
            {215, 100, 130},{90, 120, 200},
        };
        const unsigned char* c = cols[h % 8];
        out->w = out->h = 96;
        out->rgba.resize(96 * 96 * 4);
        float guv[4] = {0};
        const bool hasGlyph = glyph >= 0 && iconUv(glyph, guv);
        const int gw = iconAtlasW(), gh = iconAtlasH();
        const unsigned char* atlas = iconAtlas();
        // the cell's ink sits in its middle ~60%: drawing the cell over the
        // tile's middle 70% lands the ink at ~half the icon
        const float inset = 96.0f * 0.15f, span = 96.0f - inset * 2.0f;
        for (int y = 0; y < 96; ++y)
            for (int x = 0; x < 96; ++x) {
                unsigned char* p = out->rgba.data() + (y * 96 + x) * 4;
                const float t = y / 95.0f;
                const float lit = 1.14f - 0.30f * t;
                p[0] = (unsigned char)(c[0] * lit > 255 ? 255 : c[0] * lit);
                p[1] = (unsigned char)(c[1] * lit > 255 ? 255 : c[1] * lit);
                p[2] = (unsigned char)(c[2] * lit > 255 ? 255 : c[2] * lit);
                p[3] = 255;
                if (!hasGlyph) continue;
                const float fx = (x - inset) / span, fy = (y - inset) / span;
                if (fx < 0.0f || fx >= 1.0f || fy < 0.0f || fy >= 1.0f)
                    continue;
                const float u = guv[0] + fx * (guv[2] - guv[0]);
                const float v = guv[1] + fy * (guv[3] - guv[1]);
                const int ax = (int)(u * gw), ay = (int)(v * gh);
                if (ax < 0 || ax >= gw || ay < 0 || ay >= gh) continue;
                const int a2 = atlas[ay * gw + ax];
                p[0] += (unsigned char)((245 - p[0]) * a2 / 255);
                p[1] += (unsigned char)((247 - p[1]) * a2 / 255);
                p[2] += (unsigned char)((255 - p[2]) * a2 / 255);
            }
        return true;
    }
    void launchPackageOn(const std::string& pkg, int displayId) override {
        LOGI("preview launch %s on display %d", pkg.c_str(), displayId);
    }
    void injectTouch(int displayId, float x, float y, int action) override {
        LOGI("touch disp=%d %.0f,%.0f action=%d", displayId, x, y, action);
    }

    Engine* eng = nullptr;

private:
    int nextDisp = 100;
    bool pinsSent = false;
    bool groupsSent = false;
    std::vector<LibGroup> groups;
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
    std::vector<SetXpOpt> setXp = {
        {"preview_flag", "Preview flag", "Fixture experimental toggle", true},
        {"preview_labs", "Preview labs", "Fixture experimental toggle", false},
    };
    SetCtrl setCtrl[2];

    static void texParams(unsigned tex) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // a plausible app surface so the preview windows read like real apps:
    // app bar with an accent icon, a hero block and a few list rows. The
    // accent cycles by display so the ring's windows don't look identical
    static void makePanelTex(unsigned tex, int dispId) {
        const int w = 640, h = 360;
        std::vector<unsigned char> px(w * h * 4);
        static const unsigned char kAcc[][3] = {
            {66, 133, 244}, {52, 168, 83}, {171, 71, 188},
            {0, 172, 193}, {234, 88, 96}, {240, 164, 60},
        };
        const unsigned char* a = kAcc[((dispId % 6) + 6) % 6];
        auto box = [&](int x0, int y0, int bw, int bh, int rad,
                       unsigned char r, unsigned char g, unsigned char b) {
            for (int y = y0; y < y0 + bh; ++y)
                for (int x = x0; x < x0 + bw; ++x) {
                    if (x < 0 || y < 0 || x >= w || y >= h) continue;
                    const int qx = x < x0 + rad ? x0 + rad - x
                                     : x > x0 + bw - 1 - rad
                                       ? x - (x0 + bw - 1 - rad) : 0;
                    const int qy = y < y0 + rad ? y0 + rad - y
                                     : y > y0 + bh - 1 - rad
                                       ? y - (y0 + bh - 1 - rad) : 0;
                    if (qx * qx + qy * qy > rad * rad) continue;
                    unsigned char* p = px.data() + (y * w + x) * 4;
                    p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
                }
        };
        for (int i = 0; i < w * h; ++i) {
            px[i * 4 + 0] = 22; px[i * 4 + 1] = 25; px[i * 4 + 2] = 33;
            px[i * 4 + 3] = 255;
        }
        box(0, 0, w, 46, 0, 30, 34, 45);
        box(18, 14, 18, 18, 9, a[0], a[1], a[2]);
        box(46, 17, 130, 12, 6, 62, 68, 86);
        box(w - 110, 15, 92, 16, 8, 46, 51, 66);
        box(18, 60, w - 36, 96, 10,
            (unsigned char)(28 + a[0] / 5),
            (unsigned char)(30 + a[1] / 5),
            (unsigned char)(38 + a[2] / 5));
        box(36, 84, 170, 14, 7, 90, 96, 120);
        box(36, 108, 110, 10, 5, 64, 70, 88);
        for (int r = 0; r < 3; ++r) {
            const int ry = 172 + r * 58;
            box(18, ry, w - 36, 48, 8, 30, 34, 45);
            box(32, ry + 13, 22, 22, 11,
                (unsigned char)(a[0] * 2 / 3),
                (unsigned char)(a[1] * 2 / 3),
                (unsigned char)(a[2] * 2 / 3));
            box(66, ry + 12, 200, 10, 5, 74, 80, 98);
            box(66, ry + 28, 140, 8, 4, 52, 57, 73);
        }
        texParams(tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px.data());
    }

    // an actual keyboard layout: lettered caps rasterized with the ui
    // font via stbtt so keys read as a real IME, not a gray grid
    static void makeKbdTex(unsigned tex, int w, int h, Engine* e) {
        std::vector<unsigned char> px(w * h * 4);
        for (int i = 0; i < w * h; ++i) {
            px[i * 4 + 0] = 18; px[i * 4 + 1] = 20; px[i * 4 + 2] = 27;
            px[i * 4 + 3] = 255;
        }
        struct KRow { const char* keys; int n; float pad; };
        const KRow rows[] = {
            {"1234567890", 10, 0.02f},
            {"qwertyuiop", 10, 0.06f},
            {"asdfghjkl;", 10, 0.10f},
            {"zxcvbnm,.?", 10, 0.16f},
        };
        const float keyH = h / 5.0f;
        const bool haveFont = e && e->font.ok && !e->font.data.empty();
        auto glyph = [&](int cx, int cy, int cp, float pxh) {
            if (!haveFont) return;
            const float sc =
                stbtt_ScaleForPixelHeight(&e->font.info, pxh);
            int bw, bh, xo, yo;
            unsigned char* bm = stbtt_GetCodepointBitmap(
                &e->font.info, sc, sc, cp, &bw, &bh, &xo, &yo);
            for (int y = 0; y < bh; ++y)
                for (int x = 0; x < bw; ++x) {
                    const int tx = cx + xo + x, ty = cy + yo + y;
                    if (tx < 0 || ty < 0 || tx >= w || ty >= h) continue;
                    unsigned char* p = px.data() + (ty * w + tx) * 4;
                    const int a = bm[y * bw + x];
                    p[0] += (unsigned char)((235 - p[0]) * a / 255);
                    p[1] += (unsigned char)((237 - p[1]) * a / 255);
                    p[2] += (unsigned char)((245 - p[2]) * a / 255);
                }
            stbtt_FreeBitmap(bm, nullptr);
        };
        auto label = [&](int cx, int cy, const char* s, float pxh) {
            const float sc = stbtt_ScaleForPixelHeight(&e->font.info, pxh);
            float pen = 0.0f;
            for (const char* q = s; *q; ++q) {
                int adv;
                stbtt_GetCodepointHMetrics(&e->font.info, *q, &adv,
                                           nullptr);
                pen += adv * sc;
            }
            pen = -pen * 0.5f;
            for (const char* q = s; *q; ++q) {
                int adv;
                stbtt_GetCodepointHMetrics(&e->font.info, *q, &adv,
                                           nullptr);
                int bw, bh, xo, yo;
                unsigned char* bm = stbtt_GetCodepointBitmap(
                    &e->font.info, sc, sc, *q, &bw, &bh, &xo, &yo);
                for (int y = 0; y < bh; ++y)
                    for (int x = 0; x < bw; ++x) {
                        const int tx = cx + (int)pen + xo + x;
                        const int ty = cy + yo + y;
                        if (tx < 0 || ty < 0 || tx >= w || ty >= h)
                            continue;
                        unsigned char* p = px.data() + (ty * w + tx) * 4;
                        const int a = bm[y * bw + x];
                        p[0] += (unsigned char)((235 - p[0]) * a / 255);
                        p[1] += (unsigned char)((237 - p[1]) * a / 255);
                        p[2] += (unsigned char)((245 - p[2]) * a / 255);
                    }
                stbtt_FreeBitmap(bm, nullptr);
                pen += adv * sc;
            }
        };
        auto key = [&](float x0, float x1, float yc, int cp,
                       const char* lbl) {
            const int kx0 = (int)(x0 * w) + 5, kx1 = (int)(x1 * w) - 5;
            const int ky0 = (int)(yc * h - keyH * 0.40f);
            const int ky1 = (int)(yc * h + keyH * 0.40f);
            const int rad = 9;
            for (int y = ky0; y < ky1; ++y)
                for (int x = kx0; x < kx1; ++x) {
                    const int qx = x < kx0 + rad ? kx0 + rad - x
                                     : x >= kx1 - rad ? x - kx1 + rad + 1 : 0;
                    const int qy = y < ky0 + rad ? ky0 + rad - y
                                     : y >= ky1 - rad ? y - ky1 + rad + 1 : 0;
                    if (qx * qx + qy * qy > rad * rad || x < 0 || y < 0 ||
                        x >= w || y >= h) continue;
                    unsigned char* p = px.data() + (y * w + x) * 4;
                    p[0] = 52; p[1] = 56; p[2] = 68;
                }
            const int cx = (kx0 + kx1) / 2, cy = (ky0 + ky1) / 2;
            if (lbl) label(cx, cy, lbl, keyH * 0.34f);
            else if (cp > 0) glyph(cx, cy, cp, keyH * 0.40f);
        };
        for (int r = 0; r < 4; ++r) {
            const float yc = (r + 0.5f) / 5.0f;
            const float kw = (1.0f - rows[r].pad * 2.0f) / rows[r].n;
            for (int k = 0; k < rows[r].n; ++k)
                key(rows[r].pad + k * kw, rows[r].pad + (k + 1) * kw,
                    yc, rows[r].keys[k], nullptr);
        }
        const float yc = 4.5f / 5.0f;
        key(0.00f, 0.14f, yc, 0, "?123");
        key(0.14f, 0.14f + 0.52f, yc, ' ', nullptr);
        key(0.66f, 0.80f, yc, 0, ".");
        key(0.80f, 1.00f, yc, 0, "done");
        texParams(tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px.data());
    }
};

int main(int argc, char** argv) {
    const bool headless = argFlag(argc, argv, "-headless");
    const bool mcp = headless || argFlag(argc, argv, "-mcp");
    const bool vr = argFlag(argc, argv, "-vr");

    DashApp app;
    VrDash xr;
    if (vr) {
        if (!vrInit(&xr, "vrdash-preview")) return 1;
    } else if (headless) {
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
    backend.eng = &hud;
    hud.haveQuat = true;
    hud.dockAnchored = false;
    hud.summonMs = dashNowMs();
    if (initGlResources(&hud) != 0) return 1;
    loadGlyphs(&hud);
    queueLaunch("preview.browser");
    queueLaunch("preview.files");
    queueLaunch("preview.settings");

    if (mcp) dashStartMcp(&app);

    if (vr) {
        while (!app.quit) {
            const int st = vrPoll(&xr);
            if (st < 0) break;
            dashDrainPre();
            if (!st) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }
            const bool draw = vrBeginFrame(&xr);
            vrInput(&xr, &hud);
            const long long now = dashNowMs();
            if (takeWantRecenter()) wantRecenter();
            tickDash(&hud, vrView(xr.head), now);
            if (draw) {
                for (int eye = 0; eye < kVrEyes; ++eye) {
                    Mat4 vp;
                    if (!vrTarget(&xr, eye, &vp)) continue;
                    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    drawDashScene(&hud, vp);
                }
            }
            dashDrainPost();
            vrEnd(&xr);
        }
        vrShutdown(&xr);
        dashStop(&app);
        return 0;
    }

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
