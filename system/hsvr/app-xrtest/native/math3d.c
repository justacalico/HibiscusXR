#include "xrtest.h"

#include <math.h>
#include <string.h>

void mat4_proj(XrFovf fov, float zn, float zf, float *m) {
    float l = tanf(fov.angleLeft) * zn, r = tanf(fov.angleRight) * zn;
    float t = tanf(fov.angleUp) * zn, b = tanf(fov.angleDown) * zn;
    memset(m, 0, 64);
    m[0] = 2 * zn / (r - l);
    m[5] = 2 * zn / (t - b);
    m[8] = (r + l) / (r - l);
    m[9] = (t + b) / (t - b);
    m[10] = -(zf + zn) / (zf - zn);
    m[11] = -1;
    m[14] = -2 * zf * zn / (zf - zn);
}

// pose -> view matrix (inverse)
void mat4_view_from_pose(XrPosef p, float *m) {
    XrQuaternionf q = p.orientation;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    float R[9] = {
        1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy),
        2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx),
        2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy)};
    float tx = p.position.x, ty = p.position.y, tz = p.position.z;
    float ix = -(R[0] * tx + R[3] * ty + R[6] * tz);
    float iy = -(R[1] * tx + R[4] * ty + R[7] * tz);
    float iz = -(R[2] * tx + R[5] * ty + R[8] * tz);
    m[0] = R[0]; m[4] = R[3]; m[8] = R[6];  m[12] = ix;
    m[1] = R[1]; m[5] = R[4]; m[9] = R[7];  m[13] = iy;
    m[2] = R[2]; m[6] = R[5]; m[10] = R[8]; m[14] = iz;
    m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;
}

// pose -> model matrix
void mat4_model_from_pose(XrPosef p, float *m) {
    XrQuaternionf q = p.orientation;
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float R[9] = {
        1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
        2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
        2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)};
    m[0] = R[0]; m[4] = R[1]; m[8] = R[2];  m[12] = p.position.x;
    m[1] = R[3]; m[5] = R[4]; m[9] = R[5];  m[13] = p.position.y;
    m[2] = R[6]; m[6] = R[7]; m[10] = R[8]; m[14] = p.position.z;
    m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;
}

void mat4_mul(float *out, const float *a, const float *b) {
    float r[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[j * 4 + i] = a[i] * b[j * 4] + a[4 + i] * b[j * 4 + 1] +
                           a[8 + i] * b[j * 4 + 2] + a[12 + i] * b[j * 4 + 3];
    memcpy(out, r, sizeof(r));
}

void mat4_scale(float *m, float s) {
    memset(m, 0, 64);
    m[0] = s; m[5] = s; m[10] = s; m[15] = 1.0f;
}

void mat4_rot_y(float *m, float rad) {
    memset(m, 0, 64);
    float c = cosf(rad), s = sinf(rad);
    m[0] = c; m[2] = -s; m[5] = 1; m[8] = s; m[10] = c; m[15] = 1;
}

void mat4_translate(float *m, float x, float y, float z) {
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    m[12] = x; m[13] = y; m[14] = z;
}

// pose orientation applied to v (for controller aim rays)
void quat_rot(XrQuaternionf q, float vx, float vy, float vz, float *out) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    // v' = v + 2*cross(q.xyz, cross(q.xyz, v) + w*v)
    float cx = y * vz - z * vy + w * vx;
    float cy = z * vx - x * vz + w * vy;
    float cz = x * vy - y * vx + w * vz;
    out[0] = vx + 2 * (y * cz - z * cy);
    out[1] = vy + 2 * (z * cx - x * cz);
    out[2] = vz + 2 * (x * cy - y * cx);
}
