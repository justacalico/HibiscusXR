#include "keys.h"

#include "../common/config.h"

bool isConfirm(int code) {
    return code == kKeyEnter || code == kKeyDpadCenter ||
           code == kKeyButtonA || code == kPicoConfirm;
}
