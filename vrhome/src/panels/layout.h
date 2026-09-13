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
// padding, clamped inside the window's edges so it reads as a pill
float pillHalfWidth(float textW, float winHW);

// widest the label may get before it must shrink to stay inside the pill
float pillTextLimit(float winHW);

struct Pick {
    int idx = -1;         // panel under the ray
    float u = 0, v = 0;   // hit point in panel coords, -1..1
};

// gaze ray (head's -z from the origin) vs all panel rects; nearest wins
Pick pickPanel(const std::vector<Panel>& panels, const Mat4& head);
