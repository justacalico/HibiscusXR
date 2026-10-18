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
    void releasePanel(Panel* p) override;
    void updatePanel(Panel* p) override;

    bool takePins(std::vector<std::string>* out) override;
    void setPins(const std::vector<std::string>& pins) override;
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

    void launchPackageOn(const std::string& pkg, int displayId) override;
    void adoptTaskOn(int taskId, int displayId) override;
    void focusTask(int taskId) override;
    void removeTask(int taskId) override;
    void removeTasksOnDisplay(int displayId) override;
    void injectTouch(int displayId, float x, float y, int action) override;
    void injectTap(int displayId, float x, float y) override;
    bool centerLaunch() override { return centerOn; }
    bool takePendingAdopt(PendingPanel* out) override;
    int takePendingRelease() override;

    void sendKey(uint32_t evdev, bool down);

private:
    struct Slot {
        int displayId = -1;
        unsigned tex = 0;
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
    int sysWifi = 0, sysBatt = -1, sysChg = 0;
    std::string sysSsid;
    bool centerOn = true;
    bool pinsSent = false;
    unsigned qsMask = 0;

    Slot* slotOf(int displayId);
    WlTask* taskOf(int taskId);
    void rescanApps();
    void pollSys();
    const DeskEntry* entry(const std::string& pkg);
    void onTaskMap(WlTask* t);
    void onTaskDead(WlTask* t);
};
