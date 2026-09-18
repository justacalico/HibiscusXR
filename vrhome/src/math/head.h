#pragma once

#include "mat4.h"

// Head-tracking chain, kept free of Android/GL so the whole transform can be
// exercised in the host unit tests. This is the path that produced the
// yaw-into-roll bug; keep it pure and tested.

// rotation-vector events may omit w; rebuild it from xyz when absent
float quatW(const float* data);

// full view chain: display roll * sensor matrix * sensor-roll * mount tilt.
// useSensor=false yields just the static roll (no tracking yet).
// pos is a world-space head position for 6DoF; NULL keeps 3DoF behaviour
Mat4 headMatrix(const float quat[4], bool transpose, float sensRoll,
                float worldX, float roll, bool useSensor,
                const float pos[3]);

// per-eye view: head matrix shifted ±ipd/2 along view-space x
Mat4 eyeMatrix(const Mat4& head, float ipd, int eye);

// rotate a QVR-frame position into the sensor world frame. rvQuat and
// qvrQuat describe the same physical orientation in their two frames, so
// delta = rvQuat * conj(qvrQuat) maps qvr vectors onto world vectors
void qvrPosToWorld(const float rvQuat[4], const float qvrQuat[4],
                   const float qvrPos[3], float out[3]);

// world-space direction the head's -z axis points
void gazeDir(const Mat4& head, float out[3]);

// horizontal yaw of the gaze direction. returns false when the head is
// pitched so far the horizontal projection vanishes (caller keeps last yaw)
bool gazeYaw(const Mat4& head, float* out);

// elevation of the gaze direction, radians; always defined
float gazePitch(const Mat4& head);

// yaw/pitch/roll in degrees for the HUD
void quatToYpr(const float q[4], float* yaw, float* pitch, float* roll);
