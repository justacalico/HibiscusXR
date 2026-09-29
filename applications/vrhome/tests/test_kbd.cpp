#include "test.h"

#include "kbd/kbd.h"
#include "panels/layout.h"
#include "common/config.h"

#include <cmath>

static Panel mkPanel(int disp, const char* pkg = "com.x.app") {
    Panel p;
    p.displayId = disp;
    p.yaw = 0.0f;
    p.pkg = pkg;
    return p;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testKbd() {
    // host pick: the display the last window tap landed on wins
    std::vector<Panel> two = {mkPanel(10), mkPanel(20)};
    two[1].yaw = 0.88f;
    CHECK(kbdHostIndex(two, 20) == 1);
    CHECK(kbdHostIndex(two, 10) == 0);
    // unknown host falls back to the first visible window
    CHECK(kbdHostIndex(two, 99) == 0);
    // a minimized host can't hold the quad; the fallback skips it too
    two[0].minimized = true;
    CHECK(kbdHostIndex(two, 10) == 1);
    CHECK(kbdHostIndex(two, 99) == 1);
    two[1].minimized = true;
    CHECK(kbdHostIndex(two, -1) == -1);
    std::vector<Panel> none;
    CHECK(kbdHostIndex(none, 5) == -1);

    // the quad is its own window: host orientation, dropped under the
    // host's bottom edge, pulled toward the eye to kKbdDist
    Panel p = mkPanel(10);
    float pc[3], pr[3], pu[3], c[3], r[3], up[3];
    panelCenter(p, o0, pc, pr, pu);
    kbdCenter(p, o0, 0.0f, 0.0f, c, r, up);
    for (int i = 0; i < 3; ++i) {
        CHECK_F(r[i], pr[i], 1e-6f);
        CHECK_F(up[i], pu[i], 1e-6f);
    }
    const float drop = kPanelH * 0.5f + kKbdGap + kKbdHH;
    float n[3] = {o0[0] - pc[0], o0[1] - pc[1], o0[2] - pc[2]};
    const float nl = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    const float pull = nl - kKbdDist;
    for (int i = 0; i < 3; ++i) {
        const float want = pc[i] - up[i] * drop + n[i] / nl * pull;
        CHECK_F(c[i], want, 1e-5f);
    }
    // the pull carries the quad to its own ring nearer than the dock
    float dc[3];
    for (int i = 0; i < 3; ++i) dc[i] = c[i] + up[i] * drop;
    const float dlen = sqrtf(dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);
    CHECK_F(dlen, kKbdDist, 1e-4f);
    CHECK(kKbdDist < kDockDist);

    // the pill's own placement: offYaw swings the frame around the viewer
    // like a ring move (centre lands where a yaw+offYaw point on the same
    // ring would), offY slides it along its up
    float c2[3], r2[3], up2[3];
    kbdCenter(p, o0, 0.4f, 0.2f, c2, r2, up2);
    const float cs = cosf(0.4f), sn = sinf(0.4f);
    // centre: rotate the un-offset centre by the yaw, then shift along up
    const float cx = c[0] * cs - c[2] * sn;
    const float cz = c[0] * sn + c[2] * cs;
    for (int i = 0; i < 3; ++i) {
        const float want = (i == 0 ? cx : i == 1 ? c[1] : cz) +
                           up2[i] * 0.2f;
        CHECK_F(c2[i], want, 1e-5f);
    }
    // right/up rotate by the same yaw: the frame stays rigid
    CHECK_F(r2[0], r[0] * cs - r[2] * sn, 1e-5f);
    CHECK_F(r2[2], r[0] * sn + r[2] * cs, 1e-5f);
    CHECK_F(r2[1], r[1], 1e-5f);

    // the drag pill sits under the quad's bottom edge
    CHECK_F(kbdHandleDrop(), kKbdHH + kHandleGap + kHandleT, 1e-6f);
    CHECK(onKbdHandle(0.0f, -kbdHandleDrop() / kKbdHH));
    CHECK(!onKbdHandle(0.0f, 0.0f));   // quad centre is a key, not the pill
    CHECK(!onKbdHandle(0.0f, -1.0f));  // the bottom edge itself isn't
    CHECK(!onKbdHandle(0.9f, -kbdHandleDrop() / kKbdHH));

    // a ray from the eye straight at the quad hits inside its bounds and
    // nearer than the panel plane behind it
    float dx = c[0], dy = c[1], dz = c[2];
    const float dl = sqrtf(dx*dx + dy*dy + dz*dz);
    const float d[3] = {dx / dl, dy / dl, dz / dl};
    float u, v, t;
    CHECK(rayQuad(c, r, up, o0, o0, d, kKbdHW, kKbdHH, &u, &v, &t));
    CHECK(fabsf(u) <= 1.0f && fabsf(v) <= 1.0f);
    float pu2, pv2, pt2;
    CHECK(rayPanel(p, o0, o0, d, &pu2, &pv2, &pt2));
    CHECK(t < pt2);

    // hit->px mapping: centre is centre, corners map to display extents
    float x, y;
    kbdHitPx(0.0f, 0.0f, &x, &y);
    CHECK_F(x, kKbdW * 0.5f, 1e-4f);
    CHECK_F(y, kKbdH * 0.5f, 1e-4f);
    kbdHitPx(-1.0f, 1.0f, &x, &y);
    CHECK_F(x, 0.0f, 1e-4f);
    CHECK_F(y, 0.0f, 1e-4f);
    kbdHitPx(1.0f, -1.0f, &x, &y);
    CHECK_F(x, (float)kKbdW, 1e-4f);
    CHECK_F(y, (float)kKbdH, 1e-4f);
}
