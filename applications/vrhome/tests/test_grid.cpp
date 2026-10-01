#include "test.h"

#include "grid/layout.h"
#include "common/config.h"
#include "math/head.h"

#include <vector>
#include <cmath>

static const float o0[3] = {0.0f, 0.0f, 0.0f};

static std::vector<GridItem> mkItems(int n) {
    std::vector<GridItem> items;
    for (int i = 0; i < n; ++i) {
        GridItem it;
        it.pkg = "com.x.app" + std::to_string(i);
        it.label = it.pkg;
        items.push_back(it);
    }
    return items;
}

void testGrid() {
    // cells fill left-to-right under the title band: the first row's left
    // and right ends land inside the card, the second row drops one cell
    {
        auto items = mkItems(kGridCols * 2);
        const float ch = gridLayout(items);
        CHECK_F(ch, 2.0f * kGridCellH, 1e-6f);
        const float cellW =
            (kGridHW * 2.0f - kGridSidePad * 2.0f) / kGridCols;
        CHECK_F(items[0].x, -kGridHW + kGridSidePad + cellW * 0.5f, 1e-6f);
        CHECK_F(items[0].y, kGridHH - kGridHeadH - kGridCellH * 0.5f,
                1e-6f);
        CHECK_F(items[kGridCols].y, items[0].y - kGridCellH, 1e-6f);
        CHECK_F(items[kGridCols].x, items[0].x, 1e-6f);
        CHECK(items[kGridCols - 1].x < kGridHW);
    }

    // scroll range: a card's worth of content can't scroll, a taller one
    // stops exactly at its overflow
    {
        const int n = kGridCols * 6;
        const float mx = gridScrollMax(n);
        CHECK(mx > 0.0f);
        float lo, hi;
        gridClipBand(&lo, &hi);
        const int rows = n / kGridCols;
        CHECK_F(mx, rows * kGridCellH - (hi - lo), 1e-6f);
        CHECK_F(gridScrollMax(0), 0.0f, 1e-6f);
        CHECK_F(gridClampScroll(-1.0f, n), 0.0f, 1e-6f);
        CHECK_F(gridClampScroll(9.0f, n), mx, 1e-6f);
        CHECK_F(gridClampScroll(mx * 0.5f, n), mx * 0.5f, 1e-6f);
    }

    // hit test: a cell centre resolves to its index, padding and past-the-
    // list cells are body, the title row is body except the close disc
    {
        auto items = mkItems(kGridCols * 2);
        gridLayout(items);
        const int n = (int)items.size();
        const float u = items[3].x / kGridHW, v = items[3].y / kGridHH;
        int idx = -1;
        CHECK(gridZoneAt(n, u, v, 0.0f, &idx) == GZONE_ITEM);
        CHECK(idx == 3);
        // the far right of a partially-filled row is card body
        const float xv = (kGridHW - kGridSidePad * 0.5f) / kGridHW;
        CHECK(gridZoneAt(n, xv, v, 0.0f, &idx) == GZONE_ITEM ||
              gridZoneAt(n, xv, v, 0.0f, &idx) == GZONE_BODY);
        // scrolled one row: row 1 sits on the spot row 0 had
        const float u0 = items[0].x / kGridHW,
                    v0 = items[0].y / kGridHH;
        CHECK(gridZoneAt(n, u0, v0, kGridCellH, &idx) == GZONE_ITEM);
        CHECK(idx == kGridCols);
        // the title row and its close disc
        const float cv = (kGridHH - kGridHeadH * 0.5f) / kGridHH;
        CHECK(gridZoneAt(n, 0.0f, cv, 0.0f, &idx) == GZONE_BODY);
        CHECK(gridZoneAt(n, gridCloseX() / kGridHW, cv, 0.0f, &idx)
              == GZONE_CLOSE);
        // below the card's last cell: body
        const float bv = (-kGridHH + 0.02f) / kGridHH;
        CHECK(gridZoneAt(n, 0.0f, bv, 0.0f, &idx) == GZONE_BODY);
    }

    // the card sits on the dash's anchor cylinder at its own distance:
    // nearer than the windows, just behind the dock strip
    {
        float c[3], r[3], up[3];
        gridCenter(0.0f, 0.0f, o0, c, r, up);
        CHECK_F(c[2], -kGridDist, 1e-6f);
        CHECK(kGridDist < kPanelDist && kGridDist > kDockDist);
        float d[3] = {c[0], c[1], c[2]};
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= dl; d[1] /= dl; d[2] /= dl;
        // a ray at the card centre hits it; aim off the card misses
        auto items = mkItems(3);
        gridLayout(items);
        GridPick pk = pickGridRay((int)items.size(), 0.0f, 0.0f, 0.0f,
                                  o0, o0, d);
        CHECK(pk.hit);
        float c2[3], r2[3], u2[3];
        gridCenter(0.0f, 0.0f, o0, c2, r2, u2);
        const float off[3] = {c2[0] + r2[0] * (kGridHW + 0.4f), c2[1],
                              c2[2] + r2[2] * (kGridHW + 0.4f)};
        float d2[3] = {off[0], off[1], off[2]};
        const float dl2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]);
        d2[0] /= dl2; d2[1] /= dl2; d2[2] /= dl2;
        pk = pickGridRay((int)items.size(), 0.0f, 0.0f, 0.0f, o0, o0, d2);
        CHECK(!pk.hit);
        // a ray onto the first cell picks the item
        float d3[3] = {c[0] + r[0]*items[0].x + up[0]*items[0].y,
                       c[1] + r[1]*items[0].x + up[1]*items[0].y,
                       c[2] + r[2]*items[0].x + up[2]*items[0].y};
        const float dl3 = sqrtf(d3[0]*d3[0] + d3[1]*d3[1] + d3[2]*d3[2]);
        d3[0] /= dl3; d3[1] /= dl3; d3[2] /= dl3;
        pk = pickGridRay((int)items.size(), 0.0f, 0.0f, 0.0f, o0, o0, d3);
        CHECK(pk.hit && pk.zone == GZONE_ITEM && pk.idx == 0);
    }
}
