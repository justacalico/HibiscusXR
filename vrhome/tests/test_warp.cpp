#include "test.h"

#include "render/warp.h"
#include "common/config.h"

void testWarp() {
    Warp wp = makeWarp(1920, 2160);
    CHECK_F(wp.aspect, 1920.0f / 2160.0f, 1e-6f);
    CHECK_F(wp.cx, 0.5f, 1e-6f);
    CHECK_F(wp.cy, 0.5f, 1e-6f);
    CHECK_F(wp.k0, kLensK0, 1e-6f);
    CHECK_F(wp.k2, kLensK2, 1e-6f);
    CHECK_F(wp.k4, kLensK4, 1e-6f);
    CHECK_F(wp.k6, kLensK6, 1e-6f);

    // the field is circular in pixels: equal pixel distances on any axis
    // must give equal r2
    const float d = 500.0f;
    CHECK_F(warpR2(d / 1920.0f, 0.0f, wp.aspect),
            warpR2(0.0f, d / 2160.0f, wp.aspect), 1e-6f);
    CHECK_F(warpR2(d / 1920.0f, d / 2160.0f, wp.aspect),
            warpR2(0.0f, d * sqrtf(2.0f) / 2160.0f, wp.aspect), 1e-6f);

    // r2 is the squared tan-angle radius: r = 1 at the top/bottom edge of
    // the 1920x2160 half (45 deg vertical), ~0.89 at the side edges
    CHECK_F(warpR2(0.0f, 0.5f, wp.aspect), 1.0f, 1e-6f);
    CHECK_F(warpR2(0.5f, 0.0f, wp.aspect), 0.790123f, 1e-5f);

    // a square eye target reduces to the ndc radius
    Warp sq = makeWarp(100, 100);
    CHECK_F(sq.aspect, 1.0f, 1e-6f);
    CHECK_F(warpR2(0.3f, 0.4f, sq.aspect), 1.0f, 1e-6f);

    // degenerate height can't divide by zero
    CHECK_F(makeWarp(100, 0).aspect, 1.0f, 1e-6f);

    // Pico field: 0.74 on the axis magnifies the centre ~1.35x, crosses
    // 1.0 mid-field, ends at 1.129 on the top/bottom edge
    const float s0 = warpScale(0.0f, wp.k0, wp.k2, wp.k4, wp.k6);
    CHECK_F(s0, kLensK0, 1e-6f);
    CHECK(s0 < 1.0f);
    const float s1 = warpScale(1.0f, wp.k0, wp.k2, wp.k4, wp.k6);
    CHECK_F(s1, kLensK0 + kLensK2 + kLensK4 + kLensK6, 1e-6f);
    CHECK_F(s1, 1.1290389f, 1e-5f);
    CHECK(s1 > 1.0f);
    // monotonic across the visible field
    CHECK(warpScale(0.8f, wp.k0, wp.k2, wp.k4, wp.k6) >
          warpScale(0.4f, wp.k0, wp.k2, wp.k4, wp.k6));
}
