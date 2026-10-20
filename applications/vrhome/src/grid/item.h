#pragma once

#include <string>
#include <vector>

// where on the app grid a gaze hit lands
enum GridZone {
    GZONE_NONE = -1,
    GZONE_ITEM = 0,    // an app cell: press launches it
    GZONE_BODY,        // the card itself: holds a scroll drag, eats the tap
    GZONE_CLOSE,       // the disc on the title row
    GZONE_GROUPSEL,    // the group pill on the title row: opens the dropdown
    GZONE_GROW,        // a row of the open group dropdown; idx = row
    GZONE_DOTS,        // a cell's 3-dots affordance; idx = cell
    GZONE_MENU,        // a row of the open cell context menu; idx = row
};

// a named app set in the library: groups persist through the backend like
// the dock's pins do and the title-row pill filters the grid to one
struct LibGroup {
    std::string name;
    std::vector<std::string> pkgs;
};

// one app cell in the overlay: pkg/label come from the java side, x/y is
// the cell centre in card-local metres set by gridItems
struct GridItem {
    std::string pkg;
    std::string label;
    bool vr = false;        // immersive app: launches straight to display 0
    float x = 0, y = 0;     // cell centre before scroll, card-local metres
    float hs = 1.0f;        // smoothed hover scale, see DockItem.hs
};

struct GridPick {
    int idx = -1;           // cell under the ray, -1 on the card body
    int zone = GZONE_NONE;
    bool hit = false;       // the ray met the card at all (blocks panels)
    float u = 0, v = 0;     // hit point in card coords, -1..1
    float t = 1e9f;         // ray distance, for pick arbitration
};

// the overlay's live state on the engine: the item list rebuilds when the
// java package set or the group filter changes, the rest is per-frame
// input bookkeeping
struct GridOverlay {
    bool shown = false;
    float openT = 0.0f;     // card open progress 0..1: chases `shown`, so
                            // the card keeps drawing while it fades out
    std::vector<GridItem> all;    // every launchable app from the backend
    std::vector<GridItem> items;  // `all` filtered through groupSel
    std::vector<LibGroup> groups; // pulled + persisted through the backend
    int groupSel = -1;            // group index the grid shows, -1: all apps
    bool grpOpen = false;         // the group dropdown list is open
    std::string menuPkg;          // pkg the cell context menu is open for
    float menuX = 0.0f, menuY = 0.0f;  // its anchor, card-local metres
    float pillHW = 0.0f;          // measured group pill half width
    int appsVer = -1;       // last pulled launcherApps version
    float scroll = 0.0f;    // metres the grid is scrolled by
    int hover = -1;
    int zone = GZONE_NONE;
    float u = 0.0f, v = 0.0f;
    int press = -1;                 // cell/row a confirm press started on
    int pressZone = GZONE_NONE;
    std::string pressPkg;           // guards against a rebuild mid-press
    bool scrollHeld = false;        // confirm held and dragging the card
    float grabV = 0.0f;             // card-local v at grab
    float grabScroll = 0.0f;        // scroll offset at grab
};

// ui strings the card draws on top of the title it already pulls: slots in
// the backend's uiStrings pull, appended after the settings card's block.
// Keep UiStrings.java's snapshot order in step
enum UiLibStr {
    UILIB_ALL = 0,          // "All apps" - the pill's no-filter label
    UILIB_NEW,              // "New group" - the dropdown's create row
    UILIB_PIN,              // "Pin to dock"
    UILIB_UNPIN,            // "Remove from dock"
    UILIB_GROUPFMT,         // "Group %d" - auto name for a fresh group
    UILIB_COUNT
};
