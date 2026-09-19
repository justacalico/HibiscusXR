#pragma once

#include "../math/mat4.h"

struct HudEngine;

// Notification glue: bridge pulls, textures and dismiss - the impure half
// of the card stack; ordering/layout/hit-test policy lives in layout.cpp
// so it can run on the host.

// per-frame, render thread: pull the live notification set when the
// listener's version counter moved, resolve card icons
void syncNotifs(HudEngine* e);

// the card stack at an anchor: dash mode lifts it over the dock bar,
// toast mode centres it on the gaze yaw at eye level
void drawNotifStack(HudEngine* e, const Mat4& vp, float yaw, float pitch,
                    float lift);

// a card's dismiss badge: cancel the notification through the listener
void notifDismiss(HudEngine* e, int idx);
