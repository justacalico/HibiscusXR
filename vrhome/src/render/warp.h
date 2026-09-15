#pragma once

// Barrel-warp maths for the eye pass, pure so the host tests can check the
// distortion field; the shader re-evaluates the same polynomial per pixel
// with these values as uniforms.
//
// r is measured with x scaled by the eye aspect (w/h): each eye half is
// 1920x2160, so a uv-space circle is a pixel-space ellipse and the old
// field warped too much sideways - that asymmetry read as a fisheye.

struct Warp {
    float cx, cy;    // lens centre in eye uv
    float aspect;    // w/h of the eye target
    float k1, k2;    // radial coefficients
};

Warp makeWarp(int w, int h);
float warpR2(float px, float py, float aspect);
float warpScale(float r2, float k1, float k2);
