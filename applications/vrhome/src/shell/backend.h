#pragma once

#include <string>
#include <vector>

#include "../dock/item.h"
#include "../grid/item.h"
#include "../notif/item.h"
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
    virtual int vrVersion() { return 0; }
    virtual void runningVr(std::vector<XrTask>* out) { out->clear(); }
    virtual void sysStatus(int out[3]) { out[0] = out[1] = out[2] = 0; }
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
    virtual bool centerLaunch() { return false; }
    virtual bool takePendingAdopt(PendingPanel* out) { return false; }
    virtual int takePendingRelease() { return -1; }

    virtual void pokeCtrlThread() {}
    virtual void debugSummon() {}
    virtual void debugSummonKey(int action) {}
};
