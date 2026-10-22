#include "keys.h"

#include "../common/config.h"

bool isConfirm(int code) {
    return code == kKeyEnter || code == kKeyDpadCenter ||
           code == kKeyButtonA || code == kHwConfirm;
}

int escAction(bool shiftHeld) {
    return shiftHeld ? kKeyBack : kKeyMinimize;
}

int grabAction(bool shiftHeld) {
    return shiftHeld ? kGrabOff : kGrabToggle;
}
