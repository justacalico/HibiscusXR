#pragma once

#include "../math/mat4.h"

struct HudEngine;

// Shape primitives for the one icon still drawn procedurally, the About
// card's brand mark: strokes and discs out of shapeQuad inside a local
// square centred on `ic`, spanned by r/up. Everything else draws the
// baked Material Symbols atlas through drawGlyph.

// a straight stroke from (x0,y0) to (x1,y1) in icon-local metres, w its
// half-width
void iconSeg(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x0, float y0, float x1, float y1, float w,
             const float col[4]);

// a disc (border 0) or ring (border>0) centred on an icon-local offset
void iconDisc(HudEngine* e, const Mat4& vp, const float ic[3],
              const float r[3], const float up[3],
              float x, float y, float rad, float border,
              const float col[4]);
