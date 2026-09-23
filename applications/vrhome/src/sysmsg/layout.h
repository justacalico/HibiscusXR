#pragma once

#include "item.h"
#include "../math/mat4.h"

#include <vector>

// System-message card policy + geometry. Pure functions over plain data -
// no GL, no JNI - so layout and hit testing run in the host tests. The
// card rides the dock's anchor plane lifted to eye level in the dash, or
// the summon yaw at eye level while a covered app owns the display.

// card quad on the dock's anchor cylinder, raised `lift` along its up
void sysMsgCenter(float yaw, float pitch, float lift, const float origin[3],
                  float c[3], float r[3], float up[3]);

// button row geometry in card-local metres: n buttons share the inner
// width evenly, row centre sysMsgBtnY() below the card centre
float sysMsgBtnHW(int n);
float sysMsgBtnX(int i, int n);
float sysMsgBtnY();

// a card-local point -> zone; returns MZONE_NONE outside the card, *btn
// gets the button index on MZONE_BTN
int sysMsgAt(float u, float v, int nbtn, int* btn);

// gaze ray vs the card plane; u,v in card coords, may fall outside -1..1
bool raySysMsg(float yaw, float pitch, float lift, const float origin[3],
               const float o[3], const float d[3],
               float* u, float* v, float* t);

// the card under an arbitrary ray; only the front card is drawn so only
// it picks
SysMsgPick pickSysMsgRay(const std::vector<SysMsgItem>& items,
                         float yaw, float pitch, float lift,
                         const float origin[3], const float o[3],
                         const float d[3]);

// the card under the gaze ray
SysMsgPick pickSysMsg(const std::vector<SysMsgItem>& items,
                      float yaw, float pitch, float lift,
                      const Mat4& head, const float origin[3],
                      const float o[3]);
