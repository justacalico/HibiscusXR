#pragma once

#include "../math/mat4.h"

struct HudEngine;

// one rounded quad on a plane centred at c, spanned by r and up.
// toward>0 shifts it toward the viewer so layered chrome never z-fights the
// surface under it. ang rotates the quad inside the plane's frame so a
// capsule can tilt. qw/qh is the quad's half extent, bw/bh the shape's own:
// when they differ a soft edge can spread past the box into the quad's
// padding - pass a negative border for a fill-to-box-then-fade shadow.
// radB rounds the bottom corners separately - pass 0 for a shape that
// should sit flush on top of a square edge; <0 reuses radius for both
void shapeQuad(HudEngine* e, const Mat4& vp, const float c[3],
               const float r[3], const float up[3],
               float toward, float ang, float qw, float qh,
               float bw, float bh, float radius, float border, float soft,
               const float col[4], float radB = -1.0f);
