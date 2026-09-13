#include "test.h"

#include "panels/layout.h"
#include "common/config.h"
#include "math/head.h"

static Panel mkPanel(float yaw, const char* pkg = "com.x.app") {
    Panel p;
    p.yaw = yaw;
    p.pkg = pkg;
    return p;
}

void testLayout() {
    // panel at yaw 0 sits dead ahead on -z at ring distance
    Panel p0 = mkPanel(0.0f);
    float c[3], r[3];
    panelCenter(p0, c, r);
    CHECK_F(c[0], 0.0f, 1e-6f);
    CHECK_F(c[1], kPanelY, 1e-6f);
    CHECK_F(c[2], -kPanelDist, 1e-6f);
    CHECK_F(r[0], 1.0f, 1e-6f);   // right edge on +x
    CHECK_F(r[2], 0.0f, 1e-6f);

    // yaw +pi/2: centre on +x, right vector turns to +z
    Panel pr = mkPanel((float)M_PI / 2);
    panelCenter(pr, c, r);
    CHECK_F(c[0], kPanelDist, 1e-5f);
    CHECK_F(c[2], 0.0f, 1e-5f);
    CHECK_F(r[0], 0.0f, 1e-5f);
    CHECK_F(r[2], 1.0f, 1e-5f);

    // slot picking: empty ring -> centre slot; then left, then right
    std::vector<Panel> ps;
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[0], 1e-6f);
    ps.push_back(mkPanel(0.0f));
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[1], 1e-6f);
    ps.push_back(mkPanel(kSlotYaw[1]));
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[2], 1e-6f);
    ps.push_back(mkPanel(kSlotYaw[2]));
    CHECK_F(freeSlotYaw(ps, 0.0f), 0.0f, 1e-6f);  // full: centre

    // slots follow the ring centre
    ps.clear();
    ps.push_back(mkPanel(1.0f));
    CHECK_F(freeSlotYaw(ps, 1.0f), 1.0f + kSlotYaw[1], 1e-6f);

    // eviction: library panel is protected, oldest app goes first
    ps.clear();
    ps.push_back(mkPanel(0.0f, kLibraryPkg));
    ps.push_back(mkPanel(kSlotYaw[1]));
    ps.push_back(mkPanel(kSlotYaw[2]));
    CHECK(evictIndex(ps) == 1);
    ps[1].pkg = kLibraryPkg;
    CHECK(evictIndex(ps) == 2);
    ps[2].pkg = kLibraryPkg;
    CHECK(evictIndex(ps) == -1);
    ps.clear();
    CHECK(evictIndex(ps) == -1);

    // recenter snaps each panel to its nearest slot around the new centre
    ps.clear();
    ps.push_back(mkPanel(1.02f));   // near the centre slot -> stays
    ps.push_back(mkPanel(1.0f + kSlotYaw[1] * 0.6f));  // nearer left slot
    recenterSlots(ps, 1.0f);
    CHECK_F(ps[0].yaw, 1.0f + kSlotYaw[0], 1e-5f);
    CHECK_F(ps[1].yaw, 1.0f + kSlotYaw[1], 1e-5f);

    // gaze pick: identity head looks down -z, hits the centre panel
    Mat4 I = identity();
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    Pick pk = pickPanel(ps, I);
    CHECK(pk.idx == 0);
    CHECK_F(pk.u, 0.0f, 1e-5f);
    CHECK_F(pk.v, -kPanelY / (kPanelH / 2), 1e-4f);  // centre sits slightly up

    // panel off-axis is missed when looking straight ahead
    ps.clear();
    ps.push_back(mkPanel(kSlotYaw[1]));
    pk = pickPanel(ps, I);
    CHECK(pk.idx == -1);

    // head yawed to the left slot: quat about +Y is a view-space yaw
    const float a = kSlotYaw[1];   // turn toward the panel
    Mat4 v = quatToMat((const float[]){0, sinf(a / 2), 0, cosf(a / 2)},
                       false);
    pk = pickPanel(ps, v);
    CHECK(pk.idx == 0);
    CHECK_F(pk.u, 0.0f, 1e-3f);

    // two panels both partly under the ray: nearest wins
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    ps.push_back(mkPanel(0.05f));
    pk = pickPanel(ps, I);
    CHECK(pk.idx >= 0);

    // pill sizing: hugs the label, always narrower than the window
    const float winHW = kPanelW / 2;
    CHECK_F(pillHalfWidth(0.20f, winHW), 0.10f + kPillPadX, 1e-6f);
    CHECK(pillHalfWidth(0.20f, winHW) < winHW);
    // no label still leaves a 2:1 lozenge, not a dot
    CHECK_F(pillHalfWidth(0.0f, winHW), kBarH, 1e-6f);
    // long labels clamp inside the window edges
    CHECK_F(pillHalfWidth(10.0f, winHW), winHW - kBarInset, 1e-6f);
    // text limit matches the pill's padded interior at the clamp
    CHECK_F(pillTextLimit(winHW), (winHW - kBarInset - kPillPadX) * 2.0f,
            1e-6f);
    CHECK(pillTextLimit(winHW) < kPanelW);
}
