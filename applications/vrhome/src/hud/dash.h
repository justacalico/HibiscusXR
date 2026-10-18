#pragma once

#include "../math/mat4.h"

struct Engine;
struct HudEngine;

void anchorDash(HudEngine* e, const Mat4& head);
void drawDashScene(Engine* e, const Mat4& vp);
void tickDash(HudEngine* e, const Mat4& head, long long nowMs);
