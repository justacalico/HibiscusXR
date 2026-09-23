#pragma once

// status-line visibility: the settings app's developer toggle is the
// normal control; the debug.vrhome.hud prop stays as an adb override
// both ways - a set value (>= 0) wins, unset (-1) follows the toggle
inline bool statusLineVisible(int hudProp, bool settingOn) {
    return hudProp >= 0 ? hudProp != 0 : settingOn;
}
