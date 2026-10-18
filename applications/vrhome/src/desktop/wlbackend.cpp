#include "wlbackend.h"

#include "harness.h"
#include "icontheme.h"
#include "sysinfo.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../panels/panel.h"
#include "../quick/item.h"

#include <GLES2/gl2.h>
#include <png.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <unistd.h>

namespace fs = std::filesystem;

// a pending launch slot gets this long to bind a task before its panel
// is treated as dead
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
    sysBatt = -1; sysChg = 0;
    std::error_code ec;
    for (const auto& de :
         fs::directory_iterator("/sys/class/power_supply", ec)) {
        if (de.path().filename().string().rfind("BAT", 0) != 0)
            continue;
        std::string cap, st;
        readFile(de.path().string() + "/capacity", &cap);
        readFile(de.path().string() + "/status", &st);
        sysBatt = parseCapacity(cap);
        sysChg = parseCharging(st) ? 1 : 0;
        break;
    }
    if (sysBatt < 0) sysBatt = 0;
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
        // a pending slot whose app never maps expires so the panel dies
        if (dashNowMs() - s->bornMs > kSlotStaleMs)
            releases.push_back(p->displayId);
        return;
    }
    if (!t->mapped) return;
    if (!t->dirty || t->img.rgba.empty()) return;
    glBindTexture(GL_TEXTURE_2D, p->tex);
    if (t->img.w != t->bufW || t->img.h != t->bufH ||
            t->dx0 == 0 && t->dy0 == 0 && t->dx1 == t->img.w &&
            t->dy1 == t->img.h) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->img.w, t->img.h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, t->img.rgba.data());
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, t->dx0, t->dy0,
                        t->dx1 - t->dx0, t->dy1 - t->dy0,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        t->img.rgba.data() +
                            ((size_t)t->dy0 * t->img.w + t->dx0) * 4);
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

void WlBackend::sysStatus(int out[3]) {
    pollSys();
    out[0] = sysWifi; out[1] = sysBatt; out[2] = sysChg;
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
        "Battery saver", "Center new apps", "Report problem",
        "About device",
    };
}

void WlBackend::quickStatus(QsStatus* out) {
    pollSys();
    out->toggles = qsMask;
    if (sysWifi) out->toggles |= (1u << QS_WIFI);
    if (centerOn) out->toggles |= (1u << QS_CENTER);
    const std::string v =
        runOut("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null");
    out->volume = parseWpctlMuted(v) ? 0.0f : parseWpctlVol(v);
    if (out->volume < 0.0f) out->volume = 0.0f;
    const std::string b =
        runOut("brightnessctl -m 2>/dev/null");
    out->brightness = parseBrightness(b);
    if (out->brightness < 0.0f) out->brightness = 0.5f;
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
    else if (id == QS_CENTER)
        centerOn = on;
}

void WlBackend::quickSetSlider(int slider, float v) {
    if (slider == QSLIDER_VOLUME) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd),
                 "wpctl set-volume @DEFAULT_AUDIO_SINK@ %.2f",
                 v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v);
        runBg(cmd);
    } else if (slider == QSLIDER_BRIGHTNESS) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "brightnessctl set %.0f%%",
                 v * 100.0f);
        runBg(cmd);
    }
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
    const float sx = t->bufW > 0 ? x * t->bufW / kVdW : x;
    const float sy = t->bufH > 0 ? y * t->bufH / kVdH : y;
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
