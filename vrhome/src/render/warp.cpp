#include "warp.h"

#include "../common/config.h"

Warp makeWarp(int w, int h) {
    Warp wp;
    wp.cx = 0.5f;
    wp.cy = 0.5f;
    wp.aspect = h > 0 ? (float)w / (float)h : 1.0f;
    wp.k1 = kDistK1;
    wp.k2 = kDistK2;
    return wp;
}

float warpR2(float px, float py, float aspect) {
    const float x = px * aspect;
    return x * x + py * py;
}

float warpScale(float r2, float k1, float k2) {
    return 1.0f + k1 * r2 + k2 * r2 * r2;
}
