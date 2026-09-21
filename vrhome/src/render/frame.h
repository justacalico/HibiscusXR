#pragma once

#include "../math/mat4.h"

struct Engine;

// the per-frame pipeline both apps share: each renders its own scene into
// two eye buffers, then warps both halves onto the physical panel.
// translucent=true clears the eye buffers fully transparent (the HUD draws
// over other apps); false paints the env's dark backdrop.
// status=true draws the head-locked debug line (the HUD's own overlay);
// the env leaves it off so the HUD's copy is the only one on screen
void drawEyes(Engine* e, const Mat4& head, const Mat4& proj, bool translucent,
              bool status, void (*scene)(Engine*, const Mat4&));

// warp pass: each eye texture through barrel distortion to its half
void warpPresent(Engine* e);

// rolling one-second fps/sensor counters
void updateFps(Engine* e);

// head-locked status line text (drawn by drawEyes' caller via drawHud)
void updateHud(Engine* e, const char* extra);
