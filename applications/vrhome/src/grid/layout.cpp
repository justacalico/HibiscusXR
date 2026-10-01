#include "layout.h"

#include "../anim/anim.h"
#include "../common/config.h"
#include "../panels/layout.h"

#include <cmath>

void gridCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]) {
    // same anchor cylinder the windows and strip ride: the card hangs at
    // its own distance so it lands in front of the windows' plane
    ringPoint(yaw, pitch, kGridDist, kPanelY, origin, c, r, up);
}

void tickGridHover(std::vector<GridItem>& items, int hover, float dtMs) {
    for (int i = 0; i < (int)items.size(); ++i)
        items[i].hs = dampMs(items[i].hs,
                             i == hover ? kGridHoverScale : 1.0f,
                             dtMs, kHoverTauMs);
}

float gridLayout(std::vector<GridItem>& items) {
    const float cellW = (kGridHW * 2.0f - kGridSidePad * 2.0f) / kGridCols;
    const float x0 = -kGridHW + kGridSidePad + cellW * 0.5f;
    const float y0 = kGridHH - kGridHeadH - kGridCellH * 0.5f;
    int i = 0;
    for (auto& it : items) {
        it.x = x0 + (i % kGridCols) * cellW;
        it.y = y0 - (i / kGridCols) * kGridCellH;
        ++i;
    }
    const int rows = ((int)items.size() + kGridCols - 1) / kGridCols;
    return rows * kGridCellH;
}

void gridClipBand(float* lo, float* hi) {
    *lo = -kGridHH;
    *hi = kGridHH - kGridHeadH;
}

float gridScrollMax(int n) {
    const int rows = (n + kGridCols - 1) / kGridCols;
    const float content = rows * kGridCellH;
    float lo, hi;
    gridClipBand(&lo, &hi);
    const float over = content - (hi - lo);
    return over > 0.0f ? over : 0.0f;
}

float gridClampScroll(float scroll, int n) {
    const float mx = gridScrollMax(n);
    return scroll < 0.0f ? 0.0f : scroll > mx ? mx : scroll;
}

int gridItemAt(int n, float u, float v, float scroll) {
    const float x = u * kGridHW, y = v * kGridHH;
    float lo, hi;
    gridClipBand(&lo, &hi);
    if (y > hi || y < lo) return -1;
    const float cellW = (kGridHW * 2.0f - kGridSidePad * 2.0f) / kGridCols;
    const int col = (int)floorf((x + kGridHW - kGridSidePad) / cellW);
    if (col < 0 || col >= kGridCols) return -1;
    const int row = (int)floorf((hi - y + scroll) / kGridCellH);
    const int idx = row * kGridCols + col;
    return idx < n ? idx : -1;
}

float gridCloseX() {
    return kGridHW - kGridSidePad - kGridCloseR;
}

int gridZoneAt(int n, float u, float v, float scroll, int* idx) {
    *idx = -1;
    const float x = u * kGridHW, y = v * kGridHH;
    // the title band: the close disc first, the rest of the band is body
    if (y > kGridHH - kGridHeadH) {
        const float dx = x - gridCloseX(),
                    dy = y - (kGridHH - kGridHeadH * 0.5f);
        const float rr = kGridCloseR + 0.012f;
        if (dx * dx + dy * dy <= rr * rr) return GZONE_CLOSE;
        return GZONE_BODY;
    }
    const int i = gridItemAt(n, u, v, scroll);
    if (i >= 0) { *idx = i; return GZONE_ITEM; }
    return GZONE_BODY;
}

GridPick pickGridRay(int n, float scroll, float yaw, float pitch,
                     const float origin[3], const float o[3],
                     const float d[3]) {
    GridPick pk;
    if (n <= 0) return pk;
    float c[3], r[3], up[3];
    gridCenter(yaw, pitch, origin, c, r, up);
    if (!rayQuad(c, r, up, origin, o, d, kGridHW, kGridHH,
                 &pk.u, &pk.v, &pk.t)) return pk;
    if (fabsf(pk.u) > 1.0f || fabsf(pk.v) > 1.0f) return pk;
    pk.hit = true;
    pk.zone = gridZoneAt(n, pk.u, pk.v, scroll, &pk.idx);
    return pk;
}
