#pragma once

struct Engine;

// drain the rotation-vector queue into e->quat. MUST run inside the looper
// poll, else the pending events keep the looper non-idle and the frame loop
// never runs
void drainSensor(Engine* e);
