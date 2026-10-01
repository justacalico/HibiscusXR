#include "test.h"

#include "anim/anim.h"
#include "common/config.h"
#include "dock/layout.h"
#include "grid/layout.h"
#include "math/head.h"
#include "panels/layout.h"

static Panel mkPanel(const char* pkg) {
    Panel p;
    p.pkg = pkg;
    return p;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testAnim() {
    // easings pin the endpoints and stay monotonic through the middle
    {
        CHECK_F(clamp01(-0.5f), 0.0f, 1e-6f);
        CHECK_F(clamp01(1.5f), 1.0f, 1e-6f);
        CHECK_F(easeOutCubic(0.0f), 0.0f, 1e-6f);
        CHECK_F(easeOutCubic(1.0f), 1.0f, 1e-6f);
        CHECK_F(easeInOutCubic(0.0f), 0.0f, 1e-6f);
        CHECK_F(easeInOutCubic(0.5f), 0.5f, 1e-6f);
        CHECK_F(easeInOutCubic(1.0f), 1.0f, 1e-6f);
        // out-cubic is past halfway at the midpoint; in-out is symmetric
        CHECK(easeOutCubic(0.5f) > 0.5f);
        CHECK_F(easeInOutCubic(0.25f), 1.0f - easeInOutCubic(0.75f), 1e-5f);
        float prev = -1.0f;
        for (int i = 0; i <= 20; ++i) {
            const float v = easeInOutCubic(i / 20.0f);
            CHECK(v >= prev);
            prev = v;
        }
    }

    // progT: no stamp or no duration reads done; inside the window it runs
    {
        CHECK_F(progT(0, 5000, kSpawnMs), 1.0f, 1e-6f);
        CHECK_F(progT(1000, 500, kSpawnMs), 0.0f, 1e-6f);
        CHECK_F(progT(1000, 1000 + (long long)(kSpawnMs * 0.5f), kSpawnMs),
                0.5f, 1e-4f);
        CHECK_F(progT(1000, 1000 + (long long)kSpawnMs + 50, kSpawnMs),
                1.0f, 1e-6f);
    }

    // stepT: duration-clamped both ways, safe on a zero duration
    {
        CHECK_F(stepT(0.0f, true, 50.0f, 100.0f), 0.5f, 1e-5f);
        CHECK_F(stepT(0.8f, true, 50.0f, 100.0f), 1.0f, 1e-6f);
        CHECK_F(stepT(0.8f, false, 50.0f, 100.0f), 0.3f, 1e-5f);
        CHECK_F(stepT(0.1f, false, 50.0f, 100.0f), 0.0f, 1e-6f);
        CHECK_F(stepT(0.3f, true, 16.0f, 0.0f), 1.0f, 1e-6f);
        CHECK_F(stepT(0.3f, false, 16.0f, 0.0f), 0.0f, 1e-6f);
    }

    // dampMs: still frame is a no-op, value chases the target and snaps
    // the last epsilon instead of asymptoting forever
    {
        CHECK_F(dampMs(0.5f, 1.0f, 0.0f, 60.0f), 0.5f, 1e-6f);
        CHECK_F(dampMs(1.0f, kHoverScale, 0.0f, 60.0f), 1.0f, 1e-6f);
        const float h = dampMs(1.0f, kHoverScale, 16.0f, kHoverTauMs);
        CHECK(h > 1.0f && h < kHoverScale);
        float v = 1.0f;
        for (int i = 0; i < 60; ++i) v = dampMs(v, kHoverScale, 16.0f,
                                              kHoverTauMs);
        CHECK_F(v, kHoverScale, 1e-6f);   // ~1s of frames lands on target
        v = dampMs(v, 1.0f, 16.0f, kHoverTauMs);
        CHECK(v < kHoverScale && v > 1.0f);
    }

    // lerp3 endpoints and midpoint
    {
        const float a[3] = {1.0f, 2.0f, 3.0f}, b[3] = {3.0f, 0.0f, -1.0f};
        float o[3];
        lerp3(a, b, 0.5f, o);
        CHECK_F(o[0], 2.0f, 1e-6f);
        CHECK_F(o[1], 1.0f, 1e-6f);
        CHECK_F(o[2], 1.0f, 1e-6f);
        lerp3(a, b, 0.0f, o);
        CHECK_F(o[0], a[0], 1e-6f);
        lerp3(a, b, 1.0f, o);
        CHECK_F(o[2], b[2], 1e-6f);
    }

    // hoverP maps the smoothed scale to a 0..1 for fade-in extras
    {
        CHECK_F(hoverP(1.0f, kHoverScale), 0.0f, 1e-6f);
        CHECK_F(hoverP(kHoverScale, kHoverScale), 1.0f, 1e-6f);
        CHECK_F(hoverP(1.0f + (kHoverScale - 1.0f) * 0.5f, kHoverScale),
                0.5f, 1e-4f);
        CHECK_F(hoverP(kGridHoverScale, kGridHoverScale), 1.0f, 1e-6f);
    }

    // spawn-in: window grows in from kSpawnScale0, alpha runs ahead of the
    // scale so the surface reads before the growth finishes
    {
        CHECK_F(spawnScale(0.0f), kSpawnScale0, 1e-6f);
        CHECK_F(spawnScale(1.0f), 1.0f, 1e-6f);
        CHECK_F(spawnAlpha(0.0f), 0.0f, 1e-6f);
        CHECK_F(spawnAlpha(1.0f), 1.0f, 1e-6f);
        CHECK(spawnAlpha(0.5f) > 0.95f);
        CHECK(spawnScale(0.5f) > kSpawnScale0 && spawnScale(0.5f) < 1.0f);
    }

    // park flight: eased both ways, shrinks to nothing, fades only in the
    // last stretch so the window stays readable most of the trip
    {
        CHECK_F(minScale(0.0f), 1.0f, 1e-6f);
        CHECK_F(minScale(1.0f), 0.0f, 1e-6f);
        CHECK_F(minAlpha(0.0f), 1.0f, 1e-6f);
        CHECK_F(minAlpha(0.5f), 1.0f, 1e-4f);   // mid-flight still opaque
        CHECK_F(minAlpha(1.0f), 0.0f, 1e-6f);
        CHECK(minScale(0.5f) < 0.6f && minScale(0.5f) > 0.4f);
    }

    // summon: the strip's remaining drop drains to zero while alpha lands
    {
        CHECK_F(dashDrop(0.0f), kDashDrop, 1e-6f);
        CHECK_F(dashDrop(1.0f), 0.0f, 1e-6f);
        CHECK_F(dashAlpha(0.0f), 0.0f, 1e-6f);
        CHECK_F(dashAlpha(1.0f), 1.0f, 1e-6f);
        CHECK(dashDrop(0.5f) < kDashDrop && dashDrop(0.5f) > 0.0f);
    }

    // tickPanels: minT chases the flag over kMinMs either direction, and a
    // restore mid-flight just reverses
    {
        std::vector<Panel> ps = {mkPanel("a"), mkPanel("b")};
        ps[0].minimized = true;
        tickPanels(ps, kMinMs * 0.5f);
        CHECK_F(ps[0].minT, 0.5f, 1e-4f);
        CHECK_F(ps[1].minT, 0.0f, 1e-6f);
        tickPanels(ps, kMinMs);
        CHECK_F(ps[0].minT, 1.0f, 1e-6f);
        ps[0].minimized = false;
        tickPanels(ps, kMinMs * 0.25f);
        CHECK_F(ps[0].minT, 0.75f, 1e-4f);
        tickPanels(ps, kMinMs);
        CHECK_F(ps[0].minT, 0.0f, 1e-6f);
    }

    // a window mid-flight isn't pickable: parked or still shrinking it
    // eats no aim; once it lands it picks again
    {
        Mat4 I = identity();
        std::vector<Panel> ps = {mkPanel("a")};
        ps[0].yaw = 0.0f;
        Pick pk = pickPanel(ps, I, o0, o0);
        CHECK(pk.idx == 0);
        ps[0].minT = 0.4f;                    // mid-restore, flag dropped
        pk = pickPanel(ps, I, o0, o0);
        CHECK(pk.idx == -1);
        ps[0].minimized = true; ps[0].minT = 0.2f;   // shrinking out
        pk = pickPanel(ps, I, o0, o0);
        CHECK(pk.idx == -1);
    }

    // buildShelf parks a restoring panel too, so the flight grows out of a
    // slot that doesn't vanish under it
    {
        std::vector<Panel> ps = {mkPanel("a"), mkPanel("b")};
        ps[1].minimized = false;
        ps[1].minT = 0.7f;
        auto items = buildShelf(ps);
        CHECK(items.size() == 1);
        CHECK(items[0].panelIdx == 1 && items[0].pkg == "b");
        ps[1].minT = 0.0f;
        CHECK(buildShelf(ps).empty());
    }

    // shelfXFor hands the flight its target's pill-local x
    {
        std::vector<Panel> ps = {mkPanel("a"), mkPanel("b")};
        ps[0].minimized = true;
        ps[1].minimized = true;
        auto items = buildShelf(ps);
        shelfLayout(items);
        float x = 0.0f;
        CHECK(shelfXFor(items, 0, &x));
        CHECK_F(x, items[0].x, 1e-6f);
        CHECK(shelfXFor(items, 1, &x));
        CHECK_F(x, items[1].x, 1e-6f);
        CHECK(!shelfXFor(items, 5, &x));
        std::vector<ShelfItem> none;
        CHECK(!shelfXFor(none, 0, &x));
    }

    // carryHover: the smoothed scale survives the per-frame rebuild by
    // identity, not index - a reshuffled list keeps its values
    {
        std::vector<DockItem> prev;
        DockItem pa; pa.kind = DK_PIN; pa.pkg = "a"; pa.hs = 1.09f;
        DockItem pb; pb.kind = DK_RUN; pb.pkg = "b"; pb.hs = 1.12f;
        prev = {pa, pb};
        std::vector<DockItem> next;
        DockItem nb; nb.kind = DK_RUN; nb.pkg = "b";   // moved up front
        DockItem nc; nc.kind = DK_PIN; nc.pkg = "c";   // brand new
        next = {nb, nc};
        carryHover(next, prev);
        CHECK_F(next[0].hs, 1.12f, 1e-6f);
        CHECK_F(next[1].hs, 1.0f, 1e-6f);
        // same pkg but a different kind is a different slot
        DockItem pd; pd.kind = DK_PIN; pd.pkg = "b";
        next = {pd};
        carryHover(next, prev);
        CHECK_F(next[0].hs, 1.0f, 1e-6f);
    }

    // carryShelfHover matches by pkg - the panel index can shift under a
    // rebuild when a window closes
    {
        std::vector<ShelfItem> prev(1), next(1);
        prev[0].pkg = "a"; prev[0].panelIdx = 2; prev[0].hs = 1.1f;
        next[0].pkg = "a"; next[0].panelIdx = 0;
        carryShelfHover(next, prev);
        CHECK_F(next[0].hs, 1.1f, 1e-6f);
        // a brand new pkg carries nothing over: its fresh 1.0f stands
        ShelfItem fresh; fresh.pkg = "b";
        next = {fresh};
        carryShelfHover(next, prev);
        CHECK_F(next[0].hs, 1.0f, 1e-6f);
    }

    // hover ticks: the aimed item grows toward the cap, the rest relax,
    // and both converge exactly rather than hovering near it
    {
        std::vector<DockItem> di(2);
        tickDockHover(di, 0, 16.0f);
        CHECK(di[0].hs > 1.0f && di[0].hs < kHoverScale);
        CHECK_F(di[1].hs, 1.0f, 1e-6f);
        for (int i = 0; i < 60; ++i) tickDockHover(di, 0, 16.0f);
        CHECK_F(di[0].hs, kHoverScale, 1e-6f);
        for (int i = 0; i < 60; ++i) tickDockHover(di, -1, 16.0f);
        CHECK_F(di[0].hs, 1.0f, 1e-6f);

        std::vector<ShelfItem> si(1);
        for (int i = 0; i < 60; ++i) tickShelfHover(si, 0, 16.0f);
        CHECK_F(si[0].hs, kHoverScale, 1e-6f);

        std::vector<GridItem> gi(1);
        for (int i = 0; i < 60; ++i) tickGridHover(gi, 0, 16.0f);
        CHECK_F(gi[0].hs, kGridHoverScale, 1e-6f);
        tickGridHover(gi, -1, 16.0f);
        CHECK(gi[0].hs < kGridHoverScale);
    }

    // the summon slide: the dropped strip's plane sits `drop` lower along
    // its own up, and a ray at the rest centre lands high in bar coords -
    // draws and picks agree because both take the same drop
    {
        float c[3], r[3], up[3], dc[3];
        dockCenter(0.0f, -0.55f, o0, c, r, up);
        dockCenterDrop(0.0f, -0.55f, 0.05f, o0, dc, r, up);
        CHECK_F(dc[1], c[1] - up[1] * 0.05f, 1e-5f);
        float d[3] = {c[0], c[1], c[2]};
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= dl; d[1] /= dl; d[2] /= dl;
        float u, v, t;
        CHECK(rayDock(0.0f, -0.55f, 0.05f, o0, o0, d, 0.4f, &u, &v, &t));
        CHECK(v > 0.3f);      // the bar moved down: the ray lands high
        CHECK_F(v, 0.05f / (kDockBarH * 0.5f), 0.35f);
    }
}
