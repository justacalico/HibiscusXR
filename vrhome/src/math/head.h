#pragma once

#include "mat4.h"

// Head-tracking chain, kept free of Android/GL so the whole transform can be
// exercised in the host unit tests. This is the path that produced the
// yaw-into-roll bug; keep it pure and tested.

// rotation-vector events may omit w; rebuild it from xyz when absent
float quatW(const float* data);

// full view chain: display roll * sensor matrix * sensor-roll * mount tilt.
// useSensor=false yields just the static roll (no tracking yet)
Mat4 headMatrix(const float quat[4], bool transpose, float sensRoll,
                float worldX, float roll, bool useSensor);

// per-eye view: head matrix shifted ±ipd/2 along view-space x
Mat4 eyeMatrix(const Mat4& head, float ipd, int eye);

// world-space direction the head's -z axis points
void gazeDir(const Mat4& head, float out[3]);

// horizontal yaw of the gaze direction. returns false when the head is
// pitched so far the horizontal projection vanishes (caller keeps last yaw)
bool gazeYaw(const Mat4& head, float* out);

// yaw/pitch/roll in degrees for the HUD
void quatToYpr(const float q[4], float* yaw, float* pitch, float* roll);
