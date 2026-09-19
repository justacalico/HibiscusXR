#pragma once

#include "mat4.h"

#include <cstddef>

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

// a valid QVR pose always wins: its quat and position share one tracking
// frame so both are taken raw, and rot-vec stays a pure fallback. mapping
// the position through a live quat delta was tried and dropped - the two
// trackers' yaws drift independently so no fixed delta exists
void qvrFoldPose(float quat[4], float headPos[3], bool* quatFromQvr,
                 const float qvrQuat[4], const float qvrPos[3]);

// undo the mount corrections on a sensor-world position. headMatrix builds
// R = rotZ(roll)*Q*rotZ(sensRoll)*rotX(worldX) where the last two map GL
// world axes onto the sensor's, so positions arriving in sensor coords
// come back to GL world through the inverse
void sensorPosToWorld(const float pos[3], float sensRoll, float worldX,
                      float out[3]);

// world-space direction the head's -z axis points
void gazeDir(const Mat4& head, float out[3]);

// horizontal yaw of the gaze direction. returns false when the head is
// pitched so far the horizontal projection vanishes (caller keeps last yaw)
bool gazeYaw(const Mat4& head, float* out);

// elevation of the gaze direction, radians; always defined
float gazePitch(const Mat4& head);

// recenter direction from the head: gaze yaw/pitch when the gaze is near
// enough horizontal. When the head points near-vertical - headset flat on
// a desk - the horizontal projection collapses and yaw is noise; the head's
// right axis stays horizontal through the pitch so the heading comes back
// from it instead, and the pitch resets level so the ring sits on the
// horizon for when the headset is picked up. false when it had to guess
bool recenterAngles(const Mat4& head, float* yaw, float* pitch);

// yaw/pitch/roll in degrees for the HUD
void quatToYpr(const float q[4], float* yaw, float* pitch, float* roll);

// world position as direction arrows for the HUD debug line, e.g.
// " →0.42 ↑0.10 ↗0.05": x is →/←, y is ↑/↓, z is ↗ fwd(-z)/↙ back(+z);
// '·' under 5mm so the arrows don't flicker at rest
void fmtPosArrows(const float pos[3], char* out, size_t outSize);
