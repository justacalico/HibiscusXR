#pragma once

#include "item.h"
#include "../panels/panel.h"
#include "../math/mat4.h"

#include <string>
#include <vector>

// Dock strip policy + geometry. Pure functions over plain data - no GL, no
// JNI - so ordering, layout and hit testing all run in the host tests.

// dock quad on the same anchor cylinder the panel ring rides, at its own
// distance and elevation
void dockCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]);

// the summon slide: drop shifts the strip down its own up axis while the
// fade-in runs, and the pick path takes the same value so a hit always
// lands where the strip is actually drawn
void dockCenterDrop(float yaw, float pitch, float drop,
                    const float origin[3], float c[3], float r[3],
                    float up[3]);

// the dock's elevation when the dash anchors on a head pitch: scaled toward
// level and clamped so the strip stays below the windows, never overhead
float dockPitchFor(float headPitch);

// the window ring's elevation given the strip's: the dash is one assembly,
// so the windows ride a fixed lift over the bar instead of carrying an
// independent pitch
float ringPitchFor(float dockPitch);

// the elevation a window joins at: a running row keeps its pitch, an empty
// (or all-floating) ring derives it off the strip the dash is tied to
float dashRingPitch(const std::vector<Panel>& panels, float dockPitch);

// dock elevation while a handle drag holds: the strip is tied to the window
// ring, so it picks up exactly the pitch the ring gained since the grab -
// pole clamp included - and the dash moves as one piece
float dockDragPitch(float grabPitch, float ringGrabPitch, float ringPitch);

// ordered items for the strip: pins first (marked running when a live task
// owns the pkg), then unpinned 2D panels, then unpinned XR tasks, then the
// quick button. `sep` marks the group boundaries that draw separator gaps
std::vector<DockItem> buildDock(const std::vector<std::string>& pins,
                                const std::vector<Panel>& panels,
                                const std::vector<XrTask>& xr);

// place the status cluster on the left and each item after it, then return
// the bar's half-width in metres. st.clockW comes in measured, the other
// positions come out
float dockLayout(std::vector<DockItem>& items, DockStatus& st);

// which item a bar-local point hits (u -1..1 across the bar, v -1..1 across
// its height); *zone gets DZONE_CLOSE on a live immersive item's badge
int dockItemAt(const std::vector<DockItem>& items, float halfW,
               float u, float v, int* zone);

// gaze ray vs the dock plane; u,v in bar coords, may fall outside -1..1.
// drop is the strip's summon-slide offset (see dockCenterDrop)
bool rayDock(float yaw, float pitch, float drop, const float origin[3],
             const float o[3], const float d[3], float halfW,
             float* u, float* v, float* t);

// the dock item under an arbitrary ray; pk.bar is set even when the ray
// lands on the strip between icons, so the bar body still blocks clicks
DockPick pickDockRay(const std::vector<DockItem>& items, float halfW,
                     float yaw, float pitch, float drop,
                     const float origin[3], const float o[3],
                     const float d[3]);

// the dock item under the gaze ray
DockPick pickDock(const std::vector<DockItem>& items, float halfW,
                  float yaw, float pitch, float drop, const Mat4& head,
                  const float origin[3], const float o[3]);

// can a long-press pin this item: the quick button isn't pinnable
bool dockPinnable(const DockItem& it);

// add pkg to the end of the pin list or remove it; returns the new list
std::vector<std::string> pinToggle(const std::vector<std::string>& pins,
                                   const std::string& pkg);

// hidden panels parked on the shelf above the dock bar, in panel order. A
// panel mid-restore (minT still draining) keeps its slot so the flight has
// a stable point to grow out of and the icon can fade with it
std::vector<ShelfItem> buildShelf(const std::vector<Panel>& panels);

// the parked window's pill-local icon x, or false while the panel has no
// slot to aim at - the minimize flight targets this point
bool shelfXFor(const std::vector<ShelfItem>& items, int panelIdx,
               float* x);

// hover-scale handoff across syncDock's per-frame rebuilds: without it the
// smoothed scale would snap back to 1.0f every frame. Items match by
// identity - kind+pkg on the strip, pkg on the shelf - never by index
void carryHover(std::vector<DockItem>& items,
                const std::vector<DockItem>& prev);
void carryShelfHover(std::vector<ShelfItem>& items,
                     const std::vector<ShelfItem>& prev);

// per-frame hover smoothing: each icon's hs chases kHoverScale while it's
// the hovered slot and relaxes to 1.0f otherwise
void tickDockHover(std::vector<DockItem>& items, int hover, float dtMs);
void tickShelfHover(std::vector<ShelfItem>& items, int hover, float dtMs);

// centre the icon row on a pill and return the pill's half-width
float shelfLayout(std::vector<ShelfItem>& items);

// the pill's centre and its top edge as lifts above the dock bar's centre
float shelfLift();
float shelfTop();

// shelf quad on the dock's anchor plane, raised shelfLift along its up
void shelfCenter(float yaw, float pitch, const float origin[3],
                 float c[3], float r[3], float up[3]);

// the pill rides the strip's summon slide with it
void shelfCenterDrop(float yaw, float pitch, float drop,
                     const float origin[3], float c[3], float r[3],
                     float up[3]);

// which icon a pill-local point hits; -1 on the body or in the gaps
int shelfItemAt(const std::vector<ShelfItem>& items, float halfW,
                float u, float v);

// gaze ray vs the shelf plane; u,v in pill coords, may fall outside -1..1.
// drop matches rayDock's: the pill slides with the strip on summon
bool rayShelf(float yaw, float pitch, float drop, const float origin[3],
              const float o[3], const float d[3], float halfW,
              float* u, float* v, float* t);

// the shelf icon under an arbitrary ray; hit is set even on the pill body
// between icons, so the shelf blocks clicks like the dock bar does
ShelfPick pickShelfRay(const std::vector<ShelfItem>& items, float halfW,
                       float yaw, float pitch, float drop,
                       const float origin[3], const float o[3],
                       const float d[3]);

// the shelf icon under the gaze ray
ShelfPick pickShelf(const std::vector<ShelfItem>& items, float halfW,
                    float yaw, float pitch, float drop, const Mat4& head,
                    const float origin[3], const float o[3]);

// what an activate/close resolves to, computed off plain data so the
// focus-vs-launch policy is host-testable; dock.cpp turns the answer
// into bridge calls
enum DockOp {
    DOP_NONE = 0,     // dead item or inconsistent state: do nothing
    DOP_TOGGLE_GRID,  // the grid button flips the app-grid overlay
    DOP_LAUNCH,       // cold start pkg through the bridge
    DOP_FOCUS_PANEL,  // unminimize + focus the panel's task when it has one
    DOP_FOCUS_XR,     // immersive running item: focus the task, drop the menu
    DOP_CLOSE_PANEL,  // kill through the panel's display
    DOP_CLOSE_TASK,   // kill the immersive task directly
};

struct DockAction {
    DockOp op = DOP_NONE;
    int panelIdx = -1;  // FOCUS_PANEL / CLOSE_PANEL: index into panels
    int taskId = -1;    // task to focus/kill when one is known
    int displayId = -1; // CLOSE_PANEL: the panel's display
    std::string pkg;    // LAUNCH: package to start
};

// release on a strip item: the grid button toggles, a live item refocuses,
// a cold pin launches. A stale panel link (the panel's pkg moved on)
// launches fresh instead of focusing the wrong window
DockAction dockActivateAction(const DockItem& it,
                              const std::vector<Panel>& panels);

// the close badge: a panel's task dies through its display - the cached
// taskId can be stale - while an immersive task only has its id to go on
DockAction dockCloseAction(const DockItem& it,
                           const std::vector<Panel>& panels);

// release on a shelf icon: only a still-minimized panel that kept its pkg
// restores
DockAction shelfActivateAction(const ShelfItem& it,
                               const std::vector<Panel>& panels);
