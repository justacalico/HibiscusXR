#pragma once

#include "../math/mat4.h"

// Controller pose -> world aim ray, kept pure for the host tests.
// The shared memory pose arrives in the service's sensor frame, so it goes
// through the same mount corrections as the head (sensRoll/worldX/roll).

// horizontal yaw of a direction vector; false when it points near-vertical
// and the horizontal projection is noise
bool dirYaw(const float d[3], float* out);

// elevation of a direction vector, radians
float dirPitch(const float d[3]);

// world aim ray for one controller. q is xyzw (the sharemem layout stores
// w first - callers convert). pos is the controller's sensor-frame
// position, posScale converts it to metres (stock writes mm-ish values);
// when the scaled position looks wrong the ray falls back to a hand offset
// under the eye so a controller still points instead of vanishing.
// o = ray origin, d = unit forward; model gets the orientation matrix back
// for drawing the mesh (translate it by o yourself)
// tq mirrors headMatrix's transpose flag so the controller rides the same
// quat convention as the head
void ctrlAim(const float q[4], const float pos[3], int which,
             float sensRoll, float worldX, float roll, bool tq,
             float posScale, const float eyePos[3], float o[3], float d[3],
             Mat4* model);
