#pragma once

#include "item.h"
#include "../math/mat4.h"

// Quick panel geometry + tile policy. Pure functions over plain data - no
// GL, no JNI - so the catalog, layout and hit testing run in the host
// tests.
//
// The card's local frame: u,v in -1..1 over kQuickHW x kQuickHH. A title
// band of kQuickHeadH caps the top; under it sit the two slider rows
// (brightness, then volume), the large-tile row, then the small grid.

// what one tile is made of: `large` picks the wide row vs the small grid,
// `implemented` grey-out for tiles with no platform side yet (same rule
// the flutter panel used: greyed and inert), `toggle` separates flips
// from one-shot actions
struct QsSpec {
    int id;
    bool large;
    bool implemented;
    bool toggle;
};

// the catalog: indexed by tile position, ids in QsId display order
const QsSpec& quickSpec(int i);

// card centre + basis on the ring's anchor cylinder, at the grid's own
// distance and the window row's elevation
void quickCenter(float yaw, float pitch, const float origin[3],
                 float c[3], float r[3], float up[3]);

// tile i's centre and half extents in card-local metres
void quickTileRect(int i, float* x, float* y, float* hw, float* hh);

// slider row i's band centre in card-local metres
float quickSliderY(int i);

// the slider track's x range, card-local metres: the value maps onto this
void quickSliderTrack(float* l, float* r);

// value a card-local u picks on the track, clamped 0..1
float quickSliderValue(float u);

// disc centres on the title row, card-local metres
float quickCloseX();
float quickGearX();
float quickTitleY();

// a toggle tile's current bit in the snapshot
bool quickToggleOn(const QsStatus& st, int id);

// a large tile's subtitle: wifi and bluetooth carry the connected name
// when one's known, otherwise the on/off/not-connected word
enum QsSubKind { QSUB_ON, QSUB_OFF, QSUB_NOTCONN, QSUB_CUSTOM };
struct QsSub {
    int kind = QSUB_OFF;
    const char* text = nullptr;   // QSUB_CUSTOM: the ssid/device name
};
QsSub quickSubtitle(const QsStatus& st, int id);

// what a tile release resolves to, computed off the catalog so the
// enabled/inert policy is host-testable
enum QsOp {
    QOP_NONE = 0,     // greyed tile or unknown id: inert
    QOP_TOGGLE,       // flip the toggle through the backend
    QOP_RECENTER,     // re-anchor the dash, handled natively
    QOP_SETTINGS,     // open the settings app on a window
};
QsOp quickActivateOp(int id);

// zone a card-local hit lands on: the title row's discs first, then the
// slider bands, then tiles, else the card body
int quickZoneAt(float u, float v, int* idx, int* slider);

// arbitrary ray vs the card; hit is set on any card hit so the body still
// blocks the windows behind it
QsPick pickQuickRay(float yaw, float pitch, const float origin[3],
                    const float o[3], const float d[3]);
