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

// where on the notification shade a gaze hit lands
enum ShadeZone {
    SHZONE_NONE = -1,
    SHZONE_ROW = 0,     // a notification row's body: holds a scroll drag
    SHZONE_DISMISS,     // a row's dismiss badge; idx = row
    SHZONE_CLEAR,       // the title row's clear-all pill
    SHZONE_CLOSE,       // the title row's close disc
    SHZONE_BODY,        // the card itself: holds a scroll drag, eats the tap
};

struct ShadePick {
    int idx = -1;           // row under the ray, -1 elsewhere on the card
    int zone = SHZONE_NONE;
    bool hit = false;       // the ray met the card at all (blocks panels)
    float u = 0, v = 0;     // hit point in card coords, -1..1
    float t = 1e9f;         // ray distance, for pick arbitration
};

// the shade overlay's live state on the engine: the rows come from
// notifsAll so there's no list to rebuild - the rest is per-frame input
// bookkeeping like the app grid's. clearHW is the clear-all pill's
// measured half width, fed by the glue so hit-testing stays pure
struct ShadeOverlay {
    bool shown = false;
    float openT = 0.0f;     // card open progress 0..1: chases `shown`, so
                            // the card keeps drawing while it fades out
    float scroll = 0.0f;    // metres the list is scrolled by
    float clearHW = 0.0f;
    int hover = -1;
    int zone = SHZONE_NONE;
    float u = 0.0f, v = 0.0f;
    int press = -1;                 // row a confirm press started on
    int pressZone = SHZONE_NONE;
    std::string pressKey;           // guards against a rebuild mid-press
    bool scrollHeld = false;        // confirm held and dragging the card
    float grabV = 0.0f;             // card-local v at grab
    float grabScroll = 0.0f;        // scroll offset at grab
    float scrollVel = 0.0f;         // drag speed while held, the released
                                    // coast's velocity after (m/s)
};

// ui strings the card draws: slots in the backend's uiStrings pull,
// appended after the library block. Keep UiStrings.java's snapshot order
// in step
enum UiShadeStr {
    UISHADE_TITLE = 0,  // the card's title
    UISHADE_CLEAR,      // the clear-all pill's label
    UISHADE_EMPTY,      // the empty list's centre message
    UISHADE_COUNT
};
