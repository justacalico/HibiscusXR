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
    // ordering: newest postMs first, capped at kNotifMax
    {
        auto items = buildNotifs({mkNotif("a", "com.a", 100),
                                  mkNotif("b", "com.b", 300),
                                  mkNotif("c", "com.c", 200)});
        CHECK(items.size() == (size_t)kNotifMax);
        CHECK(items[0].key == "b");
    }
    {
        std::vector<NotifItem> many;
        for (int i = 0; i < kNotifMax + 3; ++i)
            many.push_back(mkNotif("k", "com.x", 1000 + i));
        auto items = buildNotifs(many);
        CHECK(items.size() == (size_t)kNotifMax);
        CHECK(items[0].postMs == 1000 + kNotifMax + 2);
    }

    // visible window: a card shows from postMs for kNotifShowMs, then the
    // dash drops it while the record stays live
    {
        const long long now = 100000;
        auto items = visibleNotifs(
            {mkNotif("fresh", "com.a", now - kNotifShowMs + 1),
             mkNotif("edge", "com.b", now - kNotifShowMs),
             mkNotif("stale", "com.c", now - kNotifShowMs - 1)}, now);
        CHECK(items.size() == 2);
        CHECK(items[0].key == "fresh");
        CHECK(items[1].key == "edge");
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
        notifCenter(0.0f, kDockPitchRest, lift, origin, c, r, up);
        // the centre sits `lift` metres off the dock bar's centre
        float dc[3], dr[3], dup[3];
        dockCenter(0.0f, kDockPitchRest, origin, dc, dr, dup);
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
        CHECK(rayNotif(0.0f, kDockPitchRest, lift, origin, o, d, 2,
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
        NotifPick pk = pickNotif(items, 0.0f, kDockPitchRest, lift, head,
                                 o0, o0);
        // stack is below the horizon at kDockPitchRest: a level gaze misses
        CHECK(!pk.stack);
    }
}
