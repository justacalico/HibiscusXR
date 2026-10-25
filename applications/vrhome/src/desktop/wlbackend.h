#pragma once

#include "desktopfile.h"
#include "wlcomp.h"
#include "../shell/backend.h"

#include <deque>
#include <map>

struct WlComp;
struct WlTask;

// DashBackend fed by the real Wayland clients the compositor hosts:
// .desktop launcher data, icon themes, seat input, process spawn, and
// the host's radio/power/volume tools behind the quick panel.
class WlBackend : public DashBackend {
public:
    explicit WlBackend(WlComp* c);
    ~WlBackend() override;

    bool externalPanelTextures() override { return false; }
    int createPanel(Panel* p, unsigned tex, int w, int h, int dpi) override;
    bool resizePanel(Panel* p, int w, int h, int dpi) override;
    void releasePanel(Panel* p) override;
    void updatePanel(Panel* p) override;

    bool takePins(std::vector<std::string>* out) override;
    void setPins(const std::vector<std::string>& pins) override;
    bool takeGroups(std::vector<LibGroup>* out) override;
    void setGroups(const std::vector<LibGroup>& groups) override;
    void sysStatus(int out[3]) override;
    bool iconPixels(const std::string& pkg, IconPixels* out) override;
    std::string appLabel(const std::string& pkg) override;

    int appsVersion() override;
    void launcherApps(std::vector<GridItem>* out) override;
    int uiStringsVersion() override { return 1; }
    void uiStrings(std::vector<std::string>* out) override;

    void quickStatus(QsStatus* out) override;
    void quickToggle(int id, bool on) override;
    void quickSetSlider(int slider, float v) override;

    int settingsVersion() override;
    void settingsStatus(SetSnap* out) override;
    void settingsSection(int sec) override;
    void settingsToggle(int id, bool on) override;
    void settingsSlider(int id, float v) override;
    void settingsText(int id, const std::string& v) override;
    void settingsScan(int sec) override;
    void xpToggle(const std::string& id, bool on) override;
    void wifiJoin(const std::string& ssid, int sec,
                  const std::string& pw) override;
    void wifiForget(int netId) override;
    void btPair(const std::string& addr) override;
    void btUnpair(const std::string& addr) override;

    void launchPackageOn(const std::string& pkg, int displayId) override;
    void adoptTaskOn(int taskId, int displayId) override;
    void focusTask(int taskId) override;
    void removeTask(int taskId) override;
    void removeTasksOnDisplay(int displayId) override;
    void injectTouch(int displayId, float x, float y, int action) override;
    void injectTap(int displayId, float x, float y) override;
    void injectPointerLeave(int displayId) override;
    bool hasAppCursor() override { return true; }
    bool takePendingAdopt(PendingPanel* out) override;
    int takePendingRelease() override;

    void sendKey(uint32_t evdev, bool down);

private:
    struct Slot {
        int displayId = -1;
        unsigned tex = 0;
        int w = 0, h = 0;      // px size the panel's display runs at
        int texW = 0, texH = 0; // size the texture is allocated at
        WlTask* task = nullptr;
        pid_t pid = -1;
        std::string wmClass;
        std::string deskId;   // bare desktop id, minus ".desktop"
        long long bornMs = 0;
    };

    WlComp* c;
    std::map<int, Slot> slots;
    int nextDisp = 100;
    std::deque<PendingPanel> adopts;
    std::deque<int> releases;
    std::vector<DeskEntry> apps;
    std::map<std::string, const DeskEntry*> byId;
    std::map<std::string, std::string> byClass;
    int appsVer = 0;
    long long appsScanAt = 0;
    long long sysPollAt = 0;
    int sysWifi = 0, sysEth = 0, sysBatt = -1, sysChg = 0;
    std::string sysSsid;
    float sysVol = 0.0f, sysBri = 0.5f;
    bool pinsSent = false;
    bool groupsSent = false;
    std::vector<LibGroup> groups;
    unsigned qsMask = 0;
    std::string term;
    bool termSearched = false;

    int setVer = 0;
    long long setPollAt = 0;
    long long wifiScanUntil = 0, btScanUntil = 0;
    SetSnap setSnap;
    std::vector<std::string> setCons;   // nmcli profile names, saved rows index it
    std::string setTheme = "dark", setEnv = "builtin", setLang = "en";
    bool aboutDone = false;
    std::string sysModel, sysHb;

    Slot* slotOf(int displayId);
    WlTask* taskOf(int taskId);
    void rescanApps();
    void pollSys();
    void pollSet();
    void fillAbout(SetSnap* out);
    const DeskEntry* entry(const std::string& pkg);
    void onTaskMap(WlTask* t);
    void onTaskDead(WlTask* t);
};
