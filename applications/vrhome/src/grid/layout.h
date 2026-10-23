#pragma once

#include "item.h"
#include "../math/mat4.h"

#include <vector>

// App-grid overlay geometry + hit policy. Pure functions over plain data -
// no GL, no JNI - so the layout and picking run in the host tests.
//
// The card's local frame: u,v in -1..1 over kGridHW x kGridHH. A title band
// of kGridHeadH caps the top; cells flow left-to-right in kGridCols columns
// starting under it and scroll vertically inside the rest of the card.

// card centre + basis on the ring's anchor cylinder, at the card's
// share of the ring's radius `dist` and the window row's elevation
void gridCenter(float yaw, float pitch, float dist, const float origin[3],
                float c[3], float r[3], float up[3]);

// place every cell in card-local metres (pre-scroll) and return the total
// content height in metres
float gridLayout(std::vector<GridItem>& items);

// per-frame hover smoothing: the hovered cell's hs chases kGridHoverScale,
// the rest relax to 1.0f
void tickGridHover(std::vector<GridItem>& items, int hover, float dtMs);

// the y-band cells must stay inside: clipped to under the title band.
// lo/hi are card-local metres
void gridClipBand(float* lo, float* hi);

// true while any of a cell's row can show inside the band; the shader fades
// the overlap so the margin keeps a fading row from popping. iy is the
// cell's band-space centre (pre-scaled by the open animation)
bool gridCellShown(float iy, float gs);

// scroll range for n items: 0 to the content's overflow past the card
float gridScrollMax(int n);

// clamp a scroll offset into range
float gridClampScroll(float scroll, int n);

// true once a held press's aim has moved past the tap slop in card-local
// v: below it the press still counts as a tap
bool gridSlopPassed(float grabV, float v);

// apply a wheel detent count to the scroll offset, clamped; positive
// delta reveals rows further down the list
float gridScrollBy(float scroll, float delta, int n);

// cell index at a card-local hit, or -1: the title band and the padding
// aren't cells. v is pre-scroll card coords
int gridItemAt(int n, float u, float v, float scroll);

// x of the close disc's centre, card-local metres
float gridCloseX();

// ------------------------------------------------------- groups + menus

// membership ops on the plain group list the backend persists
bool groupHas(const LibGroup& g, const std::string& pkg);
void groupToggle(LibGroup* g, const std::string& pkg);

// the next free auto name under fmt ("Group %d"): the first index the
// existing names don't already take
std::string groupNewName(const std::vector<LibGroup>& gs,
                         const char* fmt);

// the overlay's visible subset: `all` filtered through the selected group
// (-1 shows everything); dead pkgs in a group just don't match
std::vector<GridItem> gridVisible(const std::vector<GridItem>& all,
                                  const std::vector<LibGroup>& gs,
                                  int sel);

// wire format the bridge persists: one group per line, "name<TAB>pkg,pkg"
std::string groupEncode(const LibGroup& g);
bool groupDecode(const std::string& line, LibGroup* out);

// the title-row group pill: centre + half extents for a measured label
// half width
void gridGroupPill(float labelHW, float* x, float* y,
                   float* hw, float* hh);

// dropdown rows: "new group" first, "all apps", then one per group
enum GridDropRow { GD_NEW = 0, GD_ALL, GD_GROUP };
int gridDropRows(int nGroups);
int gridDropRowKind(int row);

// the dropdown hangs off the pill: rect for its row count, centred on the
// pill and clamped inside the card; cx/cy/hw/hh in card-local metres
void gridDropRect(float pillX, int rows, float* cx, float* cy,
                  float* hw, float* hh);
// the dropdown row a card-local point sits on, or -1
int gridDropRowAt(float cx, float cy, int rows, float x, float y);

// context menu rows: the pin toggle, then one per group, then "new group"
enum GridMenuRow { GM_PIN = 0, GM_GROUP, GM_NEW };
int gridMenuRows(int nGroups);
int gridMenuRowKind(int row, int nGroups);

// the menu's rect for an anchor point, clamped inside the card; cx/cy/hw/hh
// in card-local metres
void gridMenuRect(float ax, float ay, int rows, float* cx, float* cy,
                  float* hw, float* hh);
// the menu row a card-local point sits on, or -1
int gridMenuRowAt(float cx, float cy, int rows, float x, float y);

// a cell's 3-dots affordance centre, card-local metres (post-scroll y)
void gridDotsPos(const GridItem& it, float scroll, float* x, float* y);
// is a card-local point on the cell's dots
bool gridDotsHit(const GridItem& it, float scroll, float x, float y);

// zone a card-local hit lands on: open popups first, then the title row's
// close disc and group pill, then a cell or its dots, else the card body
int gridZoneAt(const GridOverlay& g, float u, float v, int* idx);

// arbitrary ray vs the card; hit is set on any card hit so the body still
// blocks the windows behind it
GridPick pickGridRay(const GridOverlay& g, float yaw, float pitch,
                     float dist, const float origin[3], const float o[3],
                     const float d[3]);
