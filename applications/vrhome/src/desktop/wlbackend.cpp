#include "wlbackend.h"

#include "cmdline.h"
#include "proctrack.h"

#include "harness.h"
#include "icontheme.h"
#include "sysinfo.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../grid/layout.h"
#include "../panels/panel.h"
#include "../quick/item.h"

#include <GLES2/gl2.h>
#include <png.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <unistd.h>

// the About card's Hibiscus version fallback when the host has no
// /etc/hibiscus-release - baked in by the Makefile from `git describe`
#ifndef HIBISCUS_VERSION
#define HIBISCUS_VERSION "dev"
#endif

namespace fs = std::filesystem;

// a pending launch slot with no spawn recorded gets this long to bind a
// task before its panel is treated as dead; spawned slots die with the
// process instead
static const long long kSlotStaleMs = 10000;

static std::string runOut(const char* cmd) {
    FILE* p = popen(cmd, "r");
    if (!p) return "";
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);
    return out;
}

static void runBg(const std::string& cmd) {
    const pid_t pid = fork();
    if (pid != 0) return;
    setsid();
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
    _exit(127);
}

static bool readFile(const std::string& path, std::string* out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[64];
    out->clear();
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
    fclose(f);
    return true;
}

static std::string configDir() {
    const char* x = getenv("XDG_CONFIG_HOME");
    if (x && *x) return x;
    const char* h = getenv("HOME");
    return h ? std::string(h) + "/.config" : "/tmp";
}

static bool decodePng(const std::string& path, IconPixels* out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                             nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);
    if (!png || !info || setjmp(png_jmpbuf(png))) {
        if (png) png_destroy_read_struct(&png, &info, nullptr);
        fclose(f);
        return false;
    }
    png_init_io(png, f);
    png_read_info(png, info);
    const int w = (int)png_get_image_width(png, info);
    const int h = (int)png_get_image_height(png, info);
    png_set_expand(png);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_interlace_handling(png);
    png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
    png_read_update_info(png, info);
    if (png_get_channels(png, info) != 4) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(f);
        return false;
    }
    out->w = w; out->h = h;
    out->rgba.resize((size_t)w * h * 4);
    std::vector<png_bytep> rows(h);
    for (int y = 0; y < h; ++y)
        rows[y] = out->rgba.data() + (size_t)y * w * 4;
    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(f);
    return true;
}

WlBackend::WlBackend(WlComp* comp) : c(comp) {
    c->onTaskMap = [this](WlTask* t) { onTaskMap(t); };
    c->onTaskDead = [this](WlTask* t) { onTaskDead(t); };
    const char* lang = getenv("LANG");
    if (lang && *lang) {
        setLang = lang;
        const size_t dot = setLang.find('.');
        if (dot != std::string::npos) setLang.resize(dot);
        for (char& ch : setLang)
            if (ch == '_') ch = '-';
    }
    rescanApps();
}

WlBackend::~WlBackend() = default;

WlBackend::Slot* WlBackend::slotOf(int displayId) {
    auto it = slots.find(displayId);
    return it == slots.end() ? nullptr : &it->second;
}

WlTask* WlBackend::taskOf(int taskId) {
    for (WlTask* t : c->tasks)
        if (t->taskId == taskId) return t;
    return nullptr;
}

void WlBackend::rescanApps() {
    apps = scanDesktopFiles(desktopAppDirs());
    byId.clear(); byClass.clear();
    for (const DeskEntry& e : apps) {
        byId[e.id] = &e;
        if (!e.wmClass.empty()) byClass[e.wmClass] = e.id;
    }
    ++appsVer;
}

void WlBackend::pollSys() {
    const long long now = dashNowMs();
    if (now - sysPollAt < 2000) return;
    sysPollAt = now;
    const std::string w =
        runOut("nmcli -t -f active,ssid dev wifi 2>/dev/null");
    sysSsid = parseWifiSsid(w);
    sysWifi = sysSsid.empty() ? 0 : 1;
    const std::string devs =
        runOut("nmcli -t -f type,state dev 2>/dev/null");
    sysEth = parseEthLink(devs) ? 1 : 0;
    // -1 survives only when no BAT* entry exists: a power_supply node
    // whose capacity won't parse still counts as a battery at 0%
    sysBatt = -1; sysChg = 0;
    std::error_code ec;
    for (const auto& de :
         fs::directory_iterator("/sys/class/power_supply", ec)) {
        if (de.path().filename().string().rfind("BAT", 0) != 0)
            continue;
        std::string cap, st;
        readFile(de.path().string() + "/capacity", &cap);
        readFile(de.path().string() + "/status", &st);
        const int pct = parseCapacity(cap);
        sysBatt = pct < 0 ? 0 : pct;
        sysChg = parseCharging(st) ? 1 : 0;
        break;
    }
    const std::string v =
        runOut("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null");
    sysVol = parseWpctlMuted(v) ? 0.0f : parseWpctlVol(v);
    if (sysVol < 0.0f) sysVol = 0.0f;
    // -c backlight: unfiltered -m can lead with a keyboard led
    sysBri = parseBrightness(
        runOut("brightnessctl -m -c backlight 2>/dev/null"));
    if (sysBri < 0.0f) sysBri = 0.5f;
}

const DeskEntry* WlBackend::entry(const std::string& pkg) {
    auto it = byId.find(pkg);
    if (it != byId.end()) return it->second;
    const std::string id = pkg + ".desktop";
    it = byId.find(id);
    return it == byId.end() ? nullptr : it->second;
}

void WlBackend::onTaskMap(WlTask* t) {
    // prefer the slot whose spawned pid matches, then the slot waiting
    // on this wmClass; anything else adopts onto a fresh panel
    for (auto& kv : slots)
        if (kv.second.task == nullptr &&
                spawnPidMatch(t->pid, getpgid(t->pid),
                              kv.second.pid)) {
            kv.second.task = t;
            t->displayId = kv.first;
            c->configureTask(t, kv.second.w, kv.second.h);
            LOGI("task %d -> disp %d (pid %d)", t->taskId, kv.first,
                 (int)t->pid);
            return;
        }
    for (auto& kv : slots) {
        Slot& s = kv.second;
        if (s.task) continue;
        if (appIdMatch(t->appId, s.wmClass, s.deskId)) {
            s.task = t;
            t->displayId = kv.first;
            c->configureTask(t, s.w, s.h);
            LOGI("task %d -> disp %d (appid %s)", t->taskId, kv.first,
                 t->appId.c_str());
            return;
        }
    }
    PendingPanel p;
    p.taskId = t->taskId;
    p.pkg = t->appId.empty() ? "wayland.app" : t->appId;
    adopts.push_back(p);
}

void WlBackend::onTaskDead(WlTask* t) {
    if (t->displayId >= 0) {
        Slot* s = slotOf(t->displayId);
        if (s && s->task == t) s->task = nullptr;
        releases.push_back(t->displayId);
        t->displayId = -1;
        return;
    }
    // an orphan died before adoption: drop its pending record
    for (auto it = adopts.begin(); it != adopts.end(); ++it)
        if (it->taskId == t->taskId) { adopts.erase(it); break; }
}

int WlBackend::createPanel(Panel* p, unsigned tex, int w, int h, int) {
    const int id = nextDisp++;
    Slot& s = slots[id];
    s.displayId = id;
    s.tex = tex;
    s.w = w;
    s.h = h;
    s.texW = w;
    s.texH = h;
    s.bornMs = dashNowMs();
    p->displayId = id;
    p->st = &slots[id];
    glBindTexture(GL_TEXTURE_2D, tex);
    std::vector<unsigned char> dark((size_t)w * h * 4);
    for (size_t i = 0; i < dark.size(); i += 4) {
        dark[i] = 26; dark[i + 1] = 28; dark[i + 2] = 34;
        dark[i + 3] = 255;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, dark.data());
    return id;
}

bool WlBackend::resizePanel(Panel* p, int w, int h, int) {
    Slot* s = slotOf(p->displayId);
    if (!s) return false;
    s->w = w;
    s->h = h;
    if (s->task) c->configureTask(s->task, w, h);
    return true;
}

void WlBackend::releasePanel(Panel* p) {
    Slot* s = slotOf(p->displayId);
    if (!s) return;
    if (s->task) {
        c->requestClose(s->task);
        s->task->displayId = -1;
    }
    slots.erase(p->displayId);
    p->st = nullptr;
}

void WlBackend::updatePanel(Panel* p) {
    // shm rows are top-down but the quad's v runs bottom-up: flip V the
    // way the android surface transform does for its OES feeds
    p->stMat[0] = 1.0f;
    p->stMat[5] = -1.0f;
    p->stMat[10] = 1.0f;
    p->stMat[13] = 1.0f;
    p->stMat[15] = 1.0f;
    Slot* s = (Slot*)p->st;
    if (!s) return;
    WlTask* t = s->task;
    if (!t) {
        // a pending slot dies as soon as its spawn (and its process
        // group, for wrappers that fork) is gone; slots that never got
        // a spawn keep a plain timeout
        if (s->pid > 0 ? spawnGone(s->pid)
                      : dashNowMs() - s->bornMs > kSlotStaleMs)
            releases.push_back(p->displayId);
        return;
    }
    if (!t->mapped || !t->dirty) return;
    if (c->taskGpuBind(t, p->tex)) { t->dirty = false; return; }
    // commits only dirtied rects: repaint the union once per update,
    // then push just the changed rows - GLES2 has no unpack row
    // length, so sub uploads run at full texture width
    c->recompose(t);
    if (!t->dirty || t->img.rgba.empty()) return;
    glBindTexture(GL_TEXTURE_2D, p->tex);
    if (s->texW != t->img.w || s->texH != t->img.h) {
        s->texW = t->img.w;
        s->texH = t->img.h;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->img.w, t->img.h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, t->img.rgba.data());
    } else {
        int y0 = t->dy0, y1 = t->dy1;
        if (y0 < 0) y0 = 0;
        if (y1 > t->img.h) y1 = t->img.h;
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y0, t->img.w, y1 - y0,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        t->img.rgba.data() + (size_t)y0 * t->img.w * 4);
    }
    t->dirty = false;
}

bool WlBackend::takePins(std::vector<std::string>* out) {
    if (pinsSent) return false;
    pinsSent = true;
    out->clear();
    std::string text;
    if (readFile(configDir() + "/hibiscus/dash-pins", &text)) {
        size_t at = 0;
        while (at <= text.size()) {
            const size_t eol = text.find('\n', at);
            const std::string line = text.substr(at,
                eol == std::string::npos ? eol : eol - at);
            at = eol == std::string::npos ? text.size() + 1 : eol + 1;
            if (!line.empty() && entry(line)) out->push_back(line);
        }
    }
    if (out->empty()) {
        const char* want[] = {"firefox.desktop", "foot.desktop",
                              "org.gnome.Nautilus.desktop"};
        for (const char* w : want)
            if (entry(w)) out->push_back(w);
    }
    return true;
}

void WlBackend::setPins(const std::vector<std::string>& pins) {
    const std::string dir = configDir() + "/hibiscus";
    std::error_code ec;
    fs::create_directories(dir, ec);
    FILE* f = fopen((dir + "/dash-pins").c_str(), "wb");
    if (!f) return;
    for (const std::string& p : pins) fprintf(f, "%s\n", p.c_str());
    fclose(f);
}

// the library's groups persist next to the pins in the same encoding the
// android bridge uses: one "name<TAB>pkg,pkg" line per group
bool WlBackend::takeGroups(std::vector<LibGroup>* out) {
    if (groupsSent) return false;
    groupsSent = true;
    out->clear();
    std::string text;
    if (readFile(configDir() + "/hibiscus/dash-groups", &text)) {
        size_t at = 0;
        while (at <= text.size()) {
            const size_t eol = text.find('\n', at);
            const std::string line = text.substr(at,
                eol == std::string::npos ? eol : eol - at);
            at = eol == std::string::npos ? text.size() + 1 : eol + 1;
            LibGroup g;
            if (groupDecode(line, &g)) out->push_back(g);
        }
    }
    groups = *out;
    return true;
}

void WlBackend::setGroups(const std::vector<LibGroup>& g) {
    groups = g;
    const std::string dir = configDir() + "/hibiscus";
    std::error_code ec;
    fs::create_directories(dir, ec);
    FILE* f = fopen((dir + "/dash-groups").c_str(), "wb");
    if (!f) return;
    for (const LibGroup& gr : g)
        fprintf(f, "%s\n", groupEncode(gr).c_str());
    fclose(f);
}

void WlBackend::sysStatus(int out[3]) {
    pollSys();
    out[0] = sysEth ? DNET_ETH : sysWifi ? DNET_WIFI : DNET_NONE;
    out[1] = sysBatt; out[2] = sysChg;
}

bool WlBackend::iconPixels(const std::string& pkg, IconPixels* out) {
    const DeskEntry* e = entry(pkg);
    const std::string icon =
        e && !e->icon.empty() ? e->icon : pkg;
    if (icon.front() == '/' && decodePng(icon, out)) return true;
    static const char* kThemes[] = {"Papirus", "Adwaita"};
    const std::vector<std::string> themes(std::begin(kThemes),
                                          std::end(kThemes));
    // libpng only: svg/xpm icons fall through to the generated look
    const std::string p = findIconFile(iconSearchRoots(), themes,
                                       icon, 96, ".png");
    return !p.empty() && decodePng(p, out);
}

std::string WlBackend::appLabel(const std::string& pkg) {
    const DeskEntry* e = entry(pkg);
    if (e) return e->name;
    auto it = byClass.find(pkg);
    if (it != byClass.end() && byId.count(it->second))
        return byId[it->second]->name;
    return pkg;
}

int WlBackend::appsVersion() {
    if (dashNowMs() - appsScanAt > 3000) {
        appsScanAt = dashNowMs();
        rescanApps();
    }
    return appsVer;
}

void WlBackend::launcherApps(std::vector<GridItem>* out) {
    out->clear();
    for (const DeskEntry& e : apps) {
        GridItem it;
        it.pkg = e.id;
        it.label = e.name;
        out->push_back(it);
    }
}

void WlBackend::uiStrings(std::vector<std::string>* out) {
    *out = {
        "Library", "Quick settings", "On", "Off", "Not connected",
        "Wi-Fi", "Boundary", "Bluetooth", "Seethrough", "Microphone",
        "Reset view", "Night mode", "Do Not Disturb", "Airplane mode",
        "Battery saver", "Report problem", "About device",
        "Ethernet",
    };
}

void WlBackend::quickStatus(QsStatus* out) {
    pollSys();
    out->toggles = qsMask;
    if (sysWifi) out->toggles |= (1u << QS_WIFI);
    out->net = sysEth ? DNET_ETH : sysWifi ? DNET_WIFI : DNET_NONE;
    // vol/brightness ride the 2s pollSys cache: spawning the probes
    // every tick was hundreds of forks a second with the panel open
    out->volume = sysVol;
    out->brightness = sysBri;
    out->batt = sysBatt;
    out->chg = sysChg;
    out->ssid = sysSsid;
}

void WlBackend::quickToggle(int id, bool on) {
    if (on) qsMask |= (1u << id); else qsMask &= ~(1u << id);
    if (id == QS_WIFI)
        runBg(std::string("nmcli radio wifi ") + (on ? "on" : "off"));
    else if (id == QS_BLUETOOTH)
        runBg(std::string("bluetoothctl power ") + (on ? "on" : "off"));
}

void WlBackend::quickSetSlider(int slider, float v) {
    if (slider == QSLIDER_VOLUME) {
        sysVol = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
        char cmd[128];
        snprintf(cmd, sizeof(cmd),
                 "wpctl set-volume @DEFAULT_AUDIO_SINK@ %.2f", sysVol);
        runBg(cmd);
    } else if (slider == QSLIDER_BRIGHTNESS) {
        sysBri = v;
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "brightnessctl set %.0f%%",
                 v * 100.0f);
        runBg(cmd);
    }
}

// -------------------------------------------------------------- settings -

void WlBackend::pollSet() {
    const long long now = dashNowMs();
    if (now - setPollAt < 2000) return;
    setPollAt = now;
    ++setVer;
    unsigned t = 0;
    if (parseNmRadio(runOut("nmcli radio wifi 2>/dev/null")))
        t |= 1u << TG_WIFI;
    if (parseBtPowered(runOut("bluetoothctl show 2>/dev/null")))
        t |= 1u << TG_BT;
    if (!parseWpctlMuted(
            runOut("wpctl get-volume @DEFAULT_AUDIO_SOURCE@ 2>/dev/null")))
        t |= 1u << TG_MIC;
    setSnap.toggles = t;
    const std::string v =
        runOut("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null");
    setSnap.sld[SL_VOL] =
        parseWpctlMuted(v) ? 0.0f : parseWpctlVol(v);
    if (setSnap.sld[SL_VOL] < 0.0f) setSnap.sld[SL_VOL] = 0.0f;
    setSnap.sld[SL_BRI] =
        parseBrightness(runOut("brightnessctl -m -c backlight 2>/dev/null"));
    if (setSnap.sld[SL_BRI] < 0.0f) setSnap.sld[SL_BRI] = 0.5f;
    setSnap.sld[SL_IPD] = kIpdMinMm;
    setCons = parseConNames(
        runOut("nmcli -t -f name con show 2>/dev/null"));
    parseWifiList(
        runOut("nmcli -t -f active,ssid,signal,security dev wifi "
               "2>/dev/null"),
        setCons, &setSnap.wifi);
    parseBtDevs(runOut("bluetoothctl devices 2>/dev/null"),
                runOut("bluetoothctl devices Connected 2>/dev/null"),
                &setSnap.bt);
}

// host identity for the About card, probed once: the spec line is cpu +
// memory + the GL renderer, the release file wins over the baked-in
// version so a Hibiscus host reports its image stamp
void WlBackend::fillAbout(SetSnap* out) {
    if (!aboutDone) {
        std::string cpuinfo, meminfo, rel;
        readFile("/proc/cpuinfo", &cpuinfo);
        readFile("/proc/meminfo", &meminfo);
        readFile("/etc/hibiscus-release", &rel);
        const char* gpu = (const char*)glGetString(GL_RENDERER);
        sysModel = specLine(parseCpuModel(cpuinfo),
                            parseMemTotal(meminfo),
                            cleanGpuName(gpu ? gpu : ""));
        sysHb = parseHibiscusRelease(rel);
        if (sysHb.empty()) sysHb = HIBISCUS_VERSION;
        aboutDone = true;
    }
    out->txt[TX_MODEL] = sysModel;
    out->txt[TX_HBVER] = sysHb;
}

int WlBackend::settingsVersion() {
    pollSet();
    return setVer;
}

void WlBackend::settingsStatus(SetSnap* out) {
    pollSet();
    *out = setSnap;
    const long long now = dashNowMs();
    if (now < wifiScanUntil) out->flags |= SFL_WIFI_SCAN;
    if (now < btScanUntil) out->flags |= SFL_BT_SCAN;
    out->txt[TX_SSID] = sysSsid;
    out->txt[TX_THEME] = setTheme;
    out->txt[TX_ENV] = setEnv;
    out->txt[TX_LANG] = setLang;
    fillAbout(out);
}

void WlBackend::settingsSection(int sec) {
    if (sec == SEC_WIFI || sec == SEC_BT) settingsScan(sec);
    else ++setVer;
}

void WlBackend::settingsToggle(int id, bool on) {
    if (on) setSnap.toggles |= (1u << id);
    else setSnap.toggles &= ~(1u << id);
    if (id == TG_WIFI)
        runBg(std::string("nmcli radio wifi ") + (on ? "on" : "off"));
    else if (id == TG_BT)
        runBg(std::string("bluetoothctl power ") + (on ? "on" : "off"));
    else if (id == TG_MIC)
        runBg(std::string("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ ") +
              (on ? "0" : "1"));
    ++setVer;
}

void WlBackend::settingsSlider(int id, float v) {
    if (id < 0 || id >= SL_COUNT) return;
    setSnap.sld[id] = v;
    if (id == SL_VOL) quickSetSlider(QSLIDER_VOLUME, v);
    else if (id == SL_BRI) quickSetSlider(QSLIDER_BRIGHTNESS, v);
    ++setVer;
}

void WlBackend::settingsText(int id, const std::string& v) {
    if (id == TX_THEME) setTheme = v;
    else if (id == TX_ENV) setEnv = v;
    else if (id == TX_LANG) setLang = v;
    ++setVer;
}

void WlBackend::settingsScan(int sec) {
    const long long now = dashNowMs();
    if (sec == SEC_WIFI) {
        runBg("nmcli dev wifi rescan 2>/dev/null");
        wifiScanUntil = now + 4000;
    } else if (sec == SEC_BT) {
        runBg("bluetoothctl --timeout 8 scan on 2>/dev/null");
        btScanUntil = now + 8000;
    }
    ++setVer;
}

void WlBackend::xpToggle(const std::string& id, bool on) {
    for (auto& f : setSnap.xp)
        if (f.id == id) { f.on = on; break; }
    ++setVer;
}

void WlBackend::wifiJoin(const std::string& ssid, int sec,
                         const std::string& pw) {
    std::vector<std::string> argv = {"nmcli", "dev", "wifi", "connect",
                                     ssid};
    if (sec != WSEC_OPEN && !pw.empty()) {
        argv.push_back("password");
        argv.push_back(pw);
    }
    runBg(shellJoin(argv) + " 2>/dev/null");
    ++setVer;
}

void WlBackend::wifiForget(int netId) {
    if (netId < 0 || netId >= (int)setCons.size()) return;
    runBg(shellJoin({"nmcli", "con", "delete", setCons[netId]}) +
          " 2>/dev/null");
    ++setVer;
}

void WlBackend::btPair(const std::string& addr) {
    runBg(shellJoin({"bluetoothctl", "pair", addr}) + " 2>/dev/null");
    ++setVer;
}

void WlBackend::btUnpair(const std::string& addr) {
    runBg(shellJoin({"bluetoothctl", "remove", addr}) + " 2>/dev/null");
    ++setVer;
}

void WlBackend::launchPackageOn(const std::string& pkg, int displayId) {
    Slot* s = slotOf(displayId);
    if (!s) return;
    const DeskEntry* e = entry(pkg);
    std::vector<std::string> argv;
    if (e) {
        argv = e->argv;
        s->deskId = deskIdBase(e->id);
        s->wmClass = e->wmClass.empty() ? s->deskId : e->wmClass;
        if (e->terminal) {
            if (!termSearched) { term = findTerminal();
                                 termSearched = true; }
            if (!term.empty()) {
                argv = terminalWrap(term, argv);
                s->wmClass = term;
            }
        }
    } else {
        argv.push_back(pkg);
        s->deskId = pkg;
        s->wmClass = pkg;
    }
    if (argv.empty()) return;
    const pid_t pid = c->spawn(argv, {});
    if (pid > 0) {
        s->pid = pid;
        LOGI("spawn %s -> pid %d disp %d", pkg.c_str(), (int)pid,
             displayId);
    }
}

void WlBackend::adoptTaskOn(int taskId, int displayId) {
    Slot* s = slotOf(displayId);
    WlTask* t = taskOf(taskId);
    if (s && t) {
        s->task = t;
        t->displayId = displayId;
        c->configureTask(t, s->w, s->h);
    }
}

void WlBackend::focusTask(int taskId) {
    WlTask* t = taskOf(taskId);
    if (t) c->setActive(t, true);
}

void WlBackend::removeTask(int taskId) {
    WlTask* t = taskOf(taskId);
    if (t) c->requestClose(t);
}

void WlBackend::removeTasksOnDisplay(int displayId) {
    Slot* s = slotOf(displayId);
    if (s && s->task) c->requestClose(s->task);
}

void WlBackend::injectTouch(int displayId, float x, float y,
                            int action) {
    Slot* s = slotOf(displayId);
    if (!s || !s->task) return;
    WlTask* t = s->task;
    const float sx = t->bufW > 0 && s->w > 0 ? x * t->bufW / s->w : x;
    const float sy = t->bufH > 0 && s->h > 0 ? y * t->bufH / s->h : y;
    c->pointerEvent(t, sx, sy, action);
}

void WlBackend::injectTap(int displayId, float x, float y) {
    injectTouch(displayId, x, y, kTouchDown);
    injectTouch(displayId, x, y, kTouchUp);
}

bool WlBackend::takePendingAdopt(PendingPanel* out) {
    if (adopts.empty()) return false;
    *out = adopts.front();
    adopts.pop_front();
    return true;
}

int WlBackend::takePendingRelease() {
    if (releases.empty()) return -1;
    const int id = releases.front();
    releases.pop_front();
    return id;
}

void WlBackend::sendKey(uint32_t evdev, bool down) {
    c->keyEvent(evdev, down);
}
