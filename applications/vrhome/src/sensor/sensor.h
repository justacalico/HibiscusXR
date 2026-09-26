#pragma once

struct Engine;

// drain the rotation-vector queue into e->quat. MUST run inside the looper
// poll, else the pending events keep the looper non-idle and the frame loop
// never runs
void drainSensor(Engine* e);

// tick the view-pose filter once a frame and rewrite e->quat / e->headPos
// with the smoothed pose, so the view, the gaze ray and recentering all run
// on it. useSensor=false (or debug.vrhome.posefilt=0) resets the filter:
// the next tracked frame snaps instead of easing in from a stale pose
void smoothPose(Engine* e, bool useSensor);
