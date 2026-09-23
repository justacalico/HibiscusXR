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

// the dock's elevation when the dash anchors on a head pitch: scaled toward
// level and clamped so the strip stays below the windows, never overhead
float dockPitchFor(float headPitch);

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

// how far under the strip's centre the move handle's centre hangs, world
// units
float dockHandleDrop();

// is a bar-local point on the move handle under the strip; the hit box is
// padded past the drawn line since gaze aim is coarse
bool onDockHandle(float u, float v, float halfW);

// gaze ray vs the dock plane; u,v in bar coords, may fall outside -1..1
bool rayDock(float yaw, float pitch, const float origin[3],
             const float o[3], const float d[3], float halfW,
             float* u, float* v, float* t);

// the dock item under an arbitrary ray; pk.bar is set even when the ray
// lands on the strip between icons, so the bar body still blocks clicks
DockPick pickDockRay(const std::vector<DockItem>& items, float halfW,
                     float yaw, float pitch, const float origin[3],
                     const float o[3], const float d[3]);

// the dock item under the gaze ray
DockPick pickDock(const std::vector<DockItem>& items, float halfW,
                  float yaw, float pitch, const Mat4& head,
                  const float origin[3], const float o[3]);

// can a long-press pin this item: the quick button isn't pinnable
bool dockPinnable(const DockItem& it);

// add pkg to the end of the pin list or remove it; returns the new list
std::vector<std::string> pinToggle(const std::vector<std::string>& pins,
                                   const std::string& pkg);

// hidden panels parked on the shelf above the dock bar, in panel order
std::vector<ShelfItem> buildShelf(const std::vector<Panel>& panels);

// centre the icon row on a pill and return the pill's half-width
float shelfLayout(std::vector<ShelfItem>& items);

// the pill's centre and its top edge as lifts above the dock bar's centre
float shelfLift();
float shelfTop();

// shelf quad on the dock's anchor plane, raised shelfLift along its up
void shelfCenter(float yaw, float pitch, const float origin[3],
                 float c[3], float r[3], float up[3]);

// which icon a pill-local point hits; -1 on the body or in the gaps
int shelfItemAt(const std::vector<ShelfItem>& items, float halfW,
                float u, float v);

// gaze ray vs the shelf plane; u,v in pill coords, may fall outside -1..1
bool rayShelf(float yaw, float pitch, const float origin[3],
              const float o[3], const float d[3], float halfW,
              float* u, float* v, float* t);

// the shelf icon under an arbitrary ray; hit is set even on the pill body
// between icons, so the shelf blocks clicks like the dock bar does
ShelfPick pickShelfRay(const std::vector<ShelfItem>& items, float halfW,
                       float yaw, float pitch, const float origin[3],
                       const float o[3], const float d[3]);

// the shelf icon under the gaze ray
ShelfPick pickShelf(const std::vector<ShelfItem>& items, float halfW,
                    float yaw, float pitch, const Mat4& head,
                    const float origin[3], const float o[3]);
