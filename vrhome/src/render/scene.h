#pragma once

#include "../math/mat4.h"

struct Engine;

// sky backdrop + floor grid + all floating panels + gaze cursor
void drawScene(Engine* e, const Mat4& viewProj);
