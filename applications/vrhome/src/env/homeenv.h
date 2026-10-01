#pragma once

#include "../math/mat4.h"

struct Engine;

// Home-environment loader glue, env process only. envTick runs once per
// frame off drawFrame: it polls the selection prop on a slow timer and,
// when a custom environment is picked, reads <id>.zip from kEnvDir, pulls
// map.obj out and uploads the baked mesh to a VBO. Loads that miss leave
// envVerts at 0 so the scene falls back to the built-in sky+grid.
void envTick(Engine* e);

// draw the loaded environment mesh - caller already checked envVerts
void envDraw(Engine* e, const Mat4& viewProj);

// free the mesh buffer; safe with no context
void envRelease(Engine* e);
