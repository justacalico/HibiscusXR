#include "mat4.h"

#include <cmath>

Mat4 identity() {
    Mat4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f; return r;
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 4; ++i) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + i] * b.m[c * 4 + k];
            r.m[c * 4 + i] = s;
        }
    return r;
}

Mat4 perspective(float fovYDeg, float aspect, float zn, float zf) {
    Mat4 r{};
    const float f = 1.0f / tanf(fovYDeg * (float)M_PI / 360.0f);
    r.m[0] = f / aspect; r.m[5] = f;
    r.m[10] = (zf + zn) / (zn - zf); r.m[11] = -1.0f;
    r.m[14] = (2.0f * zf * zn) / (zn - zf);
    return r;
}

Mat4 rotZ(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[0] = cosf(r); m.m[1] = sinf(r); m.m[4] = -sinf(r); m.m[5] = cosf(r);
    return m;
}

Mat4 rotY(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[0] = cosf(r); m.m[2] = -sinf(r); m.m[8] = sinf(r); m.m[10] = cosf(r);
    return m;
}

Mat4 rotX(float deg) {
    const float r = deg * (float)M_PI / 180.0f;
    Mat4 m = identity();
    m.m[5] = cosf(r); m.m[6] = sinf(r); m.m[9] = -sinf(r); m.m[10] = cosf(r);
    return m;
}

Mat4 quatToMat(const float* q, bool inv) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    Mat4 r = identity();
    float f[9] = {
        1-2*(y*y+z*z), 2*(x*y-z*w),   2*(x*z+y*w),
        2*(x*y+z*w),   1-2*(x*x+z*z), 2*(y*z-x*w),
        2*(x*z-y*w),   2*(y*z+x*w),   1-2*(x*x+y*y),
    };
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < 3; ++i)
            r.m[c*4+i] = inv ? f[c*3+i] : f[i*3+c];
    return r;
}

void viewDirToWorld(const Mat4& v, const float in[3], float out[3]) {
    out[0] = v.m[0]*in[0] + v.m[1]*in[1] + v.m[2]*in[2];
    out[1] = v.m[4]*in[0] + v.m[5]*in[1] + v.m[6]*in[2];
    out[2] = v.m[8]*in[0] + v.m[9]*in[1] + v.m[10]*in[2];
}

void quatMul(const float* a, const float* b, float* o) {
    o[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
    o[1] = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
    o[2] = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
    o[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
}

void quatConj(const float* q, float* o) {
    o[0] = -q[0]; o[1] = -q[1]; o[2] = -q[2]; o[3] = q[3];
}

void quatRotate(const float* q, const float* v, float* o) {
    const float tx = 2.0f * (q[1]*v[2] - q[2]*v[1]);
    const float ty = 2.0f * (q[2]*v[0] - q[0]*v[2]);
    const float tz = 2.0f * (q[0]*v[1] - q[1]*v[0]);
    o[0] = v[0] + q[3]*tx + q[1]*tz - q[2]*ty;
    o[1] = v[1] + q[3]*ty + q[2]*tx - q[0]*tz;
    o[2] = v[2] + q[3]*tz + q[0]*ty - q[1]*tx;
}

float wrapPi(float a) {
    while (a > (float)M_PI)  a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}
