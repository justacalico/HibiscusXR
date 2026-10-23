#include "anim.h"

#include "../common/config.h"

#include <cmath>

float clamp01(float t) {
    return t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
}

float easeOutCubic(float t) {
    const float u = 1.0f - clamp01(t);
    return 1.0f - u * u * u;
}

float easeInOutCubic(float t) {
    t = clamp01(t);
    return t < 0.5f ? 4.0f * t * t * t
                    : 1.0f - 4.0f * (1.0f - t) * (1.0f - t) * (1.0f - t);
}

float progT(long long startMs, long long nowMs, float ms) {
    if (startMs <= 0 || ms <= 0.0f) return 1.0f;
    return clamp01((float)(nowMs - startMs) / ms);
}

float stepT(float cur, bool toward, float dtMs, float ms) {
    if (ms <= 0.0f) return toward ? 1.0f : 0.0f;
    return clamp01(cur + (toward ? dtMs : -dtMs) / ms);
}

float dampMs(float cur, float target, float dtMs, float tauMs) {
    if (tauMs <= 0.0f || dtMs <= 0.0f) return cur;
    const float next = cur + (target - cur) * (1.0f - expf(-dtMs / tauMs));
    return fabsf(next - target) < 0.001f ? target : next;
}

float flingTrack(float vel, float dScroll, float dtMs) {
    if (dtMs <= 0.0f) return vel;
    return dampMs(vel, dScroll * 1000.0f / dtMs, dtMs, kFlingTrackMs);
}

float flingKick(float vel) {
    return fabsf(vel) < kFlingMinVel ? 0.0f : vel;
}

float flingScroll(float scroll, float* vel, float dtMs, float max) {
    if (*vel == 0.0f || dtMs <= 0.0f) return scroll;
    const float k = 1.0f - expf(-dtMs / kFlingTauMs);
    float next = scroll + *vel * kFlingTauMs * 0.001f * k;
    *vel *= expf(-dtMs / kFlingTauMs);
    if (next < 0.0f) next = 0.0f;
    else if (next > max) next = max;
    if (next <= 0.0f || next >= max || fabsf(*vel) < kFlingEps)
        *vel = 0.0f;
    return next;
}

void lerp3(const float a[3], const float b[3], float t, float out[3]) {
    for (int i = 0; i < 3; ++i) out[i] = a[i] + (b[i] - a[i]) * t;
}

float hoverP(float hs, float top) {
    return clamp01((hs - 1.0f) / (top - 1.0f));
}

float spawnScale(float t) {
    return kSpawnScale0 + (1.0f - kSpawnScale0) * easeOutCubic(t);
}

float spawnAlpha(float t) {
    // alpha leads the scale: t^0.5-ish without the sqrt, via easing a
    // faster progress so the surface is nearly opaque halfway in
    return easeOutCubic(clamp01(t * 1.6f));
}

float minEase(float t) {
    return easeInOutCubic(t);
}

float minScale(float t) {
    return 1.0f - easeInOutCubic(t);
}

float minAlpha(float t) {
    return 1.0f - clamp01((easeInOutCubic(t) - 0.55f) / 0.45f);
}

float closeEase(float t) {
    t = clamp01(t);
    return t * t * t;
}

float closeScale(float t) {
    return 1.0f - closeEase(t);
}

float closeAlpha(float t) {
    return 1.0f - easeInOutCubic(t);
}

float dashDrop(float t) {
    return kDashDrop * (1.0f - easeOutCubic(t));
}

float dashAlpha(float t) {
    return easeOutCubic(clamp01(t * 1.4f));
}
