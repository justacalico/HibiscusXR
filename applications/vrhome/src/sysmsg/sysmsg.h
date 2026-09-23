#pragma once

#include "../math/mat4.h"

struct HudEngine;

// System-message glue: bridge pulls, textures and click dispatch - the
// impure half of the crash/ANR card; layout and hit-test policy lives in
// layout.cpp so it can run on the host.

// per-frame, render thread: pull the live card set when the dropbox
// watcher's version counter moved, resolve card icons
void syncSysMsgs(HudEngine* e);

// the front card at an anchor: dash mode lifts it off the dock's plane to
// eye level, covered mode centres it on the gaze yaw at card pitch
void drawSysMsg(HudEngine* e, const Mat4& vp, float yaw, float pitch,
                float lift);

// a confirm release on a button: java maps index to action - 0 closes
// the card, 1 relaunches the app
void sysMsgBtnClick(HudEngine* e, int btn);

// BACK or a body release never fires an action: drop the front card
void sysMsgDismiss(HudEngine* e);
