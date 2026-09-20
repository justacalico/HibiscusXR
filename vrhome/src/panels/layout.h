#pragma once

#include "panel.h"
#include "../math/mat4.h"

#include <vector>

// Panel ring policy + geometry. Pure functions over a panel list so the whole
// layout is host-testable: no GL, no JNI.

// point on the ring cylinder: a circle of `dist` around the anchor, pitch
// lifting along it, plane normal facing the anchor. y0 is the ring's base
// height off the anchor; `right` runs along the quad's right edge and `up`
// its top edge, tilted so an elevated quad still looks at you
void ringPoint(float yaw, float pitch, float dist, float y0,
               const float origin[3], float out[3], float right[3],
               float up[3]);

// panel quad in world space, facing the viewer at `origin` - the point the
// whole ring hangs around, re-anchored to the head's position on recenter.
// `right` is the unit vector along the panel's right edge, `up` its top
// edge - the plane tilts with pitch so an elevated window still looks at you
void panelCenter(const Panel& p, const float origin[3], float out[3],
                 float right[3], float up[3]);

// yaw of the next free ring slot around a centre yaw; centre when full
float freeSlotYaw(const std::vector<Panel>& panels, float centre);

// the ring's current elevation: panels share one pitch, so a window opened
// while the ring is raised joins at the same height instead of the horizon
float ringPitch(const std::vector<Panel>& panels);

// index of the oldest evictable panel (first that isn't the library
// launcher), or -1 when nothing can go
int evictIndex(const std::vector<Panel>& panels);

// index of the library panel, or -1 - there is at most one
int libraryIndex(const std::vector<Panel>& panels);

// snap every panel to its nearest ring slot around a new centre yaw and pull
// the whole ring to the given elevation
void recenterSlots(std::vector<Panel>& panels, float centre, float pitch);

// widest the label may get before it must shrink to stay inside the top
// bar: the bar spans the window's full width, so the text region is what
// the left pad and the button strip leave over
float barTextLimit(float winHW, bool btns);

// x of the minimize/close button centres inside the bar, in world units
// measured from the bar centre toward its right edge
float barMinX(float winHW);
float barCloseX(float winHW);

// is (u,v) in panel coords inside the top bar band above the window
bool onBar(float u, float v);

// which button a point on the bar hits: ZONE_MIN, ZONE_CLOSE or ZONE_LABEL
int barButtonAt(float u, float v);

// how far under the panel centre the drag handle's centre hangs, world units
float handleDrop();

// is (u,v) in panel coords on the drag handle under the window; the hit box
// is padded past the drawn line since gaze aim is coarse
bool onHandle(float u, float v);

// the panel in the middle of the ring - the only one that gets a drag
// handle. Picked by lowest total angular distance to the others, so the
// centre slot wins on a full ring; minimized panels don't count
int middleIndex(const std::vector<Panel>& panels);

// arm a ring drag: snapshot every panel's yaw so dragRing can reapply them
// offset by the gaze delta
void grabRing(std::vector<Panel>& panels);

// ring drag tick: shift every panel by the gaze delta from its snapshot -
// yaw wraps around the ring, pitch elevates the whole ring and is clamped so
// the windows can't flip over the poles
void dragRing(std::vector<Panel>& panels, float dYaw, float dPitch);

// first minimized panel running pkg, or -1: relaunching an app whose window
// is minimized brings the same window back instead of opening a new one
int minimizedIndex(const std::vector<Panel>& panels, const std::string& pkg);

// gaze ray vs a quad centred at c, spanned by r/up with half extents hw/hh;
// the quad's normal is derived toward `viewer` (the ring anchor). u,v in
// quad coords, may fall outside -1..1
bool rayQuad(const float c[3], const float r[3], const float up[3],
             const float viewer[3], const float o[3], const float d[3],
             float hw, float hh, float* u, float* v, float* t);

// gaze ray vs one panel's plane; u,v in panel coords, may fall outside -1..1.
// The ray starts at o - the head's live position - while the plane sits on
// the ring anchored at origin; the two differ once the head moves
bool rayPanel(const Panel& p, const float origin[3], const float o[3],
              const float d[3], float* u, float* v, float* t = nullptr);

struct Pick {
    int idx = -1;                // panel under the ray
    float u = 0, v = 0;          // hit point in panel coords, -1..1
    int zone = ZONE_NONE;        // which chrome part the hit landed on
    float t = 1e9f;              // ray distance, for pick arbitration
};

// gaze ray (head's -z, starting at the live eye position o) vs all panels:
// the window rects plus the top bar above them; minimized panels are
// skipped. nearest wins
Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head,
               const float origin[3], const float o[3]);

// gaze point on one panel in display px, clamped inside the window so a held
// drag keeps streaming events after the cursor leaves the edges; false when
// the ray can never reach the panel's plane
bool dragPoint(const Panel& p, const Mat4& head, const float origin[3],
               const float o[3], float* px, float* py);

// drag speed gain: the injected point runs ahead of the raw gaze point,
// measured from where the drag grabbed; clamps to the display edge
float dragBoost(float anchor, float p, float max);
