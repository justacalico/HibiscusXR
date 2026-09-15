#include "test.h"

#include "render/warp.h"
#include "common/config.h"

void testWarp() {
    Warp wp = makeWarp(1920, 2160);
    CHECK_F(wp.aspect, 1920.0f / 2160.0f, 1e-6f);
    CHECK_F(wp.cx, 0.5f, 1e-6f);
    CHECK_F(wp.cy, 0.5f, 1e-6f);
    CHECK_F(wp.k1, kDistK1, 1e-6f);
    CHECK_F(wp.k2, kDistK2, 1e-6f);

    // the field is circular in pixels: equal pixel distances on any axis
    // must give equal r2
    const float d = 500.0f;
    CHECK_F(warpR2(d / 1920.0f, 0.0f, wp.aspect),
            warpR2(0.0f, d / 2160.0f, wp.aspect), 1e-6f);
    CHECK_F(warpR2(d / 1920.0f, d / 2160.0f, wp.aspect),
            warpR2(0.0f, d * sqrtf(2.0f) / 2160.0f, wp.aspect), 1e-6f);

    // a square eye target reduces to the plain uv radius
    Warp sq = makeWarp(100, 100);
    CHECK_F(sq.aspect, 1.0f, 1e-6f);
    CHECK_F(warpR2(0.3f, 0.4f, sq.aspect), 0.25f, 1e-6f);

    // degenerate height can't divide by zero
    CHECK_F(makeWarp(100, 0).aspect, 1.0f, 1e-6f);

    // polynomial: identity at the centre, growing outward for barrel coeffs
    CHECK_F(warpScale(0.0f, wp.k1, wp.k2), 1.0f, 1e-6f);
    CHECK_F(warpScale(0.25f, 0.22f, 0.24f),
            1.0f + 0.22f * 0.25f + 0.24f * 0.0625f, 1e-6f);
    CHECK(warpScale(0.4f, wp.k1, wp.k2) > warpScale(0.2f, wp.k1, wp.k2));

    // horizontal correction weakens vs the old unscaled field on a
    // taller-than-wide eye; vertical stays the same
    CHECK(warpR2(0.5f, 0.0f, wp.aspect) < 0.25f);
    CHECK_F(warpR2(0.0f, 0.5f, wp.aspect), 0.25f, 1e-6f);
}
