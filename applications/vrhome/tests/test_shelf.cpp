#include "test.h"

#include "dock/layout.h"
#include "notif/layout.h"
#include "common/config.h"
#include "math/head.h"

static Panel mkPanel(const char* pkg, int dispId) {
    Panel p;
    p.pkg = pkg;
    p.displayId = dispId;
    return p;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testShelf() {
    // build: only minimized panels park, in panel order
    {
        std::vector<Panel> ps = {mkPanel("com.a", 1), mkPanel("com.b", 2),
                                 mkPanel("com.c", 3)};
        ps[0].minimized = true;
        ps[2].minimized = true;
        auto items = buildShelf(ps);
        CHECK(items.size() == 2);
        CHECK(items[0].panelIdx == 0 && items[0].pkg == "com.a");
        CHECK(items[1].panelIdx == 2 && items[1].pkg == "com.c");
        CHECK(buildShelf({}).empty());
        std::vector<Panel> vis = {mkPanel("com.a", 1)};
        CHECK(buildShelf(vis).empty());
    }

    // layout: the icon row centres on the pill, pad on both ends
    {
        ShelfItem a; a.pkg = "a";
        ShelfItem b; b.pkg = "b";
        std::vector<ShelfItem> items = {a, b};
        const float hw = shelfLayout(items);
        const float want = kShelfPad + kShelfIconHW * 2.0f +
                           kShelfGap * 0.5f;
        CHECK_F(hw, want, 1e-6f);
        CHECK_F(items[0].x, -hw + kShelfPad + kShelfIconHW, 1e-6f);
        CHECK_F(items[1].x, hw - kShelfPad - kShelfIconHW, 1e-6f);
        CHECK(items[0].x < 0.0f && items[1].x > 0.0f);
        // a single icon lands dead centre
        std::vector<ShelfItem> one = {a};
        const float hw1 = shelfLayout(one);
        CHECK_F(hw1, kShelfPad + kShelfIconHW, 1e-6f);
        CHECK_F(one[0].x, 0.0f, 1e-6f);
        std::vector<ShelfItem> none;
        CHECK_F(shelfLayout(none), 0.0f, 1e-6f);
    }

    // hit test: icon centres hit, the gap between them and off-pill miss
    {
        ShelfItem a; a.pkg = "a";
        ShelfItem b; b.pkg = "b";
        std::vector<ShelfItem> items = {a, b};
        const float hw = shelfLayout(items);
        CHECK(shelfItemAt(items, hw, items[0].x / hw, 0.0f) == 0);
        CHECK(shelfItemAt(items, hw, items[1].x / hw, 0.0f) == 1);
        CHECK(shelfItemAt(items, hw, 0.0f, 0.0f) == -1);   // gap midpoint
        CHECK(shelfItemAt(items, hw, 0.0f, 1.2f) == -1);
        CHECK(shelfItemAt(items, hw, 1.2f, 0.0f) == -1);
        CHECK(shelfItemAt(items, 0.0f, 0.0f, 0.0f) == -1);
    }

    // lift: the pill's bottom edge clears the bar's top, and its own top
    // edge is what the notif stack rises above
    {
        CHECK(shelfLift() - kShelfHH > kDockBarH * 0.5f);
        CHECK_F(shelfTop(), shelfLift() + kShelfHH, 1e-6f);
        CHECK_F(notifLift(2), notifLiftAbove(2, kDockBarH * 0.5f), 1e-6f);
        CHECK(notifLiftAbove(2, shelfTop()) > notifLift(2));
    }

    // geometry: the pill rides the dock plane lifted along its up; a ray
    // at its centre hits u=v=0, a ray at the bar misses it entirely
    {
        float c[3], r[3], up[3];
        shelfCenter(0.0f, -0.55f, o0, c, r, up);
        float dc[3], dr[3], dup[3];
        dockCenter(0.0f, -0.55f, o0, dc, dr, dup);
        CHECK_F(c[0], dc[0] + dup[0] * shelfLift(), 1e-6f);
        CHECK_F(c[1], dc[1] + dup[1] * shelfLift(), 1e-6f);
        CHECK_F(c[2], dc[2] + dup[2] * shelfLift(), 1e-6f);
        CHECK_F(r[0], dr[0], 1e-6f);
        CHECK_F(up[1], dup[1], 1e-6f);

        float d[3] = {c[0], c[1], c[2]};
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= dl; d[1] /= dl; d[2] /= dl;
        float u, v, t;
        CHECK(rayShelf(0.0f, -0.55f, o0, o0, d, 0.2f, &u, &v, &t));
        CHECK_F(u, 0.0f, 1e-4f);
        CHECK_F(v, 0.0f, 1e-4f);
        CHECK_F(t, dl, 1e-4f);
    }

    // pick: gaze at the pill centre picks its icon; gaze at the bar below
    // or off to the side misses; an empty shelf never reports a hit
    {
        ShelfItem a; a.pkg = "a";
        std::vector<ShelfItem> items = {a};
        const float hw = shelfLayout(items);
        float c[3], r[3], up[3];
        shelfCenter(0.0f, -0.55f, o0, c, r, up);
        Mat4 aim = identity();
        float d[3] = {c[0], c[1], c[2]};
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        aim.m[2] = -d[0] / dl; aim.m[6] = -d[1] / dl; aim.m[10] = -d[2] / dl;
        ShelfPick pk = pickShelf(items, hw, 0.0f, -0.55f, aim, o0, o0);
        CHECK(pk.hit && pk.idx == 0);
        float dc[3], dr[3], dup[3];
        dockCenter(0.0f, -0.55f, o0, dc, dr, dup);
        const float bdl = sqrtf(dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);
        aim.m[2] = -dc[0] / bdl; aim.m[6] = -dc[1] / bdl;
        aim.m[10] = -dc[2] / bdl;
        pk = pickShelf(items, hw, 0.0f, -0.55f, aim, o0, o0);
        CHECK(!pk.hit);
        std::vector<ShelfItem> none;
        pk = pickShelf(none, 0.0f, 0.0f, -0.55f, aim, o0, o0);
        CHECK(!pk.hit && pk.idx == -1);
    }
}
