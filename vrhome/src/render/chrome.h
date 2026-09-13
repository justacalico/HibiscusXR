#pragma once

#include "../math/mat4.h"

struct Engine;

// window chrome: shadow + bottom bar + app surface + border + label
void drawPanels(Engine* e, const Mat4& viewProj);

// gaze cursor on the hovered panel: thin ring + centre dot
void drawCursor(Engine* e, const Mat4& viewProj);
