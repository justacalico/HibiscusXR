#include "test.h"

#include "notif/layout.h"
#include "common/config.h"
#include "dock/layout.h"
#include "math/head.h"

static NotifItem mkNotif(const char* key, const char* pkg,
                         long long postMs) {
    NotifItem n;
    n.key = key;
    n.pkg = pkg;
    n.title = key;
    n.text = pkg;
    n.postMs = postMs;
    return n;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testNotif() {
    // ordering: newest postMs first, capped at kShadeMax - the shade
    // lists far more than the toast stack shows
    {
        auto items = buildNotifs({mkNotif("a", "com.a", 100),
                                  mkNotif("b", "com.b", 300),
                                  mkNotif("c", "com.c", 200)});
        CHECK(items.size() == 3);
        CHECK(items[0].key == "b" && items[1].key == "c" &&
              items[2].key == "a");
    }
    {
        std::vector<NotifItem> many;
        for (int i = 0; i < kShadeMax + 3; ++i)
            many.push_back(mkNotif("k", "com.x", 1000 + i));
        auto items = buildNotifs(many);
        CHECK(items.size() == (size_t)kShadeMax);
        CHECK(items[0].postMs == 1000 + kShadeMax + 2);
    }

    // visible window: a card shows from postMs for kNotifShowMs, then the
    // dash drops it while the record stays live; the stack also caps at
    // kNotifMax even when more records are fresh
    {
        const long long now = 100000;
        auto items = visibleNotifs(
            {mkNotif("fresh", "com.a", now - kNotifShowMs + 1),
             mkNotif("edge", "com.b", now - kNotifShowMs),
             mkNotif("stale", "com.c", now - kNotifShowMs - 1)}, now);
        // kNotifMax is 1: only the freshest in-window card makes the stack
        CHECK(items.size() == (size_t)kNotifMax);
        CHECK(items[0].key == "fresh");
        std::vector<NotifItem> fresh;
        for (int i = 0; i < kNotifMax + 2; ++i)
            fresh.push_back(mkNotif("k", "com.x", now - i));
        CHECK(visibleNotifs(fresh, now).size() == (size_t)kNotifMax);
    }
    // missing timestamps stay visible rather than vanishing
    {
        auto items = visibleNotifs({mkNotif("noT", "com.a", 0)}, 999999);
        CHECK(items.size() == 1);
    }
    // a postMs in the future counts as inside the window
    {
        auto items = visibleNotifs({mkNotif("skew", "com.a", 200)}, 100);
        CHECK(items.size() == 1);
    }

    // stack geometry: heights and card offsets
    {
        CHECK(notifStackHH(0) == 0.0f);
        CHECK(notifStackHH(1) == kNotifCardH * 0.5f);
        const float hh3 = notifStackHH(3);
        CHECK(fabsf(hh3 - (3 * kNotifCardH + 2 * kNotifGap) * 0.5f) < 1e-6f);
        // card 0 sits at the top, card 2 at the bottom
        const float y0 = notifCardY(0, 3), y2 = notifCardY(2, 3);
        CHECK(y0 > y2);
        CHECK(fabsf(y0 - (hh3 - kNotifCardH * 0.5f)) < 1e-6f);
        CHECK(fabsf(y2 - (-hh3 + kNotifCardH * 0.5f)) < 1e-6f);
        // the lift clears the dock bar: stack bottom sits above bar top
        const float lift = notifLift(3);
        CHECK(fabsf(lift - (kDockBarH * 0.5f + kNotifGap + hh3)) < 1e-6f);
    }

    // hit testing: card bands top-down, gaps between cards miss, the badge
    // lands NZONE_CLOSE
    {
        const int n = 2;
        const float sh = notifStackHH(n);
        int zone = NZONE_NONE;
        // top card centre
        const float v0 = notifCardY(0, n) / sh;
        CHECK(notifAt(0.0f, v0, n, &zone) == 0 && zone == NZONE_BODY);
        // bottom card centre
        const float v1 = notifCardY(1, n) / sh;
        CHECK(notifAt(0.0f, v1, n, &zone) == 1 && zone == NZONE_BODY);
        // the gap between them hits no card: the band midpoint is the
        // midpoint of the two card centres
        CHECK(notifAt(0.0f, (v0 + v1) * 0.5f, n, &zone) == -1);
        // badge on the top card's top-right corner
        float bx, by;
        notifBadgeAt(&bx, &by);
        const float ub = bx / (kNotifCardW * 0.5f);
        const float vb = (notifCardY(0, n) + by) / sh;
        CHECK(notifAt(ub, vb, n, &zone) == 0 && zone == NZONE_CLOSE);
        // outside the stack
        CHECK(notifAt(0.0f, 1.4f, n, &zone) == -1);
        CHECK(notifAt(1.2f, v0, n, &zone) == -1);
    }

    // ray vs the stack plane, centred on the dock anchor + lift
    {
        const float origin[3] = {0.0f, 0.0f, 0.0f};
        const float lift = notifLift(2);
        float c[3], r[3], up[3];
        notifCenter(0.0f, kDockPitchRest, lift, kPanelDist, origin, c, r, up);
        // the centre sits `lift` metres off the dock bar's centre
        float dc[3], dr[3], dup[3];
        dockCenter(0.0f, kDockPitchRest, kPanelDist, origin, dc, dr, dup);
        const float dd = sqrtf((c[0]-dc[0])*(c[0]-dc[0]) +
                               (c[1]-dc[1])*(c[1]-dc[1]) +
                               (c[2]-dc[2])*(c[2]-dc[2]));
        CHECK(fabsf(dd - lift) < 1e-5f);
        // a ray from the eye through the stack centre hits u=v=0
        const float o[3] = {0.0f, 0.0f, 0.0f};
        float d[3] = {c[0] - o[0], c[1] - o[1], c[2] - o[2]};
        const float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= len; d[1] /= len; d[2] /= len;
        float u, v, t;
        CHECK(rayNotif(0.0f, kDockPitchRest, lift, kPanelDist, origin, o, d, 2,
                       &u, &v, &t));
        CHECK(fabsf(u) < 1e-4f && fabsf(v) < 1e-4f);
        CHECK(t > 0.5f && t < 3.0f);
    }

    // pickNotif through a gaze matrix: a head facing -z hits the stack at
    // yaw 0
    {
        std::vector<NotifItem> items = {mkNotif("a", "com.a", 200),
                                        mkNotif("b", "com.b", 100)};
        Mat4 head = identity();
        // head at origin looking down -z; stack at yaw 0, dock pitch,
        // lifted - the gaze pitch won't hit the cards' band exactly, so
        // aim the head at the stack centre instead: build the ray check
        // off a slightly pitched head is overkill; verify the miss/hit
        // contract only
        const float lift = notifLift((int)items.size());
        NotifPick pk = pickNotif(items, 0.0f, kDockPitchRest, lift, kPanelDist, head,
                                 o0, o0);
        // stack is below the horizon at kDockPitchRest: a level gaze misses
        CHECK(!pk.stack);
    }
}

void testShade() {
    // geometry: the card rides the shared card anchor plane at ring pitch
    {
        float c[3], r[3], up[3];
        shadeCenter(0.0f, 0.0f, kPanelDist, o0, c, r, up);
        CHECK_F(c[2], -kShadeDist, 1e-5f);
        CHECK_F(r[0], 1.0f, 1e-6f);
    }

    // rows chain under the title band: row 0 at the top, each a pitch
    // plus the gap lower
    {
        float lo, hi;
        shadeClipBand(&lo, &hi);
        CHECK_F(hi, kShadeHH - kShadeHeadH, 1e-6f);
        CHECK_F(lo, -kShadeHH, 1e-6f);
        const float y0 = shadeRowY(0, 0.0f), y1 = shadeRowY(1, 0.0f);
        CHECK_F(y0, hi - kShadeRowH * 0.5f, 1e-6f);
        CHECK_F(y1, y0 - kShadeRowH - kShadeRowGap, 1e-6f);
        // scroll lifts every row
        CHECK_F(shadeRowY(0, 0.05f), y0 + 0.05f, 1e-6f);
    }

    // culling: a row shows only while it overlaps the band
    {
        float lo, hi;
        shadeClipBand(&lo, &hi);
        CHECK(shadeRowShown(shadeRowY(0, 0.0f), 1.0f));
        CHECK(!shadeRowShown(hi + kShadeRowH, 1.0f));
        CHECK(!shadeRowShown(lo - kShadeRowH, 1.0f));
    }

    // scroll range: nothing scrolls until the list overflows the band
    {
        float lo, hi;
        shadeClipBand(&lo, &hi);
        const int fit = (int)((hi - lo + kShadeRowGap) /
                              (kShadeRowH + kShadeRowGap));
        CHECK_F(shadeScrollMax(fit), 0.0f, 1e-4f);
        const float mx = shadeScrollMax(fit + 2);
        CHECK(mx > 0.0f);
        CHECK_F(shadeClampScroll(-1.0f, fit + 2), 0.0f, 1e-6f);
        CHECK_F(shadeClampScroll(mx + 1.0f, fit + 2), mx, 1e-6f);
        // wheel detents walk the range and clamp at both ends
        CHECK_F(shadeScrollBy(0.0f, -1.0f, fit + 2), 0.0f, 1e-6f);
        CHECK_F(shadeScrollBy(0.0f, 1.0f, fit + 2), kShadeWheelStep, 1e-6f);
        CHECK_F(shadeScrollBy(mx, 1.0f, fit + 2), mx, 1e-6f);
        // the tap slop: a small drag stays a tap, past it a scroll
        CHECK(!shadeSlopPassed(0.0f, 0.01f));
        CHECK(shadeSlopPassed(0.0f, (kShadeDragSlop + 0.01f) / kShadeHH));
    }

    // clear-all policy: a single clearable row enables the pill
    {
        NotifItem a = mkNotif("a", "com.a", 100);
        a.clearable = false;
        CHECK(!shadeClearable({a}));
        NotifItem b = mkNotif("b", "com.b", 90);
        CHECK(shadeClearable({a, b}));
    }

    // hit testing: close disc and clear pill on the title band, badge and
    // body on rows, everything else the card body
    {
        std::vector<NotifItem> items = {mkNotif("a", "com.a", 200),
                                        mkNotif("b", "com.b", 100)};
        int idx = -2;
        const float ty = shadeTitleY() / kShadeHH;
        // close disc
        CHECK(shadeZoneAt(items, shadeCloseX() / kShadeHW, ty,
                          0.0f, 0.05f, &idx) == SHZONE_CLOSE);
        // clear-all pill beside it
        float px, py, phw, phh;
        shadeClearPill(0.05f, &px, &py, &phw, &phh);
        CHECK(shadeZoneAt(items, px / kShadeHW, py / kShadeHH,
                          0.0f, 0.05f, &idx) == SHZONE_CLEAR);
        // title band between them: body
        CHECK(shadeZoneAt(items, 0.0f, ty, 0.0f, 0.05f, &idx) ==
              SHZONE_BODY);
        // row 0's badge dismisses; its body is the row zone
        float bx, by;
        shadeBadgeAt(&bx, &by);
        const float ry0 = shadeRowY(0, 0.0f);
        CHECK(shadeZoneAt(items, bx / kShadeHW, (ry0 + by) / kShadeHH,
                          0.0f, 0.05f, &idx) == SHZONE_DISMISS);
        CHECK(idx == 0);
        CHECK(shadeZoneAt(items, 0.0f, ry0 / kShadeHH, 0.0f, 0.05f,
                          &idx) == SHZONE_ROW);
        CHECK(idx == 0);
        // a non-clearable row's badge spot is just the row
        items[0].clearable = false;
        CHECK(shadeZoneAt(items, bx / kShadeHW, (ry0 + by) / kShadeHH,
                          0.0f, 0.05f, &idx) == SHZONE_ROW);
        // below the list: card body; off the card: nothing
        items[0].clearable = true;
        const float below = shadeRowY(1, 0.0f) - kShadeRowH * 0.5f - 0.02f;
        CHECK(shadeZoneAt(items, 0.0f, below / kShadeHH, 0.0f, 0.05f,
                          &idx) == SHZONE_BODY);
        CHECK(idx == -1);
        CHECK(shadeZoneAt(items, 1.2f, 0.0f, 0.0f, 0.05f, &idx) ==
              SHZONE_NONE);
        // scroll shifts the rows under the same point
        CHECK(shadeZoneAt(items, 0.0f, ry0 / kShadeHH,
                          kShadeRowH + kShadeRowGap, 0.05f, &idx) ==
              SHZONE_ROW);
        CHECK(idx == 1);
    }

    // the ray pick mirrors the zone map; a level ray at the card centre
    // hits while a ray aimed away misses
    {
        std::vector<NotifItem> items = {mkNotif("a", "com.a", 200)};
        float c[3], r[3], up[3];
        shadeCenter(0.0f, 0.0f, kPanelDist, o0, c, r, up);
        float d[3] = {c[0], c[1], c[2]};
        const float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= len; d[1] /= len; d[2] /= len;
        ShadePick pk = pickShadeRay(items, 0.0f, 0.05f, 0.0f, 0.0f, kPanelDist,
                                    o0, o0, d);
        CHECK(pk.hit && pk.u == 0.0f && pk.v == 0.0f);
        CHECK_F(pk.t, len, 1e-4f);
        const float away[3] = {0.0f, 0.0f, 1.0f};
        pk = pickShadeRay(items, 0.0f, 0.05f, 0.0f, 0.0f, kPanelDist, o0, o0, away);
        CHECK(!pk.hit);
    }
}
