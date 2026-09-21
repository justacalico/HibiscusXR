#pragma once

#include "../math/mat4.h"

struct HudEngine;

// window chrome: shadow + bottom bar + app surface + border + label
void drawPanels(HudEngine* e, const Mat4& viewProj);

// gaze cursor on the hovered panel: thin ring + centre dot
void drawCursor(HudEngine* e, const Mat4& viewProj);

// hold-to-recenter feedback: screen-space overlay ring that fills while
// the summon key is held, holdP 0..1
void drawHoldRing(HudEngine* e);
