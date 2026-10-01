#pragma once

#include "../math/mat4.h"

struct HudEngine;

// window chrome: shadow + bottom bar + app surface + border + label
void drawPanels(HudEngine* e, const Mat4& viewProj);

// the shared move pill: a short line centred under a quad, drawn in the
// quad's plane frame at `drop` below its centre; hot brightens it while
// the aim is on it or it is being dragged
void drawMovePill(HudEngine* e, const Mat4& viewProj, const float c[3],
                  const float r[3], const float up[3], float drop,
                  bool hot);

// the floating keyboard quad under its host panel
void drawKbd(HudEngine* e, const Mat4& viewProj);

// gaze cursor on the hovered panel: thin ring + centre dot
void drawCursor(HudEngine* e, const Mat4& viewProj);

// hold-to-recenter feedback: screen-space overlay ring that fills while
// the summon key is held, holdP 0..1
void drawHoldRing(HudEngine* e);
