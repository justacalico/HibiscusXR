#pragma once

// Lens-warp maths for the eye pass, pure so the host tests can check the
// distortion field; the shader re-evaluates the same polynomial per pixel
// with these values as uniforms.
//
// r is the tan-angle radius off the lens axis: for the aspect-corrected
// 90-degree projection ndc = 2*uv and tan(theta) = (aspect*ndc.x, ndc.y),
// so r^2 = 4*((px*aspect)^2 + py^2) for a uv offset (px,py). r = 1 at the
// top/bottom edges of the 1920x2160 eye half, ~0.89 at the sides.

struct Warp {
    float cx, cy;          // lens centre in eye uv
    float aspect;          // w/h of the eye target
    float k0, k2, k4, k6;  // even powers of the lens polynomial
};

Warp makeWarp(int w, int h);
float warpR2(float px, float py, float aspect);
float warpScale(float r2, float k0, float k2, float k4, float k6);
