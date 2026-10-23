#pragma once

#include "item.h"
#include "../math/mat4.h"

#include <vector>

// Settings card geometry + row policy. Pure functions over plain data -
// no GL, no JNI - so the catalog, layout and hit testing run in the host
// tests.
//
// The card's local frame: u,v in -1..1 over kSetHW x kSetHH. A title band
// of kSetHeadH caps the top; under it a fixed sidebar on the left and a
// scrolling row pane on the right. Dialogs are modal: while one is up,
// hits only resolve inside its rect.

// card centre + basis on the ring's anchor cylinder, at the card's
// share of the ring's radius `dist` and the window row's elevation
void setCenter(float yaw, float pitch, float dist, const float origin[3],
               float c[3], float r[3], float up[3]);

// the content row list for a section against a snap: list-backed sections
// expand into one row per platform entry; android=false drops the rows
// only an Android host can serve
void setBuildRows(int sec, bool android, const SetSnap& snap,
                  std::vector<SetRow>* out);

// row i's band in card-local metres, scroll applied. Caller passes the
// overlay's row list; rows past the clip band still get rects so the draw
// can skip them by band test
void setRowRect(const SetOverlay& ov, int i, float* x, float* y,
                float* hw, float* hh);

// the band content scrolls inside: under the title band, down to the card
void setClipBand(float* lo, float* hi);

// true while any of row i's band shows inside the clip band
bool setRowShown(const SetOverlay& ov, int i);

// scroll range for the row list
float setScrollMax(const SetOverlay& ov);
float setClampScroll(const SetOverlay& ov, float scroll);

// wheel detents -> scroll offset, clamped
float setScrollBy(const SetOverlay& ov, float delta);

// a held press turned scroll past this much card-local v
bool setSlopPassed(float grabV, float v);

// sidebar row i's rect in card-local metres
void setSecRect(int i, float* x, float* y, float* hw, float* hh);

// the sidebar slots a host serves and the SetSec a slot maps to: the
// wayland build hides the Android-bound sections so slot and section id
// diverge there
int setSecCount(bool android);
int setSecAt(bool android, int slot);
int setSecSlot(bool android, int sec);   // -1 when the host hides sec

// sidebar column edges
float setSideL();
float setSideR();

// title row geometry
float setCloseX();
float setTitleY();

// a slider row's track x-range and the value a card-local u picks
void setSliderTrack(float* l, float* r);
float setSliderValue(float u);

// a segmented row's option cells: count and cell rect for option o
// (cell y centres on the row's own cy)
int setSegCount(int pick);
void setSegRect(int pick, int opt, float cy, float* x, float* y,
                float* hw, float* hh);

// the segmented row's live option index in the snap, -1 for no match
int setSegCurrent(int pick, const SetSnap& snap);

// the wire value a segmented option writes: theme/locale/dof strings
const char* setSegValue(int pick, int opt);

// the scan header's refresh pill, right-anchored in the row: the half
// width follows the label's rendered width, clamped so a wide
// translation clips instead of spilling over the rim. setScanPillL is
// the widest pill's left edge - the hit zone and the row's text bound
// hang off it so a narrow label never moves them
void setScanPill(float textW, float* x, float* hw);
float setScanPillL();

// the about row's hero group: the brand mark left of the two-line text
// block, centred as a unit on the row. minL pins the group's left edge
// when a long translation would overflow the row; iconX is the mark's
// centre, textX the text column's left edge, ty/dy the two line centres
// as offsets from the row's own centre
void setBrandGroup(float rowCx, float minL, float textW, float* iconX,
                   float* textX, float* ty, float* dy);

// rows whose platform side is not trusted yet draw greyed and inert -
// same convention as the quick panel's unimplemented tiles
bool setImplemented(int rid);

// the row-level form of the gate: setImplemented reads aux as a SetRowId,
// which only named rows carry; a list row's aux is an index into a snap
// vector, never a gate
bool setRowImplemented(const SetRow& r);

// a wifi net row's tap resolves to the join/manage dialog; needsPw means
// the dialog carries the password field + mini keyboard
bool setWifiNeedsPw(const SetWifi& n);

// env list rows: aux 0 = passthrough, 1 = builtin, 2+n the nth zip
const char* setEnvId(const SetSnap& snap, int aux);
bool setEnvActive(const SetSnap& snap, int aux);

// a bt row tap: unbonded pairs at once, bonded asks through DLG_BT
bool setBtNeedsConfirm(const SetBtDev& d);

// dialog geometry: the modal card rect and its slots
void setDlgRect(int dlg, bool kb, float* x, float* y, float* hw,
                float* hh);
int setDlgBtnCount(int dlg, const SetOverlay& ov);
void setDlgBtnRect(int dlg, bool kb, int slot, float* x, float* y,
                   float* hw, float* hh);
void setDlgFieldRect(float* x, float* y, float* hw, float* hh);

// the dialog mini keyboard
int setKbdCount(int page);
void setKeyRect(int page, int key, float* x, float* y, float* hw,
                float* hh);
int setKeyCode(int page, int key, bool shift);
const char* setKeyLabel(int page, int key);

// zone a card-local hit lands on: dialog zones while a dialog is up,
// then the title disc, the sidebar, then content rows
int setZoneAt(const SetOverlay& ov, float u, float v, int* idx, int* aux);

// arbitrary ray vs the card; hit is set on any card hit so the body still
// blocks the windows behind it
SetHit pickSetRay(const SetOverlay& ov, float yaw, float pitch,
                  float dist, const float origin[3], const float o[3],
                  const float d[3]);
