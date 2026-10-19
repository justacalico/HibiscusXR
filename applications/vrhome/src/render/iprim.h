#pragma once

#include "../math/mat4.h"

struct HudEngine;

// Shape primitives for procedural icons: strokes, discs, arcs and boxes
// drawn out of shapeQuad inside a local square centred on `ic`, spanned
// by r/up. Shared by the quick panel's tile glyphs and the settings
// card's row/section icons.

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

// a wedge of a ring opening upward from an icon-local centre; rot steers
// the opening (0 = up)
void iconArc(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x, float y, float rad, float border, float rot,
             const float col[4]);

// an outlined rounded box in icon-local metres
void iconBox(HudEngine* e, const Mat4& vp, const float ic[3],
             const float r[3], const float up[3],
             float x, float y, float hw, float hh, float border,
             const float col[4]);

// four corner brackets, the crop_free/focus frame
void iconCorners(HudEngine* e, const Mat4& vp, const float ic[3],
                 const float r[3], const float up[3], float s,
                 float w, const float col[4]);
