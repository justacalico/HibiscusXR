#pragma once

#include "../math/mat4.h"

#include "icon_ids.h"

struct Engine;
struct HudEngine;

// uploads the baked glyph atlas; called once GL is live from
// initGlResources, like the font atlas
void loadGlyphs(Engine* e);

// one Material Symbols glyph centred on ic in the plane's r/up frame.
// s is the glyph's ink half-size - the symbols carry a uniform padding
// inside their box, so the quad is grown past s and callers can keep
// passing the half-size the old procedural icons measured.
// toward shifts it at the viewer like shapeQuad's, col the tint. Runs
// through the text program with the atlas bound - the caller's
// blend/depth state applies and shapeProg is restored afterwards, same
// convention as drawIconTex
void drawGlyph(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s, float toward,
               const float col[4]);
