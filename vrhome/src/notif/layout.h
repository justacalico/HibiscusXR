#pragma once

#include "item.h"
#include "../math/mat4.h"

#include <vector>

// Notification card stack policy + geometry. Pure functions over plain
// data - no GL, no JNI - so ordering, layout and hit testing all run in
// the host tests. The stack rides the dock's anchor plane, lifted above
// the bar; over a covered app the toast anchors it to the summon yaw.

// the live stack: newest postMs first, capped at kNotifMax
std::vector<NotifItem> buildNotifs(const std::vector<NotifItem>& in);

// the subset still inside its visible window: a card shows from postMs
// until postMs + kNotifShowMs, then the dash drops it even though the
// shade record stays live. nowMs shares postMs's epoch (wall clock).
std::vector<NotifItem> visibleNotifs(const std::vector<NotifItem>& in,
                                     long long nowMs);

// half-height of a stack of `count` cards in metres
float notifStackHH(int count);

// lift above the dock bar's centre so the stack clears the strip
float notifLift(int count);

// stack quad on the dock's anchor cylinder, raised `lift` along its up
void notifCenter(float yaw, float pitch, float lift, const float origin[3],
                 float c[3], float r[3], float up[3]);

// card i's centre offset along +up inside the stack, metres; i=0 is the
// newest card and sits at the top
float notifCardY(int i, int count);

// the close badge's centre in card-local coords, metres from card centre
void notifBadgeAt(float* bx, float* by);

// a stack-local point -> card index + zone; -1 outside the cards (gaps)
int notifAt(float u, float v, int count, int* zone);

// gaze ray vs the stack plane; u,v in stack coords, may fall outside -1..1
bool rayNotif(float yaw, float pitch, float lift, const float origin[3],
              const float o[3], const float d[3], int count,
              float* u, float* v, float* t);

// the card under the gaze ray; stack is set even between cards so the
// region blocks clicks like the dock bar does
NotifPick pickNotif(const std::vector<NotifItem>& items,
                    float yaw, float pitch, float lift,
                    const Mat4& head, const float origin[3],
                    const float o[3]);
