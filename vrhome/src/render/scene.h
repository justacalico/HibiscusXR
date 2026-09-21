#pragma once

#include "../math/mat4.h"

struct Engine;

// env scenery only: sky backdrop + floor grid. The HUD draws its own chrome
// (panels + cursor) via render/chrome.h instead
void drawScene(Engine* e, const Mat4& viewProj);
