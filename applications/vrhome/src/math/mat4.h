#pragma once

// Minimal column-major 4x4 matrix math. No Android or GL dependencies so the
// same code runs on-device and in the host unit tests.

struct Mat4 { float m[16]; };

Mat4 identity();
Mat4 multiply(const Mat4& a, const Mat4& b);
Mat4 perspective(float fovYDeg, float aspect, float zn, float zf);
Mat4 rotZ(float deg);
Mat4 rotY(float deg);
Mat4 rotX(float deg);

// Quaternion (x,y,z,w) -> rotation matrix, column-major. inv=true transposes
// it: the sensor reports device->world and the view matrix wants the inverse.
Mat4 quatToMat(const float* q, bool inv);

// world-space direction a unit view-space vector points after view matrix V
// (rotation part only, orthonormal, so transpose == inverse)
void viewDirToWorld(const Mat4& v, const float in[3], float out[3]);

// quaternion helpers (x,y,z,w): product a*b applies b's rotation first,
// conj inverts a unit quat, rotate applies a quat to a vec3
void quatMul(const float a[4], const float b[4], float out[4]);
void quatConj(const float q[4], float out[4]);
void quatRotate(const float q[4], const float v[3], float out[3]);

// wrap an angle into [-pi, pi]
float wrapPi(float a);
