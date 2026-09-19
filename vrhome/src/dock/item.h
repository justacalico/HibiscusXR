#pragma once

#include <string>
#include <vector>

// where on a dock item a gaze hit lands
enum DockZone {
    DZONE_NONE = -1,
    DZONE_ICON = 0,    // the icon body: activate the app
    DZONE_CLOSE,       // the close badge on a live immersive item
};

// item kinds in left-to-right group order: pinned favourites, live tasks,
// the quick-panel button on the end
enum DockKind {
    DK_PIN = 0,
    DK_RUN,            // a live task: a 2D panel app or an immersive XR app
    DK_QUICK,
};

// one immersive task on the physical display, as reported by the java
// poller - taskId feeds focus/remove, pkg feeds the icon and label
struct XrTask {
    int taskId = -1;
    std::string pkg;
};

struct DockItem {
    int kind = DK_PIN;
    std::string pkg;
    std::string label;      // resolved app label, shown while hovered
    int taskId = -1;        // live task: panel's or immersive app's
    int panelIdx = -1;      // index into the panel list when one hosts it
    bool running = false;   // a live task owns this pkg (draws the dot)
    bool minimized = false; // panel alive but hidden; icon dims
    bool vr = false;        // immersive app: amber ring + close badge
    bool sep = false;       // group separator gap before this item
    float x = 0;            // bar-local centre x in metres, set by dockLayout
};

// icon texture + resolved metadata for one package, cached per pkg. tex is
// 0 until the GL upload lands; tried stops a missing icon being refetched
struct DockIcon {
    unsigned tex = 0;
    bool tried = false;
    bool vr = false;        // cached isVrApp answer for pinned items
    bool vrTried = false;
    std::string label;
};

struct DockPick {
    int idx = -1;           // item under the ray, -1 for the bar body/gaps
    bool bar = false;       // the ray hit the strip at all (blocks panels)
    float u = 0, v = 0;     // hit point in bar coords, -1..1
    float t = 1e9f;         // ray distance, for pick arbitration
    int zone = DZONE_NONE;
};
