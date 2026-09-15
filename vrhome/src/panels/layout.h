#pragma once

#include "panel.h"
#include "../math/mat4.h"

#include <vector>

// Panel ring policy + geometry. Pure functions over a panel list so the whole
// layout is host-testable: no GL, no JNI.

// panel quad in world space, facing the viewer at the origin. `right` is the
// unit vector along the panel's right edge
void panelCenter(const Panel& p, float out[3], float right[3]);

// yaw of the next free ring slot around a centre yaw; centre when full
float freeSlotYaw(const std::vector<Panel>& panels, float centre);

// index of the oldest evictable panel (first that isn't the library
// launcher), or -1 when nothing can go
int evictIndex(const std::vector<Panel>& panels);

// snap every panel to its nearest ring slot around a new centre yaw
void recenterSlots(std::vector<Panel>& panels, float centre);

// half-width of the label pill under a window: hugs the text with side
// padding plus the button strip when the window has one, clamped inside the
// window's edges so it reads as a pill
float pillHalfWidth(float textW, float winHW, bool btns);

// widest the label may get before it must shrink to stay inside the pill
float pillTextLimit(float winHW, bool btns);

// x of the minimize/close button centres inside the pill, in world units
// measured from the pill centre toward its right edge
float pillMinX(float pillHW);
float pillCloseX(float pillHW);

// is (u,v) in panel coords inside the pill band under the window
bool onPill(float u, float v, float pillHW);

// which button a point on the pill hits: ZONE_MIN, ZONE_CLOSE or ZONE_LABEL
int pillButtonAt(float u, float v, float pillHW);

// how far under the panel centre the drag handle's centre hangs, world units
float handleDrop();

// is (u,v) in panel coords on the drag handle under the pill; the hit box is
// padded past the drawn line since gaze aim is coarse
bool onHandle(float u, float v);

// the panel in the middle of the ring - the only one that gets a drag
// handle. Picked by lowest total angular distance to the others, so the
// centre slot wins on a full ring; minimized panels don't count
int middleIndex(const std::vector<Panel>& panels);

// arm a ring drag: snapshot every panel's yaw so dragRing can reapply them
// offset by the gaze delta
void grabRing(std::vector<Panel>& panels);

// ring drag tick: shift every panel's yaw by the delta from its snapshot,
// keeping the ring's shape while the grabbed point tracks the gaze
void dragRing(std::vector<Panel>& panels, float delta);

// first minimized panel running pkg, or -1: relaunching an app whose window
// is minimized brings the same window back instead of opening a new one
int minimizedIndex(const std::vector<Panel>& panels, const std::string& pkg);

// gaze ray vs one panel's plane; u,v in panel coords, may fall outside -1..1
bool rayPanel(const Panel& p, const float d[3], float* u, float* v,
              float* t = nullptr);

struct Pick {
    int idx = -1;                // panel under the ray
    float u = 0, v = 0;          // hit point in panel coords, -1..1
    int zone = ZONE_NONE;        // which chrome part the hit landed on
};

// gaze ray (head's -z from the origin) vs all panels: the window rects plus
// the pill band under them; minimized panels are skipped. nearest wins
Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head);

// gaze point on one panel in display px, clamped inside the window so a held
// drag keeps streaming events after the cursor leaves the edges; false when
// the ray can never reach the panel's plane
bool dragPoint(const Panel& p, const Mat4& head, float* px, float* py);

// drag speed gain: the injected point runs ahead of the raw gaze point,
// measured from where the drag grabbed; clamps to the display edge
float dragBoost(float anchor, float p, float max);
