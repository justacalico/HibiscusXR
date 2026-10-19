#include "bridge.h"

#include "../common/config.h"
#include "../common/props.h"
#include "../hud/engine.h"
#include "../dock/layout.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../set/item.h"

#include <deque>
#include <mutex>

static std::deque<std::string> gLaunchQ;
static std::mutex gLaunchMu;
static volatile bool gWantRecenter = false;

void queueLaunch(const char* pkg) {
    std::lock_guard<std::mutex> l(gLaunchMu);
    gLaunchQ.push_back(pkg);
}

void wantRecenter() { gWantRecenter = true; }

bool takeWantRecenter() {
    const bool r = gWantRecenter;
    gWantRecenter = false;
    return r;
}

void syncUiStrings(HudEngine* e) {
    if (!e->backend) return;
    const int v = e->backend->uiStringsVersion();
    if (v == e->uiStrVer) return;
    std::vector<std::string> s;
    e->backend->uiStrings(&s);
    e->uiStrVer = v;
    if (!s.empty()) e->uiLib = s[0];
    // slots 1..UIQS_COUNT are the quick panel's labels: title, the
    // state words, then one per tile in QsId order
    for (int i = 0; i < UIQS_COUNT; ++i)
        e->uiQs[i] = i + 1 < (int)s.size() ? s[i + 1] : "";
    // the settings block follows the quick panel's
    for (int i = 0; i < UISET_COUNT; ++i)
        e->uiSet[i] = i + 1 + UIQS_COUNT < (int)s.size()
                          ? s[i + 1 + UIQS_COUNT] : "";
    for (auto& p : e->panels) p.label.clear();
    for (auto& kv : e->dockIcons) kv.second.label.clear();
}

static float ringPitchNow(HudEngine* e) {
    return dashRingPitch(e->panels, e->dockPitch);
}

static float launchYaw(HudEngine* e) {
    return e->centerLaunch ? centreSlotYaw(e->panels, e->dockYaw)
                           : freeSlotYaw(e->panels, e->dockYaw);
}

static int panelCap(const HudEngine* e) {
    return e->centerLaunch ? kMaxPanelRecs : kMaxPanels;
}

void pumpBridge(HudEngine* e) {
    if (!e->backend) return;
    for (;;) {
        std::string pkg;
        {
            std::lock_guard<std::mutex> l(gLaunchMu);
            if (gLaunchQ.empty()) break;
            pkg = gLaunchQ.front(); gLaunchQ.pop_front();
        }
        if (e->backend->isVrApp(pkg)) { e->backend->launchVrApp(pkg); continue; }
        const int mi = minimizedIndex(e->panels, pkg);
        if (mi >= 0) {
            restorePanel(e->panels, mi, e->dockYaw, ringPitchNow(e));
            if (e->panels[mi].taskId >= 0)
                e->backend->focusTask(e->panels[mi].taskId);
            continue;
        }
        if (panelIndex(e->panels, pkg) >= 0) {
            for (auto& p : e->panels)
                if (p.pkg == pkg && p.taskId >= 0) {
                    e->backend->focusTask(p.taskId); break;
                }
            continue;
        }
        if ((int)e->panels.size() >= panelCap(e) && !evictOldestApp(e))
            continue;
        const int idx = openPanel(e, launchYaw(e), ringPitchNow(e));
        if (idx < 0) continue;
        e->panels[idx].pkg = pkg;
        e->backend->launchPackageOn(pkg, e->panels[idx].displayId);
    }
    const bool was = e->covered;
    e->covered = e->backend->isCovered();
    if (was && !e->covered) wantRecenter();
    e->toastOnly = e->backend->toastOnly();
    e->debugHud = e->backend->debugHud();
    e->debugOnly = e->backend->debugOnly();
    e->sysMsgOnly = e->backend->sysMsgOnly();
    e->centerLaunch = e->backend->centerLaunch();
    if (propI("debug.vrhome.noadopt", 0)) return;
    for (;;) {
        PendingPanel p;
        if (!e->backend->takePendingAdopt(&p)) break;
        if ((int)e->panels.size() >= panelCap(e)) evictOldestApp(e);
        const int idx = openPanel(e, launchYaw(e), ringPitchNow(e));
        if (idx >= 0) {
            e->panels[idx].taskId = p.taskId;
            e->panels[idx].pkg = p.pkg;
            e->backend->adoptTaskOn(p.taskId, e->panels[idx].displayId);
        } else {
            e->backend->removeTask(p.taskId);
        }
    }
    for (;;) {
        const int id = e->backend->takePendingRelease();
        if (id < 0) break;
        for (int i = 0; i < (int)e->panels.size(); ++i)
            if (e->panels[i].displayId == id) { closePanel(e, i); break; }
    }
    for (auto& p : e->panels)
        if (!p.pkg.empty() && p.label.empty())
            p.label = e->backend->appLabel(p.pkg);
}
