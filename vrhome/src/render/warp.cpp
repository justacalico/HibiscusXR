#include "warp.h"

#include "../common/config.h"

Warp makeWarp(int w, int h) {
    Warp wp;
    wp.cx = 0.5f;
    wp.cy = 0.5f;
    wp.aspect = h > 0 ? (float)w / (float)h : 1.0f;
    wp.k0 = kLensK0;
    wp.k2 = kLensK2;
    wp.k4 = kLensK4;
    wp.k6 = kLensK6;
    return wp;
}

float warpR2(float px, float py, float aspect) {
    const float x = px * aspect;
    return 4.0f * (x * x + py * py);
}

float warpScale(float r2, float k0, float k2, float k4, float k6) {
    return k0 + r2 * (k2 + r2 * (k4 + r2 * k6));
}
