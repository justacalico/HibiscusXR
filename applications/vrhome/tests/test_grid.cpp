#include "test.h"

#include "grid/dashapps.h"
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

static GridOverlay mkOverlay(int n, float scroll = 0.0f) {
    GridOverlay g;
    g.items = mkItems(n);
    g.all = g.items;
    g.scroll = scroll;
    gridLayout(g.items);
    return g;
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

    // the draw cull: a cell shows while any of its row can land inside the
    // band (plus the fade margin); a row scrolled fully past either edge is
    // skipped, at any open scale
    {
        float lo, hi;
        gridClipBand(&lo, &hi);
        const float half = kGridCellH * 0.5f;
        CHECK(gridCellShown(0.0f, 1.0f));
        CHECK(gridCellShown(hi, 1.0f));
        CHECK(gridCellShown(lo, 1.0f));
        CHECK(gridCellShown(hi + half + 0.01f, 1.0f));
        CHECK(gridCellShown(lo - half - 0.01f, 1.0f));
        CHECK(!gridCellShown(hi + half + 0.03f, 1.0f));
        CHECK(!gridCellShown(lo - half - 0.03f, 1.0f));
        // iy arrives already scaled by the open animation: a mid-open cell
        // at the scaled band edge still shows
        CHECK(gridCellShown(hi * 0.5f, 0.5f));
        CHECK(!gridCellShown((hi + half) * 0.5f + 0.03f, 0.5f));
    }

    // tap slop: a held press counts as a scroll drag once the aim's
    // card-local v has travelled kGridDragSlop metres, either way
    {
        const float slopV = kGridDragSlop / kGridHH;
        CHECK(!gridSlopPassed(0.0f, 0.0f));
        CHECK(!gridSlopPassed(0.0f, slopV * 0.9f));
        CHECK(!gridSlopPassed(0.0f, -slopV * 0.9f));
        CHECK(gridSlopPassed(0.0f, slopV * 1.1f));
        CHECK(gridSlopPassed(0.0f, -slopV * 1.1f));
        CHECK(gridSlopPassed(0.3f, 0.3f + slopV * 1.1f));
    }

    // wheel scroll: a detent moves kGridWheelStep metres, positive going
    // further down the list, and the result clamps to the content
    {
        const int n = kGridCols * 6;
        CHECK_F(gridScrollBy(0.0f, 1.0f, n), kGridWheelStep, 1e-6f);
        CHECK_F(gridScrollBy(kGridWheelStep, -0.5f, n),
                kGridWheelStep * 0.5f, 1e-6f);
        CHECK_F(gridScrollBy(0.0f, -1.0f, n), 0.0f, 1e-6f);
        CHECK_F(gridScrollBy(0.0f, 99.0f, n), gridScrollMax(n), 1e-6f);
        // a short list takes the deltas but the offset never leaves 0
        CHECK_F(gridScrollBy(0.0f, 1.0f, kGridCols), 0.0f, 1e-6f);
    }

    // hit test: a cell centre resolves to its index, padding and past-the-
    // list cells are body, the title row is body except the close disc
    {
        auto ov = mkOverlay(kGridCols * 2);
        const float u = ov.items[3].x / kGridHW,
                    v = ov.items[3].y / kGridHH;
        int idx = -1;
        CHECK(gridZoneAt(ov, u, v, &idx) == GZONE_ITEM);
        CHECK(idx == 3);
        // the far right of a partially-filled row is card body
        const float xv = (kGridHW - kGridSidePad * 0.5f) / kGridHW;
        CHECK(gridZoneAt(ov, xv, v, &idx) == GZONE_ITEM ||
              gridZoneAt(ov, xv, v, &idx) == GZONE_BODY);
        // scrolled one row: row 1 sits on the spot row 0 had
        const float u0 = ov.items[0].x / kGridHW,
                    v0 = ov.items[0].y / kGridHH;
        ov.scroll = kGridCellH;
        CHECK(gridZoneAt(ov, u0, v0, &idx) == GZONE_ITEM);
        CHECK(idx == kGridCols);
        // the title row and its close disc
        const float cv = (kGridHH - kGridHeadH * 0.5f) / kGridHH;
        CHECK(gridZoneAt(ov, 0.0f, cv, &idx) == GZONE_BODY);
        CHECK(gridZoneAt(ov, gridCloseX() / kGridHW, cv, &idx)
              == GZONE_CLOSE);
        // below the card's last cell: body
        const float bv = (-kGridHH + 0.02f) / kGridHH;
        CHECK(gridZoneAt(ov, 0.0f, bv, &idx) == GZONE_BODY);
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
        auto ov = mkOverlay(3);
        GridPick pk = pickGridRay(ov, 0.0f, 0.0f, o0, o0, d);
        CHECK(pk.hit);
        float c2[3], r2[3], u2[3];
        gridCenter(0.0f, 0.0f, o0, c2, r2, u2);
        const float off[3] = {c2[0] + r2[0] * (kGridHW + 0.4f), c2[1],
                              c2[2] + r2[2] * (kGridHW + 0.4f)};
        float d2[3] = {off[0], off[1], off[2]};
        const float dl2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]);
        d2[0] /= dl2; d2[1] /= dl2; d2[2] /= dl2;
        pk = pickGridRay(ov, 0.0f, 0.0f, o0, o0, d2);
        CHECK(!pk.hit);
        // a ray onto the first cell picks the item
        float d3[3] = {c[0] + r[0]*ov.items[0].x + up[0]*ov.items[0].y,
                       c[1] + r[1]*ov.items[0].x + up[1]*ov.items[0].y,
                       c[2] + r[2]*ov.items[0].x + up[2]*ov.items[0].y};
        const float dl3 = sqrtf(d3[0]*d3[0] + d3[1]*d3[1] + d3[2]*d3[2]);
        d3[0] /= dl3; d3[1] /= dl3; d3[2] /= dl3;
        pk = pickGridRay(ov, 0.0f, 0.0f, o0, o0, d3);
        CHECK(pk.hit && pk.zone == GZONE_ITEM && pk.idx == 0);
    }

    // groups: membership toggles, the filter keeps the group's pkgs only,
    // and a bad selection falls back to the whole list
    {
        LibGroup g;
        g.name = "Group 1";
        CHECK(!groupHas(g, "com.x.app0"));
        groupToggle(&g, "com.x.app0");
        groupToggle(&g, "com.x.app2");
        CHECK(groupHas(g, "com.x.app0") && groupHas(g, "com.x.app2"));
        groupToggle(&g, "com.x.app0");
        CHECK(!groupHas(g, "com.x.app0") && groupHas(g, "com.x.app2"));
        std::vector<LibGroup> gs = {g};
        auto all = mkItems(6);
        auto vis = gridVisible(all, gs, 0);
        CHECK(vis.size() == 1 && vis[0].pkg == "com.x.app2");
        CHECK(gridVisible(all, gs, -1).size() == 6);
        CHECK(gridVisible(all, gs, 7).size() == 6);
    }

    // auto names take the first free index under the fmt
    {
        CHECK(groupNewName({}, "Group %d") == "Group 1");
        LibGroup a; a.name = "Group 1";
        LibGroup b; b.name = "Group 3";
        std::vector<LibGroup> gs = {a, b};
        CHECK(groupNewName(gs, "Group %d") == "Group 2");
        // a fmt with no %d collides forever except when the list is empty
        CHECK(groupNewName({}, "Group") == "Group");
    }

    // the wire encoding round-trips; a line with no tab or name is junk
    {
        LibGroup g;
        g.name = "Games";
        g.pkgs = {"com.x.a", "com.x.b"};
        CHECK(groupEncode(g) == "Games\tcom.x.a,com.x.b");
        LibGroup back;
        CHECK(groupDecode("Games\tcom.x.a,com.x.b", &back));
        CHECK(back.name == "Games" && back.pkgs.size() == 2 &&
              back.pkgs[1] == "com.x.b");
        CHECK(!groupDecode("Games", &back));
        CHECK(!groupDecode("\tcom.x.a", &back));
        CHECK(groupDecode("Empty\t", &back) && back.pkgs.empty());
    }

    // the dots affordance sits at the icon's top-right and tracks scroll
    {
        auto ov = mkOverlay(kGridCols * 2);
        const GridItem& it = ov.items[2];
        float dx, dy;
        gridDotsPos(it, 0.0f, &dx, &dy);
        CHECK_F(dx, it.x + kGridDotsOff, 1e-6f);
        CHECK_F(dy, it.y + kGridDotsOff, 1e-6f);
        CHECK(gridDotsHit(it, 0.0f, dx, dy));
        CHECK(!gridDotsHit(it, 0.0f, it.x, it.y));
        // its zone resolves through gridZoneAt like a cell corner would
        int idx = -1;
        const int zone = gridZoneAt(ov, dx / kGridHW, dy / kGridHH, &idx);
        CHECK(zone == GZONE_DOTS && idx == 2);
        // a row-1 cell scrolled up into the band keeps its dots live
        const int lower = kGridCols + 2;
        ov.scroll = kGridCellH;
        gridDotsPos(ov.items[lower], ov.scroll, &dx, &dy);
        CHECK(gridZoneAt(ov, dx / kGridHW, dy / kGridHH, &idx) ==
              GZONE_DOTS && idx == lower);
    }

    // the title-row pill parks left of the close disc and the dropdown
    // hangs under it inside the card
    {
        float px, py, phw, phh;
        gridGroupPill(0.05f, &px, &py, &phw, &phh);
        CHECK(px + phw < gridCloseX());
        CHECK(py + phh < kGridHH && py - phh > kGridHH - kGridHeadH);
        auto ov = mkOverlay(4);
        ov.pillHW = 0.05f;
        int idx = -1;
        CHECK(gridZoneAt(ov, px / kGridHW, py / kGridHH, &idx) ==
              GZONE_GROUPSEL);
        // the dropdown: new-group row on top, then all apps, then groups
        const int rows = gridDropRows(2);
        CHECK(rows == 4);
        CHECK(gridDropRowKind(0) == GD_NEW);
        CHECK(gridDropRowKind(1) == GD_ALL);
        CHECK(gridDropRowKind(2) == GD_GROUP &&
              gridDropRowKind(3) == GD_GROUP);
        float cx, cy, dhw, dhh;
        gridDropRect(px, rows, &cx, &cy, &dhw, &dhh);
        CHECK(cx - dhw > -kGridHW && cx + dhw < kGridHW);
        CHECK(cy - dhh > -kGridHH);
        CHECK(cy + dhh < kGridHH - kGridHeadH);
        // row 0 sits at the rect's top edge
        const float ry0 = cy + dhh - kGridMenuPad -
                          kGridMenuRowH * 0.5f;
        CHECK(gridDropRowAt(cx, cy, rows, cx, ry0) == 0);
        CHECK(gridDropRowAt(cx, cy, rows, cx,
                            ry0 - kGridMenuRowH * 2.5f) == 2);
        CHECK(gridDropRowAt(cx, cy, rows, cx, cy + dhh + 0.05f) == -1);
        // and it wins the hit while open
        ov.grpOpen = true;
        CHECK(gridZoneAt(ov, cx / kGridHW, ry0 / kGridHH, &idx) ==
              GZONE_GROW && idx == 0);
    }

    // the cell menu: pin row, one row per group, the create row last; its
    // rect clamps inside the card and rows hit-test top-down
    {
        CHECK(gridMenuRows(0) == 2);
        CHECK(gridMenuRows(3) == 5);
        CHECK(gridMenuRowKind(0, 3) == GM_PIN);
        CHECK(gridMenuRowKind(1, 3) == GM_GROUP &&
              gridMenuRowKind(3, 3) == GM_GROUP);
        CHECK(gridMenuRowKind(4, 3) == GM_NEW);
        float cx, cy, mhw, mhh;
        // anchored off the card's right edge: the clamp pulls it back in
        gridMenuRect(kGridHW - 0.01f, kGridHH, 5, &cx, &cy, &mhw, &mhh);
        CHECK(cx + mhw <= kGridHW - kGridMenuPad + 1e-5f);
        CHECK(cy - mhh >= -kGridHH + kGridMenuPad - 1e-5f);
        CHECK(cy + mhh <= kGridHH - kGridMenuPad + 1e-5f);
        const float ry0 = cy + mhh - kGridMenuPad -
                          kGridMenuRowH * 0.5f;
        CHECK(gridMenuRowAt(cx, cy, 5, cx, ry0) == 0);
        CHECK(gridMenuRowAt(cx, cy, 5, cx,
                            ry0 - kGridMenuRowH * 4.0f) == 4);
        CHECK(gridMenuRowAt(cx, cy, 5, cx, cy - mhh - 0.02f) == -1);
        // an open menu steals its rows' hits from the cells under it
        auto ov = mkOverlay(kGridCols * 2);
        ov.menuPkg = "com.x.app0";
        ov.menuX = 0.0f;
        ov.menuY = 0.0f;
        gridMenuRect(0.0f, 0.0f, gridMenuRows(0), &cx, &cy, &mhw, &mhh);
        const float mry = cy + mhh - kGridMenuPad -
                          kGridMenuRowH * 0.5f;
        int idx = -1;
        CHECK(gridZoneAt(ov, cx / kGridHW, mry / kGridHH, &idx) ==
              GZONE_MENU && idx == 0);
    }

    // dash entries: the registry sits indexed by DashPanel, merges ahead
    // of the real apps, and resolves back from its pseudo pkgs
    {
        for (int p = 0; p < DP_COUNT; ++p)
            CHECK(dashLibApps()[p].panel == p);
        CHECK(dashPanelForPkg("dash.settings") == DP_SETTINGS);
        CHECK(dashPanelForPkg("dash.quick") == DP_QUICK);
        CHECK(dashPanelForPkg("com.x.app0") == -1);
        CHECK(dashPanelForPkg("") == -1);
        auto apps = mkItems(2);
        const size_t n0 = apps.size();
        dashLibMerge(&apps);
        int baked = 0;
        for (int p = 0; p < DP_COUNT; ++p)
            if (dashLibApps()[p].canShowInLibrary) ++baked;
        CHECK(apps.size() == n0 + baked);
        for (int i = 0; i < baked; ++i) {
            CHECK(apps[i].dashPanel == i);
            CHECK(apps[i].pkg == dashLibApps()[i].pkg);
        }
        CHECK(apps[baked].pkg == "com.x.app0");
        CHECK(apps[baked].dashPanel == -1);
    }

    // a dash cell's dots spot zones as the item itself: dash panels
    // can't pin or group, so the affordance never arms
    {
        auto ov = mkOverlay(4);
        ov.items[1].dashPanel = DP_SETTINGS;
        ov.items[1].pkg = "dash.settings";
        float dx, dy;
        gridDotsPos(ov.items[1], 0.0f, &dx, &dy);
        int idx = -1;
        CHECK(gridZoneAt(ov, dx / kGridHW, dy / kGridHH, &idx) ==
              GZONE_ITEM && idx == 1);
        // a real app on the same spot still gets its dots
        gridDotsPos(ov.items[2], 0.0f, &dx, &dy);
        CHECK(gridZoneAt(ov, dx / kGridHW, dy / kGridHH, &idx) ==
              GZONE_DOTS && idx == 2);
    }
}
