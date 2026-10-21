#pragma once

#include "item.h"

#include "icon_ids.h"

#include <vector>

// The dash's own cards can sit in the library next to real apps. Each
// panel declares canShowInLibrary plus the glyph and name its cell
// draws, so the entries are baked at build time - opening the grid
// never pulls a package or rebinds an icon for them
enum DashPanel {
    DP_SETTINGS = 0,   // the settings card (src/set)
    DP_QUICK,          // the quick panel (src/quick)
    DP_COUNT
};

struct DashLibApp {
    int panel;               // DashPanel the cell's tap opens
    const char* pkg;         // stable pseudo key; never a real package
    int glyph;               // baked atlas icon, a GlyphId
    const char* name;        // label until the uiStrings pull lands
    bool canShowInLibrary;
};

// the registry, indexed by DashPanel
const DashLibApp* dashLibApps();

// pkg -> DashPanel, or -1 for a real app
int dashPanelForPkg(const std::string& pkg);

// prepend every opted-in panel to an app list, in DashPanel order
void dashLibMerge(std::vector<GridItem>* apps);
