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

    // dragPoint: gaze centred on the panel -> display centre-ish coords
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    float dx, dy;
    CHECK(dragPoint(ps[0], I, &dx, &dy));
    CHECK_F(dx, kVdW * 0.5f, 1e-3f);
    CHECK_F(dy, (0.5f + kPanelY / kPanelH) * kVdH, 1e-3f);

    // gaze yawed onto a side panel: drag point lands on its centre column
    Panel pr2 = mkPanel(kSlotYaw[1]);
    Mat4 v2 = quatToMat((const float[]){0, sinf(kSlotYaw[1] / 2), 0,
                        cosf(kSlotYaw[1] / 2)}, false);
    CHECK(dragPoint(pr2, v2, &dx, &dy));
    CHECK_F(dx, kVdW * 0.5f, 1e-3f);

    // gaze way past the panel edge clamps inside the window, not outside
    Mat4 far = quatToMat((const float[]){0, sinf(-0.30f), 0,
                         cosf(-0.30f)}, false);
    CHECK(dragPoint(ps[0], far, &dx, &dy));
    CHECK_F(dx, 0.0f, 1e-3f);

    // facing away entirely: the ray can't reach the plane
    Mat4 back = quatToMat((const float[]){0, 1.0f, 0, 0}, false);
    CHECK(!dragPoint(ps[0], back, &dx, &dy));

    // dragBoost amplifies the delta from the grab point and clamps
    CHECK_F(dragBoost(400.0f, 500.0f, kVdW), 400.0f + 100.0f * kDragGain,
            1e-3f);
    CHECK_F(dragBoost(400.0f, 300.0f, kVdW), 400.0f - 100.0f * kDragGain,
            1e-3f);
    CHECK_F(dragBoost(400.0f, 400.0f, kVdW), 400.0f, 1e-3f);
    CHECK_F(dragBoost(1500.0f, 1600.0f, kVdW), (float)kVdW, 1e-3f);
    CHECK_F(dragBoost(100.0f, 0.0f, kVdW), 0.0f, 1e-3f);

    // rayPanel reports the unclamped offset so misses are detectable
    float ru, rv, rt;
    float fwd[3] = {0, 0, -1};
    CHECK(rayPanel(ps[0], fwd, &ru, &rv, &rt));
    CHECK_F(ru, 0.0f, 1e-5f);
    CHECK_F(rt, kPanelDist, 1e-4f);
    float away[3] = {0, 0, 1};
    CHECK(!rayPanel(ps[0], away, &ru, &rv, &rt));

    // pill sizing: hugs the label, always narrower than the window
    const float winHW = kPanelW / 2;
    CHECK_F(pillHalfWidth(0.20f, winHW, false), 0.10f + kPillPadX, 1e-6f);
    CHECK(pillHalfWidth(0.20f, winHW, false) < winHW);
    // no label still leaves a 2:1 lozenge, not a dot
    CHECK_F(pillHalfWidth(0.0f, winHW, false), kBarH, 1e-6f);
    // long labels clamp inside the window edges
    CHECK_F(pillHalfWidth(10.0f, winHW, false), winHW - kBarInset, 1e-6f);
    // text limit matches the pill's padded interior at the clamp
    CHECK_F(pillTextLimit(winHW, false),
            (winHW - kBarInset - kPillPadX) * 2.0f, 1e-6f);
    CHECK(pillTextLimit(winHW, false) < kPanelW);

    // a closable pill reserves the button strip on its right end
    CHECK_F(pillHalfWidth(0.20f, winHW, true),
            0.10f + kPillPadX + kPillBtnW, 1e-6f);
    CHECK_F(pillHalfWidth(0.0f, winHW, true), kPillPadX + kPillBtnW, 1e-6f);
    CHECK_F(pillHalfWidth(10.0f, winHW, true), winHW - kBarInset, 1e-6f);
    CHECK_F(pillTextLimit(winHW, true),
            (winHW - kBarInset - kPillPadX - kPillBtnW) * 2.0f, 1e-6f);

    // button layout: close hugs the right edge, minimize sits to its left
    const float phw = 0.30f;
    CHECK_F(pillCloseX(phw), phw - kPillBtnPad - kPillBtnR, 1e-6f);
    CHECK_F(pillMinX(phw),
            phw - kPillBtnPad - 3.0f * kPillBtnR - kPillBtnGap, 1e-6f);
    CHECK(pillMinX(phw) > 0.0f && pillMinX(phw) < pillCloseX(phw));

    // the pill band hangs under the window: centred v is on it, the window
    // interior and points below the pill are not
    const float pillVC = -(kPanelH * 0.5f + kBarGap + kBarH * 0.5f) /
                         (kPanelH * 0.5f);
    CHECK(onPill(0.0f, pillVC, phw));
    CHECK(!onPill(0.0f, 0.0f, phw));
    CHECK(!onPill(0.0f, pillVC - 0.20f, phw));
    CHECK(!onPill(phw / (kPanelW * 0.5f) + 0.05f, pillVC, phw));
    // button hits land on their discs, the middle of the pill is label
    const float uc = pillCloseX(phw) / (kPanelW * 0.5f);
    const float um = pillMinX(phw) / (kPanelW * 0.5f);
    CHECK(pillButtonAt(uc, pillVC, phw) == ZONE_CLOSE);
    CHECK(pillButtonAt(um, pillVC, phw) == ZONE_MIN);
    CHECK(pillButtonAt(0.0f, pillVC, phw) == ZONE_LABEL);

    // the drag handle hangs under the pill's middle: its centre hits, the
    // pill band, the window interior and far below do not
    const float handleVC = -handleDrop() / (kPanelH * 0.5f);
    CHECK(onHandle(0.0f, handleVC));
    CHECK(!onHandle(0.0f, 0.0f));
    CHECK(!onHandle(0.0f, pillVC));
    CHECK(!onHandle(0.0f, handleVC - 0.30f));
    CHECK(!onHandle((kHandleW + kHandlePad + 0.02f) / (kPanelW * 0.5f),
                    handleVC));

    // gaze picks report the chrome zone: aim a fake head straight at a
    // world point (pickPanel only reads the head's -z column)
    ps.clear();
    Panel bp = mkPanel(0.0f);
    bp.pillHW = phw;
    ps.push_back(bp);
    const float pillY = kPanelY - (kPanelH * 0.5f + kBarGap + kBarH * 0.5f);
    Mat4 aim = identity();
    aim.m[2] = -0.0f; aim.m[6] = -pillY; aim.m[10] = kPanelDist;
    pk = pickPanel(ps, aim);   // dead centre of the pill: the label
    CHECK(pk.idx == 0 && pk.zone == ZONE_LABEL);
    aim.m[2] = -pillCloseX(phw);   // right end: the close button
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == 0 && pk.zone == ZONE_CLOSE);
    aim.m[2] = -pillMinX(phw);     // next to it: minimize
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == 0 && pk.zone == ZONE_MIN);
    aim.m[6] = 0.60f;              // way under the pill: nothing
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == -1 && pk.zone == ZONE_NONE);
    // centred under the pill: the drag handle
    const float handleY = kPanelY - handleDrop();
    aim.m[2] = -0.0f; aim.m[6] = -handleY;
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == 0 && pk.zone == ZONE_HANDLE);
    aim.m[2] = -(kHandleW + kHandlePad + 0.03f);   // past the line: nothing
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == -1 && pk.zone == ZONE_NONE);

    // a ring drag shifts every panel by the same delta, keeping slot offsets
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    ps.push_back(mkPanel(kSlotYaw[1]));
    ps.push_back(mkPanel(kSlotYaw[2]));

    // the middle of the ring is the centre-slot panel; only it owns a handle
    CHECK(middleIndex(ps) == 0);
    ps[0].minimized = true;   // centre hidden: a side panel takes over
    CHECK(middleIndex(ps) == 1);
    ps[0].minimized = false;
    ps.clear();
    CHECK(middleIndex(ps) == -1);
    ps.push_back(mkPanel(0.5f));
    CHECK(middleIndex(ps) == 0);
    ps.pop_back();
    ps.push_back(mkPanel(0.0f));
    ps.push_back(mkPanel(kSlotYaw[1]));
    ps.push_back(mkPanel(kSlotYaw[2]));

    // a side panel has no handle: aiming under its pill hits nothing
    Mat4 side = identity();
    side.m[2] = -sinf(kSlotYaw[1]) * kPanelDist;
    side.m[6] = -handleY;
    side.m[10] = cosf(kSlotYaw[1]) * kPanelDist;
    pk = pickPanel(ps, side);
    CHECK(pk.idx == -1 && pk.zone == ZONE_NONE);

    grabRing(ps);
    dragRing(ps, 0.30f);
    CHECK_F(ps[0].yaw, kSlotYaw[0] + 0.30f, 1e-6f);
    CHECK_F(ps[1].yaw, kSlotYaw[1] + 0.30f, 1e-6f);
    CHECK_F(ps[2].yaw, kSlotYaw[2] + 0.30f, 1e-6f);
    // the next tick re-applies the snapshot, not accumulated yaw
    dragRing(ps, -0.10f);
    CHECK_F(ps[0].yaw, -0.10f, 1e-6f);
    CHECK_F(ps[2].yaw, kSlotYaw[2] - 0.10f, 1e-6f);
    // delta wraps across +-pi instead of throwing panels off the ring
    ps.clear();
    ps.push_back(mkPanel((float)M_PI - 0.05f));
    grabRing(ps);
    dragRing(ps, 0.20f);
    CHECK_F(ps[0].yaw, -(float)M_PI + 0.15f, 1e-5f);

    // the library pill has no buttons: its whole band picks as label
    ps.clear();
    ps.push_back(mkPanel(0.0f, kLibraryPkg));
    aim.m[2] = -pillCloseX(kBarH); aim.m[6] = -pillY;
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == 0 && pk.zone == ZONE_LABEL);

    // minimized windows vanish from picking and stay findable for restore
    ps[0].minimized = true;
    pk = pickPanel(ps, aim);
    CHECK(pk.idx == -1);
    CHECK(minimizedIndex(ps, kLibraryPkg) == 0);
    CHECK(minimizedIndex(ps, "com.x.app") == -1);
    ps[0].minimized = false;
    CHECK(minimizedIndex(ps, kLibraryPkg) == -1);
}
