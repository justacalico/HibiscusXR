#pragma once

#include <string>
#include <vector>

#include "../dock/item.h"
#include "../grid/item.h"
#include "../notif/item.h"
#include "../quick/item.h"
#include "../set/item.h"
#include "../sysmsg/item.h"

struct Kbd;
struct Panel;

constexpr int kTouchDown = 0;
constexpr int kTouchUp = 1;
constexpr int kTouchMove = 2;
constexpr int kKeyDown = 0;
constexpr int kKeyUp = 1;

struct PendingPanel {
    int taskId = -1;
    std::string pkg;
};

struct IconPixels {
    int w = 0, h = 0;
    std::vector<unsigned char> rgba;
};

struct KbdState {
    bool shown = false;
    int displayId = -1;
    bool only = false;
};

// Everything the dash needs from the host process. The Android
// implementation wraps ShellBridge; the desktop preview feeds the same
// calls from fixtures, so layout, input, animation and drawing stay shared.
class DashBackend {
public:
    virtual ~DashBackend() = default;

    virtual bool externalPanelTextures() { return true; }
    virtual int createPanel(Panel* p, unsigned tex, int w, int h,
                            int dpi) { return -1; }
    // relayout the app at a new display size; false = the display kept
    // its px size and the panel's dispW/dispH must not move
    virtual bool resizePanel(Panel* p, int w, int h, int dpi) {
        return false;
    }
    virtual void releasePanel(Panel* p) {}
    virtual void updatePanel(Panel* p) {}

    virtual bool createKbdSurface(Kbd* k, unsigned tex, int w, int h,
                                  int dpi) { return false; }
    virtual void updateKbd(Kbd* k) {}
    virtual bool takeKbdQuery() { return false; }
    virtual void sendKbdSurface() {}
    virtual void sendKbdHide() {}
    virtual KbdState kbdState() { return {}; }

    virtual bool takePins(std::vector<std::string>* out) { return false; }
    virtual void setPins(const std::vector<std::string>& pins) {}
    // the library's app groups, same take/set handshake the pins use
    virtual bool takeGroups(std::vector<LibGroup>* out) { return false; }
    virtual void setGroups(const std::vector<LibGroup>& groups) {}
    virtual int vrVersion() { return 0; }
    virtual void runningVr(std::vector<XrTask>* out) { out->clear(); }
    // {net, battPct, charging}; net is a DockNet link kind, battPct -1 =
    // the device has no battery
    virtual void sysStatus(int out[3]) { out[0] = 0; out[1] = -1; out[2] = 0; }
    virtual bool iconPixels(const std::string& pkg, IconPixels* out) {
        return false;
    }
    virtual std::string appLabel(const std::string& pkg) { return pkg; }
    virtual bool isVrApp(const std::string& pkg) { return false; }

    virtual int appsVersion() { return 0; }
    virtual void launcherApps(std::vector<GridItem>* out) { out->clear(); }
    virtual int notifVersion() { return 0; }
    virtual void notifs(std::vector<NotifItem>* out) { out->clear(); }
    virtual void dismissNotif(const std::string& key) {}
    virtual int sysMsgVersion() { return 0; }
    virtual void sysMsgs(std::vector<SysMsgItem>* out) { out->clear(); }
    virtual void sysMsgClick(long long id, int btn) {}
    virtual void sysMsgDismiss(long long id) {}
    virtual int uiStringsVersion() { return 0; }
    virtual void uiStrings(std::vector<std::string>* out) { out->clear(); }

    // the quick panel's platform side: a fresh snapshot while the card is
    // up, toggle writes keyed by QsId, slider writes keyed by QsSlider
    virtual void quickStatus(QsStatus* out) {}
    virtual void quickToggle(int id, bool on) {}
    virtual void quickSetSlider(int slider, float v) {}

    // the settings card's platform side: versioned snapshot pulls while
    // the card is up, writes keyed by the SetTgl/SetSld/SetTxt/SetAct
    // ids; the section calls carry open/change/close so the host can arm
    // scans and the controller link only while the card needs them
    virtual int settingsVersion() { return 0; }
    virtual void settingsStatus(SetSnap* out) {}
    virtual void settingsSection(int sec) {}
    virtual void settingsClose() {}
    virtual void settingsToggle(int id, bool on) {}
    virtual void settingsSlider(int id, float v) {}
    virtual void settingsText(int id, const std::string& v) {}
    virtual void settingsAction(int id) {}
    virtual void settingsScan(int sec) {}
    virtual void settingsToggleScan() {}
    virtual void settingsReboot() {}
    virtual void wifiJoin(const std::string& ssid, int sec,
                          const std::string& pw) {}
    virtual void wifiForget(int netId) {}
    virtual void btPair(const std::string& addr) {}
    virtual void btUnpair(const std::string& addr) {}
    virtual void imePick(const std::string& id) {}
    virtual void envDelete(const std::string& id) {}

    virtual void launchPackageOn(const std::string& pkg, int displayId) {}
    virtual void launchVrApp(const std::string& pkg) {}
    virtual void adoptTaskOn(int taskId, int displayId) {}
    virtual void focusTask(int taskId) {}
    virtual void removeTask(int taskId) {}
    virtual void removeTasksOnDisplay(int displayId) {}
    virtual void injectTouch(int displayId, float x, float y, int action) {}
    virtual void injectTap(int displayId, float x, float y) {}
    virtual void dismissMenu() {}

    virtual bool isCovered() { return false; }
    virtual bool toastOnly() { return false; }
    virtual bool debugHud() { return false; }
    virtual bool debugOnly() { return false; }
    virtual bool sysMsgOnly() { return false; }
    virtual bool takePendingAdopt(PendingPanel* out) { return false; }
    virtual int takePendingRelease() { return -1; }

    virtual void pokeCtrlThread() {}
    virtual void debugSummon() {}
    virtual void debugSummonKey(int action) {}
};
