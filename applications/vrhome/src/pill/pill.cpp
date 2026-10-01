#include "pill.h"

#include "../common/config.h"

#include <cmath>

float movePillDrop(float hh) {
    return hh + kHandleGap + kHandleT;
}

bool onMovePill(float u, float v, float hw, float hh) {
    const float x = u * hw, y = v * hh;
    return fabsf(x) <= kHandleW + kHandlePad &&
           fabsf(y + movePillDrop(hh)) <= kHandleT + kHandlePad;
}
