#include "aim.h"

#include "ctrl_state.h"
#include "../math/head.h"

#include <cmath>
#include <cstring>

bool dirYaw(const float d[3], float* out) {
    if (d[0]*d[0] + d[2]*d[2] < 1e-6f) return false;
    *out = atan2f(d[0], -d[2]);
    return true;
}

float dirPitch(const float d[3]) {
    return asinf(fmaxf(-1.0f, fminf(1.0f, d[1])));
}

void ctrlAim(const float q[4], const float pos[3], int which,
             float sensRoll, float worldX, float roll, bool tq,
             float posScale, const float eyePos[3], float o[3], float d[3],
             Mat4* model) {
    // same chain as headMatrix: view = rotZ(roll) * Q * rotZ(sensRoll) *
    // rotX(worldX); the controller forward is its own -z
    Mat4 m = quatToMat(q, tq);
    m = multiply(m, rotZ(sensRoll));
    m = multiply(m, rotX(worldX));
    m = multiply(rotZ(roll), m);
    if (model) *model = m;
    const float fwd[3] = {0.0f, 0.0f, -1.0f};
    viewDirToWorld(m, fwd, d);

    // the position rides the same frame as headPos: undo the mount
    // corrections, then scale the service's units down to metres
    float w[3];
    sensorPosToWorld(pos, sensRoll, worldX, w);
    const float mag = sqrtf(w[0]*w[0] + w[1]*w[1] + w[2]*w[2]) * posScale;
    if (mag > 0.05f && mag < 8.0f) {
        for (int i = 0; i < 3; ++i) o[i] = w[i] * posScale;
        return;
    }

    // no usable position: park the controller under the eye, offset to its
    // side - the aim still swings with orientation
    const float side = which == CTRL_LEFT ? -1.0f : 1.0f;
    const float local[3] = {0.18f * side, -0.25f, -0.20f};
    viewDirToWorld(m, local, w);
    for (int i = 0; i < 3; ++i) o[i] = eyePos[i] + w[i];
}
