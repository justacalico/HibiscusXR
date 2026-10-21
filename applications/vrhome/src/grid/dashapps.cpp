#include "dashapps.h"

static const DashLibApp kDashApps[DP_COUNT] = {
    {DP_SETTINGS, "dash.settings", GI_SETTINGS, "Settings", true},
    {DP_QUICK,    "dash.quick",    GI_TUNE,     "Quick settings", true},
};

const DashLibApp* dashLibApps() {
    return kDashApps;
}

int dashPanelForPkg(const std::string& pkg) {
    for (int i = 0; i < DP_COUNT; ++i)
        if (kDashApps[i].canShowInLibrary && pkg == kDashApps[i].pkg)
            return kDashApps[i].panel;
    return -1;
}

void dashLibMerge(std::vector<GridItem>* apps) {
    std::vector<GridItem> head;
    for (int i = 0; i < DP_COUNT; ++i) {
        if (!kDashApps[i].canShowInLibrary) continue;
        GridItem it;
        it.pkg = kDashApps[i].pkg;
        it.dashPanel = kDashApps[i].panel;
        head.push_back(it);
    }
    apps->insert(apps->begin(), head.begin(), head.end());
}
