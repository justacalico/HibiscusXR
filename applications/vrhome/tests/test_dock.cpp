#include "test.h"

#include "dock/layout.h"
#include "common/config.h"
#include "math/head.h"

static Panel mkPanel(const char* pkg, int taskId = -1) {
    Panel p;
    p.pkg = pkg;
    p.taskId = taskId;
    return p;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testDock() {
    // ordering: pins first, then unpinned running tasks, quick last; the
    // pinned running app carries its panel instead of duplicating
    {
        std::vector<std::string> pins = {"com.a.pin", "com.b.busy"};
        std::vector<Panel> panels = {mkPanel("com.b.busy", 11),
                                     mkPanel("com.c.free", 12)};
        std::vector<XrTask> xr = {{21, "com.d.vr"}};
        auto items = buildDock(pins, panels, xr);
        CHECK(items.size() == 5);
        CHECK(items[0].kind == DK_PIN && items[0].pkg == "com.a.pin");
        CHECK(!items[0].running && items[0].panelIdx < 0);
        CHECK(items[1].kind == DK_PIN && items[1].pkg == "com.b.busy");
        CHECK(items[1].running && items[1].panelIdx == 0 &&
              items[1].taskId == 11);
        CHECK(items[2].kind == DK_RUN && items[2].pkg == "com.c.free");
        CHECK(items[2].sep);               // pin -> running boundary
        CHECK(items[3].kind == DK_RUN && items[3].pkg == "com.d.vr");
        CHECK(items[3].vr && items[3].taskId == 21 && !items[3].sep);
        CHECK(items[4].kind == DK_QUICK && items[4].sep);
        CHECK(items[4].pkg == kQuickPanelPkg);
    }

    // a pinned XR app shows once, marked immersive + running
    {
        std::vector<std::string> pins = {"com.d.vr"};
        std::vector<XrTask> xr = {{7, "com.d.vr"}};
        auto items = buildDock(pins, {}, xr);
        CHECK(items.size() == 2);
        CHECK(items[0].kind == DK_PIN && items[0].vr && items[0].running &&
              items[0].taskId == 7);
    }

    // a pin for the quick-panel app is dropped: its slot already exists
    {
        std::vector<std::string> pins = {kQuickPanelPkg, "com.a.pin"};
        auto items = buildDock(pins, {}, {});
        CHECK(items.size() == 2);
        CHECK(items[0].pkg == "com.a.pin");
        CHECK(items[1].kind == DK_QUICK);
    }

    // a running quick-panel task rides its own button: no duplicate icon,
    // the button carries the running dot and the task
    {
        std::vector<Panel> panels = {mkPanel(kQuickPanelPkg, 9)};
        auto items = buildDock({}, panels, {});
        CHECK(items.size() == 1);
        CHECK(items[0].kind == DK_QUICK && items[0].running);
        CHECK(items[0].taskId == 9 && items[0].panelIdx == 0);
        CHECK(!items[0].sep);              // alone: nothing to split off
    }

    // minimized panels stay listed as running, marked minimized
    {
        std::vector<Panel> panels = {mkPanel("com.c.free", 5)};
        panels[0].minimized = true;
        auto items = buildDock({}, panels, {});
        CHECK(items.size() == 2);
        CHECK(items[0].kind == DK_RUN && items[0].minimized &&
              !items[0].sep);              // nothing before it to split off
    }

    // layout: monotone x, separators widen the bar, symmetric extents
    {
        std::vector<DockItem> items;
        DockItem a; a.pkg = "a"; a.kind = DK_PIN;
        DockItem b; b.pkg = "b"; b.kind = DK_RUN; b.sep = true;
        DockItem c; c.pkg = "c"; c.kind = DK_QUICK; c.sep = true;
        items = {a, b, c};
        DockStatus st;
        st.clockW = 0.06f;
        const float hw = dockLayout(items, st);
        // cluster pills + their separator + 3 icons + 2 seps + pad + gaps
        const float clusterW = kSysPillPad * 4.0f + 3.0f * kSysIconW +
                               2.0f * kSysGap + kSysPillGap + st.clockW;
        const float want = (2 * kDockPad + clusterW + kDockGap + kDockSepW +
                            3 * kDockIconW + 2 * kDockSepW +
                            2 * kDockGap) * 0.5f;
        CHECK_F(hw, want, 1e-6f);
        CHECK(items[0].x < items[1].x && items[1].x < items[2].x);
        CHECK_F(items[2].x, hw - kDockPad - kDockIconHW, 1e-6f);
        // the separator gap is wider than a plain icon gap
        CHECK(items[1].x - items[0].x > kDockIconW + kDockGap);
        CHECK(items[2].x - items[1].x > kDockIconW + kDockGap);
    }

    // status cluster: pill A wraps clock-battery-wifi, pill B wraps the
    // bell alone, separator lands before the icons
    {
        std::vector<DockItem> items;
        DockItem a; a.pkg = "a"; a.kind = DK_QUICK;
        items = {a};
        DockStatus st;
        st.clockW = 0.06f;
        const float hw = dockLayout(items, st);
        CHECK_F(st.pillAL, -hw + kDockPad, 1e-6f);
        CHECK(st.pillAL < st.clockX && st.clockX < st.battX);
        CHECK(st.battX < st.wifiX && st.wifiX < st.pillAR);
        CHECK(st.pillAR < st.pillBL && st.pillBL < st.bellX);
        CHECK(st.bellX < st.pillBR && st.pillBR < st.sepX);
        CHECK(st.sepX < items[0].x);
        // a wider clock string widens pill A and pushes the rest right
        DockStatus wide; wide.clockW = 0.12f;
        std::vector<DockItem> items2 = items;
        dockLayout(items2, wide);
        CHECK(wide.pillAR > st.pillAR);
        CHECK(wide.pillBL > st.pillBL);
        CHECK(wide.sepX > st.sepX);
    }

    // an empty item list still places the cluster, just with no separator
    {
        std::vector<DockItem> items;
        DockStatus st;
        st.clockW = 0.06f;
        const float hw = dockLayout(items, st);
        CHECK_F(st.pillAL, -hw + kDockPad, 1e-6f);
        CHECK_F(st.clockX, st.pillAL + kSysPillPad, 1e-6f);
        CHECK(hw > st.clockW);
    }

    // hit test: icon centres hit, gaps and off-bar points miss; a live XR
    // item's badge reports DZONE_CLOSE
    {
        std::vector<DockItem> items;
        DockItem a; a.pkg = "a"; a.kind = DK_PIN;
        DockItem b; b.pkg = "b"; b.kind = DK_RUN; b.vr = true;
                b.running = true; b.sep = true;
        items = {a, b};
        DockStatus st;
        st.clockW = 0.06f;
        const float hw = dockLayout(items, st);
        int zone = DZONE_NONE;
        const float ua = items[0].x / hw, ub = items[1].x / hw;
        const float uy = kDockIconY / (kDockBarH * 0.5f);
        CHECK(dockItemAt(items, hw, ua, uy, &zone) == 0);
        CHECK(zone == DZONE_ICON);
        CHECK(dockItemAt(items, hw, ub, uy, &zone) == 1);
        CHECK(zone == DZONE_ICON);
        // midway between the two icons: bar body, no item
        CHECK(dockItemAt(items, hw, (ua + ub) * 0.5f, uy, &zone) == -1);
        CHECK(zone == DZONE_NONE);
        CHECK(dockItemAt(items, hw, 1.2f, 0.0f, &zone) == -1);
        CHECK(dockItemAt(items, hw, 0.0f, 1.2f, &zone) == -1);
        // the badge: top-right corner of the XR icon
        float bx, by;
        // recompute badge spot: x + iconHW*0.72, y = iconY + iconHW*0.72
        bx = items[1].x + kDockIconHW * 0.72f;
        by = kDockIconY + kDockIconHW * 0.72f;
        CHECK(dockItemAt(items, hw, bx / hw, by / (kDockBarH * 0.5f),
                         &zone) == 1);
        CHECK(zone == DZONE_CLOSE);
        // a non-VR item has no badge there
        bx = items[0].x + kDockIconHW * 0.72f;
        CHECK(dockItemAt(items, hw, bx / hw, by / (kDockBarH * 0.5f),
                         &zone) == 0);
        CHECK(zone == DZONE_ICON);
    }

    // dock pitch: level head drops the strip below the horizon; looking up
    // raises it but never past the clamp
    {
        CHECK_F(dockPitchFor(0.0f), -kDockPitchDrop, 1e-6f);
        CHECK(dockPitchFor(2.0f) <= kDockPitchMax);
        CHECK(dockPitchFor(-3.0f) >= kDockPitchMin);
        CHECK(dockPitchFor(0.8f) > dockPitchFor(0.0f));
    }

    // pin toggle: append missing, drop present
    {
        auto pins = pinToggle({}, "com.a.pin");
        CHECK(pins.size() == 1 && pins[0] == "com.a.pin");
        pins = pinToggle(pins, "com.b.busy");
        CHECK(pins.size() == 2);
        pins = pinToggle(pins, "com.a.pin");
        CHECK(pins.size() == 1 && pins[0] == "com.b.busy");
    }

    // geometry: dock at yaw 0 sits on -z at its own distance, facing the
    // anchor; the gaze ray dead-centre hits u=v=0
    {
        float c[3], r[3], up[3];
        dockCenter(0.0f, -0.55f, o0, c, r, up);
        CHECK_F(c[0], 0.0f, 1e-6f);
        CHECK_F(c[2], -kDockDist, 1e-6f);
        CHECK(c[1] < 0.0f);                // pitched below the anchor
        CHECK_F(r[0], 1.0f, 1e-6f);
        float d[3] = {c[0], c[1], c[2]};   // eye at origin aims at centre
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= dl; d[1] /= dl; d[2] /= dl;
        float u, v, t;
        CHECK(rayDock(0.0f, -0.55f, o0, o0, d, 0.4f, &u, &v, &t));
        CHECK_F(u, 0.0f, 1e-4f);
        CHECK_F(v, 0.0f, 1e-4f);
        CHECK_F(t, dl, 1e-4f);
    }

    // move handle: a padded line centred under the strip; the bar body and
    // points far below or to the side miss
    {
        const float hw = 0.4f;
        const float hv = -dockHandleDrop() / (kDockBarH * 0.5f);
        CHECK(onDockHandle(0.0f, hv, hw));
        CHECK(!onDockHandle(0.0f, 0.0f, hw));
        CHECK(!onDockHandle(0.0f, hv - 0.40f, hw));
        CHECK(!onDockHandle((kHandleW + kHandlePad + 0.02f) / hw, hv, hw));
    }

    // the handle pick lives outside the bar box: pickDock reports it
    // before the in-bar bounds reject, with no item index
    {
        std::vector<DockItem> items;
        DockItem a; a.pkg = "a"; a.kind = DK_QUICK;
        items = {a};
        DockStatus st; st.clockW = 0.06f;
        const float hw = dockLayout(items, st);
        float c[3], r[3], up[3];
        dockCenter(0.0f, -0.55f, o0, c, r, up);
        const float hd = dockHandleDrop();
        Mat4 aim = identity();
        aim.m[2]  = -(c[0] - up[0] * hd);   // -z column aims at the handle
        aim.m[6]  = -(c[1] - up[1] * hd);
        aim.m[10] = -(c[2] - up[2] * hd);
        DockPick pk = pickDock(items, hw, 0.0f, -0.55f, aim, o0, o0);
        CHECK(pk.bar && pk.idx == -1 && pk.zone == DZONE_HANDLE);
        // aiming at the bar body still picks the bar, not the handle
        aim.m[2] = -c[0]; aim.m[6] = -c[1]; aim.m[10] = -c[2];
        pk = pickDock(items, hw, 0.0f, -0.55f, aim, o0, o0);
        CHECK(pk.bar && pk.zone != DZONE_HANDLE);
    }

    // pinnable: everything but the quick button
    {
        DockItem q; q.kind = DK_QUICK;
        DockItem p; p.kind = DK_PIN;
        DockItem r; r.kind = DK_RUN;
        CHECK(!dockPinnable(q));
        CHECK(dockPinnable(p) && dockPinnable(r));
    }
}
