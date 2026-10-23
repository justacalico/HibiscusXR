#pragma once

#include "item.h"
#include "../math/mat4.h"

#include <vector>

// Notification card stack policy + geometry. Pure functions over plain
// data - no GL, no JNI - so ordering, layout and hit testing all run in
// the host tests. The stack rides the dock's anchor plane, lifted above
// the bar; over a covered app the toast anchors it to the summon yaw.

// the live record set: newest postMs first, capped at kShadeMax - this is
// the shade's list, wider than the toast stack's kNotifMax
std::vector<NotifItem> buildNotifs(const std::vector<NotifItem>& in);

// the subset still inside its visible window: a card shows from postMs
// until postMs + kNotifShowMs, then the dash drops it even though the
// shade record stays live. The stack keeps at most kNotifMax cards.
// nowMs shares postMs's epoch (wall clock).
std::vector<NotifItem> visibleNotifs(const std::vector<NotifItem>& in,
                                     long long nowMs);

// half-height of a stack of `count` cards in metres
float notifStackHH(int count);

// lift above the dock bar's centre so the stack clears the strip
float notifLift(int count);

// same, clearing `clear` metres of chrome under the stack - the shelf's
// top edge while minimized windows are parked above the bar
float notifLiftAbove(int count, float clear);

// stack quad on the dock's anchor cylinder, raised `lift` along its up;
// `dist` is the ring's radius - the stack shares the strip's share of it
void notifCenter(float yaw, float pitch, float lift, float dist,
                 const float origin[3],
                 float c[3], float r[3], float up[3]);

// card i's centre offset along +up inside the stack, metres; i=0 is the
// newest card and sits at the top
float notifCardY(int i, int count);

// the close badge's centre in card-local coords, metres from card centre
void notifBadgeAt(float* bx, float* by);

// a stack-local point -> card index + zone; -1 outside the cards (gaps)
int notifAt(float u, float v, int count, int* zone);

// gaze ray vs the stack plane; u,v in stack coords, may fall outside -1..1
bool rayNotif(float yaw, float pitch, float lift, float dist,
              const float origin[3],
              const float o[3], const float d[3], int count,
              float* u, float* v, float* t);

// the card under an arbitrary ray; stack is set even between cards so the
// region blocks clicks like the dock bar does
NotifPick pickNotifRay(const std::vector<NotifItem>& items,
                       float yaw, float pitch, float lift, float dist,
                       const float origin[3], const float o[3],
                       const float d[3]);

// the card under the gaze ray
NotifPick pickNotif(const std::vector<NotifItem>& items,
                    float yaw, float pitch, float lift, float dist,
                    const Mat4& head, const float origin[3],
                    const float o[3]);

// --------------------------------------------------------- the shade
//
// The card the dock's bell opens: every live record as a row, newest
// first. The card's local frame is u,v in -1..1 over kShadeHW x kShadeHH;
// a title band of kShadeHeadH caps the top and the row list scrolls
// inside the band under it.

// card centre + basis on the ring's anchor cylinder, at the card's share
// of the ring's radius `dist` and the window row's elevation
void shadeCenter(float yaw, float pitch, float dist, const float origin[3],
                 float c[3], float r[3], float up[3]);

// the y-band rows must stay inside: clipped to under the title band.
// lo/hi are card-local metres
void shadeClipBand(float* lo, float* hi);

// row i's centre in card-local metres, scroll applied; i=0 is the newest
float shadeRowY(int i, float scroll);

// true while any of a row can show inside the band; the shader fades the
// overlap so the margin keeps a fading row from popping. ry is the row's
// band-space centre (pre-scaled by the open animation)
bool shadeRowShown(float ry, float gs);

// scroll range for n rows: 0 to the content's overflow past the band
float shadeScrollMax(int n);

// clamp a scroll offset into range
float shadeClampScroll(float scroll, int n);

// apply a wheel detent count to the scroll offset, clamped; positive
// delta reveals rows further down the list
float shadeScrollBy(float scroll, float delta, int n);

// true once a held press's aim has moved past the tap slop in card-local
// v: below it the press still counts as a tap
bool shadeSlopPassed(float grabV, float v);

// a row's dismiss badge centre in card-local x and row-local y
void shadeBadgeAt(float* bx, float* by);

// title row geometry, card-local metres
float shadeTitleY();
float shadeCloseX();

// the clear-all pill: centre + half extents for a measured label half
// width, hugging the close disc's left
void shadeClearPill(float labelHW, float* x, float* y,
                    float* hw, float* hh);

// can the clear-all pill fire: one clearable row is enough
bool shadeClearable(const std::vector<NotifItem>& items);

// zone a card-local hit lands on: the title row's discs first, then a
// row's badge or body, else the card body
int shadeZoneAt(const std::vector<NotifItem>& items, float u, float v,
                float scroll, float clearHW, int* idx);

// arbitrary ray vs the card; hit is set on any card hit so the body still
// blocks the windows behind it
ShadePick pickShadeRay(const std::vector<NotifItem>& items, float scroll,
                       float clearHW, float yaw, float pitch, float dist,
                       const float origin[3], const float o[3],
                       const float d[3]);
