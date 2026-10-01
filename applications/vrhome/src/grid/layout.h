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

// card centre + basis on the ring's anchor cylinder, at the grid's own
// distance and the window row's elevation
void gridCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]);

// place every cell in card-local metres (pre-scroll) and return the total
// content height in metres
float gridLayout(std::vector<GridItem>& items);

// the y-band cells must stay inside: clipped to under the title band.
// lo/hi are card-local metres
void gridClipBand(float* lo, float* hi);

// scroll range for n items: 0 to the content's overflow past the card
float gridScrollMax(int n);

// clamp a scroll offset into range
float gridClampScroll(float scroll, int n);

// cell index at a card-local hit, or -1: the title band and the padding
// aren't cells. v is pre-scroll card coords
int gridItemAt(int n, float u, float v, float scroll);

// x of the close disc's centre, card-local metres
float gridCloseX();

// zone a card-local hit lands on: close disc on the title row, an item,
// or the card body
int gridZoneAt(int n, float u, float v, float scroll, int* idx);

// arbitrary ray vs the card; hit is set on any card hit so the body still
// blocks the windows behind it
GridPick pickGridRay(int n, float scroll, float yaw, float pitch,
                     const float origin[3], const float o[3],
                     const float d[3]);
