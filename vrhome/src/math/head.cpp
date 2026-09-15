#include "head.h"

#include <cmath>

float quatW(const float* d) {
    if (d[3] != 0.0f) return d[3];
    const float sq = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
    return sq < 1.0f ? sqrtf(1.0f - sq) : 0.0f;
}

Mat4 headMatrix(const float quat[4], bool transpose, float sensRoll,
                float worldX, float roll, bool useSensor) {
    Mat4 head = useSensor ? quatToMat(quat, transpose) : identity();
    if (useSensor) {
        head = multiply(head, rotZ(sensRoll));
        head = multiply(head, rotX(worldX));
    }
    return multiply(rotZ(roll), head);
}

Mat4 eyeMatrix(const Mat4& head, float ipd, int eye) {
    Mat4 shift = identity();
    shift.m[12] = (eye == 0 ? -ipd : ipd) / 2.0f;
    return multiply(head, shift);
}

void gazeDir(const Mat4& head, float out[3]) {
    const float fwd[3] = {0.0f, 0.0f, -1.0f};
    viewDirToWorld(head, fwd, out);
}

bool gazeYaw(const Mat4& head, float* out) {
    float d[3];
    gazeDir(head, d);
    if (d[0]*d[0] + d[2]*d[2] < 1e-6f) return false;
    *out = atan2f(d[0], -d[2]);
    return true;
}

float gazePitch(const Mat4& head) {
    float d[3];
    gazeDir(head, d);
    return asinf(fmaxf(-1.0f, fminf(1.0f, d[1])));
}

void quatToYpr(const float q[4], float* yaw, float* pitch, float* roll) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    *yaw   = atan2f(2*(w*y + x*z), 1 - 2*(y*y + x*x)) * 180.0f / (float)M_PI;
    *pitch = asinf(fmaxf(-1.0f, fminf(1.0f, 2*(w*x - y*z)))) * 180.0f / (float)M_PI;
    *roll  = atan2f(2*(w*z + x*y), 1 - 2*(z*z + x*x)) * 180.0f / (float)M_PI;
}
