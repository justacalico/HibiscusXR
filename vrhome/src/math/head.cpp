#include "head.h"

#include <cmath>
#include <cstdio>
#include <cstring>

float quatW(const float* d) {
    if (d[3] != 0.0f) return d[3];
    const float sq = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
    return sq < 1.0f ? sqrtf(1.0f - sq) : 0.0f;
}

Mat4 headMatrix(const float quat[4], bool transpose, float sensRoll,
                float worldX, float roll, bool useSensor,
                const float pos[3]) {
    Mat4 head = useSensor ? quatToMat(quat, transpose) : identity();
    if (useSensor) {
        head = multiply(head, rotZ(sensRoll));
        head = multiply(head, rotX(worldX));
    }
    head = multiply(rotZ(roll), head);
    if (pos) {
        // view = R * T(-pos): the translation column is the world-space
        // head position negated through the rotation
        for (int i = 0; i < 3; ++i)
            head.m[12 + i] = -(head.m[i] * pos[0] + head.m[4 + i] * pos[1] +
                               head.m[8 + i] * pos[2]);
    }
    return head;
}

Mat4 eyeMatrix(const Mat4& head, float ipd, int eye) {
    // view = T(-eye) * head: the ±ipd/2 offset lives in view space so it
    // stays perpendicular to the gaze under head rotation
    Mat4 r = head;
    r.m[12] += (eye == 0 ? -ipd : ipd) / 2.0f;
    return r;
}

void qvrFoldPose(float quat[4], float headPos[3], bool* quatFromQvr,
                 const float qvrQuat[4], const float qvrPos[3]) {
    *quatFromQvr = true;
    memcpy(quat, qvrQuat, 4 * sizeof(float));
    memcpy(headPos, qvrPos, 3 * sizeof(float));
}

void sensorPosToWorld(const float pos[3], float sensRoll, float worldX,
                      float out[3]) {
    // C = rotZ(sensRoll)*rotX(worldX) maps GL onto sensor axes, so the way
    // back applies rotZ(-sensRoll) first then rotX(-worldX)
    const float cs = cosf(sensRoll * (float)M_PI / 180.0f);
    const float sn = sinf(sensRoll * (float)M_PI / 180.0f);
    const float x = cs * pos[0] + sn * pos[1];
    const float y = -sn * pos[0] + cs * pos[1];
    const float cw = cosf(worldX * (float)M_PI / 180.0f);
    const float sw = sinf(worldX * (float)M_PI / 180.0f);
    out[0] = x;
    out[1] = cw * y + sw * pos[2];
    out[2] = -sw * y + cw * pos[2];
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

static const char* arrowFor(float v, const char* pos, const char* neg) {
    if (v > 0.005f) return pos;
    if (v < -0.005f) return neg;
    return "\xC2\xB7"; // '·'
}

void fmtPosArrows(const float pos[3], char* out, size_t outSize) {
    // z negates so 'forward' (-z in this world) shows as the up-right arrow
    snprintf(out, outSize, " %s%.2f %s%.2f %s%.2f",
        arrowFor(pos[0], "\xE2\x86\x92", "\xE2\x86\x90"), fabsf(pos[0]),
        arrowFor(pos[1], "\xE2\x86\x91", "\xE2\x86\x93"), fabsf(pos[1]),
        arrowFor(-pos[2], "\xE2\x86\x97", "\xE2\x86\x99"), fabsf(pos[2]));
}
