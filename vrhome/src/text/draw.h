#pragma once

#include "../math/mat4.h"

struct Engine;

// measure a string in metres, baking any missing glyphs first
float measureText(Engine* e, const char* utf8, float mPerPx);

// text laid flat on the z plane; returns the pen advance in metres
float drawText(Engine* e, const char* utf8, float x, float y, float z,
               float mPerPx);

// text on a panel plane: o is the baseline start in world space, r and up the
// plane's edge vectors so glyphs tilt with a pitched window. bold > 0
// double-strikes each glyph offset that far along +x (in metres)
void drawTextPanel(Engine* e, const char* utf8, const float o[3],
                   const float r[3], const float up[3], float mPerPx,
                   float bold = 0.0f);

// HUD: head-locked status line so the pipeline can be verified without adb
void drawHud(Engine* e, const Mat4& proj);
