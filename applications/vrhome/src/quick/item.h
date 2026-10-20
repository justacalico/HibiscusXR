#pragma once

#include <string>

// tile ids in display order: the large row first, then the small grid.
// The int values are the wire contract with QuickState.java's switch, so
// keep both files in step
enum QsId {
    QS_WIFI = 0,
    QS_BOUNDARY,
    QS_BLUETOOTH,
    QS_SEETHROUGH,
    QS_MIC,
    QS_RESET_VIEW,
    QS_NIGHT,
    QS_DND,
    QS_AIRPLANE,
    QS_SAVER,
    QS_REPORT,
    QS_ABOUT,
    QS_COUNT
};

// the card's two sliders, top row first; same ordering QuickState.java
// switches on
enum QsSlider {
    QSLIDER_BRIGHTNESS = 0,
    QSLIDER_VOLUME,
    QSLIDER_COUNT
};

// where a gaze hit on the card lands
enum QsZone {
    QZONE_NONE = -1,
    QZONE_TILE = 0,   // a tile: toggles flip and actions fire on release
    QZONE_BODY,       // the card itself: eats the tap
    QZONE_CLOSE,      // the disc on the title row
    QZONE_GEAR,       // the settings shortcut on the title row
    QZONE_SLIDER,     // a slider row: press and drag set the value
};

// one platform snapshot for the panel: toggles in a bitmask indexed by
// QsId (action ids never set a bit), sliders normalized 0..1, the radio
// subtitles as strings. QuickState.snap() fills it fresh on every pull.
struct QsStatus {
    unsigned toggles = 0;
    int batt = -1;          // 0..100, -1: the device has no battery
    int chg = 0;
    float volume = 0.0f;
    float brightness = 0.0f;
    std::string ssid;
    std::string btName;
};

struct QsPick {
    int idx = -1;           // tile under the ray
    int zone = QZONE_NONE;
    int slider = -1;        // QZONE_SLIDER: which row, a QsSlider
    bool hit = false;       // the ray met the card at all (blocks panels)
    float u = 0, v = 0;     // hit point in card coords, -1..1
    float t = 1e9f;         // ray distance, for pick arbitration
};

// the overlay's live state on the engine. The tiles are a fixed catalog,
// so there's no list to rebuild - hover smoothing lives in hs[] indexed
// by tile position
struct QuickPanel {
    bool shown = false;
    float openT = 0.0f;     // card open progress 0..1: chases `shown`
    QsStatus st;
    float hs[QS_COUNT];
    int hover = -1;
    int zone = QZONE_NONE;
    int slider = -1;
    float u = 0.0f, v = 0.0f;
    int press = -1;                 // tile a confirm press started on
    int pressZone = QZONE_NONE;
    int slideHeld = -1;             // slider row a held confirm is driving
    QuickPanel() { for (int i = 0; i < QS_COUNT; ++i) hs[i] = 1.0f; }
};

// ui strings the card draws: slots in the backend's uiStrings pull. The
// tile label for QsId id sits at UIQS_LABEL0 + id so a localized label
// lookup never switches. Keep UiStrings.java's snapshot order in step
enum UiQsStr {
    UIQS_TITLE = 0,
    UIQS_ON,
    UIQS_OFF,
    UIQS_NOTCONN,
    UIQS_LABEL0,            // = 4; tile labels follow in QsId order
    UIQS_COUNT = UIQS_LABEL0 + QS_COUNT
};
