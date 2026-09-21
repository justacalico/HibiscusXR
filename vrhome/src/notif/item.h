#pragma once

#include <string>
#include <vector>

// where on a notification card a gaze hit lands
enum NotifZone {
    NZONE_NONE = -1,
    NZONE_BODY = 0,   // the card itself
    NZONE_CLOSE,      // the dismiss badge at the card's top-right
};

// one posted notification, as reported by the java listener - key feeds
// cancelNotification, pkg feeds the app icon, postMs orders the stack
struct NotifItem {
    std::string key;
    std::string pkg;
    std::string title;
    std::string text;
    long long postMs = 0;
    bool clearable = true;
};

struct NotifPick {
    int idx = -1;           // card under the ray, -1 for gaps between cards
    bool stack = false;     // the ray hit the stack region at all
    float t = 1e9f;         // ray distance, for pick arbitration
    int zone = NZONE_NONE;
};
