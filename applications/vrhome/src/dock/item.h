#pragma once

#include <string>
#include <vector>

// where on a dock item a gaze hit lands
enum DockZone {
    DZONE_NONE = -1,
    DZONE_ICON = 0,    // the icon body: activate the app
    DZONE_CLOSE,       // the close badge on a live immersive item
    DZONE_HANDLE,      // the drag line under the strip: moves the whole ring
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

// one parked window on the minimized shelf above the dock bar: panelIdx
// links back into the panel list, x is the icon's pill-local centre
struct ShelfItem {
    int panelIdx = -1;
    std::string pkg;
    std::string label;      // resolved app label, shown while hovered
    float x = 0;            // pill-local centre x, set by shelfLayout
};

struct ShelfPick {
    int idx = -1;           // shelf slot under the ray, -1 on the pill body
    bool hit = false;       // the ray hit the pill at all (blocks panels)
    float t = 1e9f;         // ray distance, for pick arbitration
};

// status cluster pinned to the strip's left end: clock, battery and wifi
// grouped in one pill, the notification bell alone in a second, then a
// separator before the app icons. clockW goes in measured; the rest come
// back positioned by dockLayout
struct DockStatus {
    float clockW = 0;   // measured width of the time text
    float clockX = 0;   // text's left edge in bar coords
    float wifiX = 0;    // icon centres
    float battX = 0;
    float bellX = 0;
    float pillAL = 0;   // first pill's left/right edges
    float pillAR = 0;
    float pillBL = 0;   // bell pill's left/right edges
    float pillBR = 0;
    float sepX = 0;     // separator line before the app icons
};
