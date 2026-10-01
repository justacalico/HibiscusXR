#pragma once

// HUD motion: every transition's easing, timing and interpolation lives
// here as pure functions so the whole animation model runs in the host
// tests. Times are CLOCK_MONOTONIC ms; progresses are 0..1 unless noted.

float clamp01(float t);

// easings: out lands soft from a fast start, in-out is symmetric for the
// flights that can reverse mid-run (minimize while restoring and back)
float easeOutCubic(float t);
float easeInOutCubic(float t);

// progress inside a duration window: 0 at startMs, 1 after ms. startMs<=0
// means "no timestamp recorded" - the transition reads as done so state
// from before the animation existed never plays a stale intro
float progT(long long startMs, long long nowMs, float ms);

// duration-based step toward a boolean target state, clamped 0..1 - the
// driver for reversible transitions (park flights, overlay open/close)
float stepT(float cur, bool toward, float dtMs, float ms);

// exponential approach with a millisecond time constant: framerate-proof
// smoothing for continuous chases like the icon hover scale. Snaps the
// last epsilon so a settled value compares equal to its target
float dampMs(float cur, float target, float dtMs, float tauMs);

// point lerp along a flight path
void lerp3(const float a[3], const float b[3], float t, float out[3]);

// hover scale -> 0..1 "how hovered" for fading the glow, ring and dots
// with the icon's own size instead of popping them on the flag; top is
// the hovered value (kHoverScale on the strip, kGridHoverScale on cells)
float hoverP(float hs, float top);

// panel spawn-in: the window grows in from kSpawnScale0 while its alpha
// runs ahead, so it reads before the scale finishes settling
float spawnScale(float t);
float spawnAlpha(float t);

// the minimize flight (and its reverse on restore). t is the raw park
// progress Panel::minT; geometry rides the eased curve, the quad shrinks
// onto its shelf slot, and the fade only bites in the last stretch so the
// window stays readable through most of the trip
float minEase(float t);
float minScale(float t);
float minAlpha(float t);

// the dash summon: the strip slides up from kDashDrop below its anchor
// while fading in. dashDrop returns the remaining downward offset in
// metres - draws shift the plane down by it and the picks take the same
// drop so the aim always lands where the strip is drawn
float dashDrop(float t);
float dashAlpha(float t);
