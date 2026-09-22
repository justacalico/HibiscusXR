#pragma once

#include "../math/mat4.h"

struct HudEngine;

// both live controllers at their shared-memory poses; the active one's beam
// runs to the picked surface. Loads the embedded meshes on first call
void drawControllers(HudEngine* e, const Mat4& viewProj);
