#pragma once

#include <string>
#include <vector>

// where on a system-message card a gaze hit lands
enum SysMsgZone {
    MZONE_NONE = -1,    // outside the card
    MZONE_BODY = 0,     // the card itself
    MZONE_BTN,          // one of the action buttons; pick.btn says which
};

// one crash/ANR card, as reported by the java dropbox watcher - id feeds
// click/dismiss back to java, pkg feeds the app icon, buttons holds the
// action labels (0 is always Close, 1 is Restart when present)
struct SysMsgItem {
    long long id = 0;
    std::string pkg;
    std::string title;
    std::string text;
    std::vector<std::string> buttons;
};

struct SysMsgPick {
    bool hit = false;       // the ray landed on the card at all
    int zone = MZONE_NONE;
    int btn = -1;
    float t = 1e9f;         // ray distance, for pick arbitration
};
