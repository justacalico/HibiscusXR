#pragma once

#include <string>
#include <vector>

// where on the app grid a gaze hit lands
enum GridZone {
    GZONE_NONE = -1,
    GZONE_ITEM = 0,    // an app cell: press launches it
    GZONE_BODY,        // the card itself: holds a scroll drag, eats the tap
    GZONE_CLOSE,       // the disc on the title row
};

// one app cell in the overlay: pkg/label come from the java side, x/y is
// the cell centre in card-local metres set by gridItems
struct GridItem {
    std::string pkg;
    std::string label;
    bool vr = false;        // immersive app: launches straight to display 0
    float x = 0, y = 0;     // cell centre before scroll, card-local metres
};

struct GridPick {
    int idx = -1;           // cell under the ray, -1 on the card body
    int zone = GZONE_NONE;
    bool hit = false;       // the ray met the card at all (blocks panels)
    float u = 0, v = 0;     // hit point in card coords, -1..1
    float t = 1e9f;         // ray distance, for pick arbitration
};

// the overlay's live state on the engine: item list rebuilt when the java
// package set changes, the rest is per-frame input bookkeeping
struct GridOverlay {
    bool shown = false;
    std::vector<GridItem> items;
    int appsVer = -1;       // last pulled launcherApps version
    float scroll = 0.0f;    // metres the grid is scrolled by
    int hover = -1;
    int zone = GZONE_NONE;
    float u = 0.0f, v = 0.0f;
    int press = -1;                 // cell a confirm press started on
    int pressZone = GZONE_NONE;
    std::string pressPkg;           // guards against a rebuild mid-press
    bool scrollHeld = false;        // confirm held on the card body
    float grabV = 0.0f;             // card-local v at grab
    float grabScroll = 0.0f;        // scroll offset at grab
};
