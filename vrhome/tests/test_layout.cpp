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

// most geometry tests run at the world origin: a zero anchor and a zero
// eye position keep the old origin-centred semantics explicit
static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testLayout() {
    // panel at yaw 0 sits dead ahead on -z at ring distance
    Panel p0 = mkPanel(0.0f);
    float c[3], r[3], up[3];
    panelCenter(p0, o0, c, r, up);
    CHECK_F(c[0], 0.0f, 1e-6f);
    CHECK_F(c[1], kPanelY, 1e-6f);
    CHECK_F(c[2], -kPanelDist, 1e-6f);
    CHECK_F(r[0], 1.0f, 1e-6f);   // right edge on +x
    CHECK_F(r[2], 0.0f, 1e-6f);
    CHECK_F(up[0], 0.0f, 1e-5f);
    CHECK(up[1] > 0.99f);         // up is ~+y, nudged by the kPanelY lift

    // yaw +pi/2: centre on +x, right vector turns to +z
    Panel pr = mkPanel((float)M_PI / 2);
    panelCenter(pr, o0, c, r, up);
    CHECK_F(c[0], kPanelDist, 1e-5f);
    CHECK_F(c[2], 0.0f, 1e-5f);
    CHECK_F(r[0], 0.0f, 1e-5f);
    CHECK_F(r[2], 1.0f, 1e-5f);

    // pitch raises the panel on the cylinder and tilts its up vector toward
    // the viewer; the ring keeps its full horizontal spread
    Panel pp = mkPanel(0.0f);
    pp.pitch = 0.5f;
    panelCenter(pp, o0, c, r, up);
    CHECK_F(c[0], 0.0f, 1e-6f);
    CHECK_F(c[1], kPanelY + sinf(0.5f) * kPanelDist, 1e-5f);
    CHECK_F(c[2], -kPanelDist, 1e-6f);
    CHECK(up[1] < 1.0f && up[2] > 0.0f);
    CHECK_F(up[0]*up[0] + up[1]*up[1] + up[2]*up[2], 1.0f, 1e-5f);

    // rayPanel sees through the tilt: aiming at the raised centre lands u=v=0
    float pu, pv;
    float dpc[3] = {c[0], c[1], c[2]};
    CHECK(rayPanel(pp, o0, o0, dpc, &pu, &pv));
    CHECK_F(pu, 0.0f, 1e-4f);
    CHECK_F(pv, 0.0f, 1e-4f);

    // the ring re-anchors to the head's spot: a nonzero origin shifts the
    // whole panel there without changing its local shape
    const float hi[3] = {0.4f, 1.55f, -0.3f};
    panelCenter(p0, hi, c, r, up);
    CHECK_F(c[0], 0.4f, 1e-6f);
    CHECK_F(c[1], 1.55f + kPanelY, 1e-6f);
    CHECK_F(c[2], -0.3f - kPanelDist, 1e-6f);
    CHECK(up[1] > 0.99f);   // plane normal still points back at the anchor

    // an eye displaced from the anchor still picks right: standing 1.5m
    // above the ring and aiming at the panel centre lands u=v=0, where the
    // old origin-started ray would land far off
    {
        const float eye[3] = {0.0f, 1.5f, 0.0f};
        float tc[3], tr[3], tu[3];
        panelCenter(p0, o0, tc, tr, tu);
        const float ddx = tc[0] - eye[0], ddy = tc[1] - eye[1],
                    ddz = tc[2] - eye[2];
        const float dl = sqrtf(ddx*ddx + ddy*ddy + ddz*ddz);
        const float da[3] = {ddx/dl, ddy/dl, ddz/dl};
        float au, av;
        CHECK(rayPanel(p0, o0, eye, da, &au, &av));
        CHECK_F(au, 0.0f, 1e-4f);
        CHECK_F(av, 0.0f, 1e-4f);
        // the same aim started at the world origin lands visibly off centre
        float bu, bv;
        CHECK(rayPanel(p0, o0, o0, da, &bu, &bv));
        CHECK(fabsf(bv) > 0.2f);
    }

    // pickPanel honours both positions: ring anchored at hi, head right on
    // the anchor looking dead ahead picks the centre panel centred
    {
        std::vector<Panel> ps2;
        ps2.push_back(mkPanel(0.0f));
        const float eye2[3] = {0.4f, 1.55f, -0.3f};
        Mat4 I2 = identity();
        Pick pk2 = pickPanel(ps2, I2, hi, eye2);
        CHECK(pk2.idx == 0);
        CHECK_F(pk2.u, 0.0f, 1e-4f);
        CHECK_F(pk2.v, -kPanelY / (kPanelH / 2), 1e-3f);

        // and off the anchor: head half a metre right, aimed at the centre
        const float offEye[3] = {0.9f, 1.55f, -0.3f};
        const float pdx = 0.4f - offEye[0], pdy = 1.55f + kPanelY - offEye[1],
                    pdz = -0.3f - kPanelDist - offEye[2];
        const float pl = sqrtf(pdx*pdx + pdy*pdy + pdz*pdz);
        Mat4 aim2 = identity();
        aim2.m[2] = -pdx / pl; aim2.m[6] = -pdy / pl; aim2.m[10] = -pdz / pl;
        Pick pk3 = pickPanel(ps2, aim2, hi, offEye);
        CHECK(pk3.idx == 0);
        CHECK_F(pk3.u, 0.0f, 1e-4f);
        CHECK_F(pk3.v, 0.0f, 1e-3f);
    }

    // the ring's shared elevation: empty is flat, otherwise the first panel's
    {
        std::vector<Panel> rp;
        CHECK_F(ringPitch(rp), 0.0f, 1e-6f);
        rp.push_back(pp);
        CHECK_F(ringPitch(rp), 0.5f, 1e-6f);
    }

    // slot picking: empty ring -> centre slot; then left, then right
    std::vector<Panel> ps;
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[0], 1e-6f);
    ps.push_back(mkPanel(0.0f));
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[1], 1e-6f);
    ps.push_back(mkPanel(kSlotYaw[1]));
    CHECK_F(freeSlotYaw(ps, 0.0f), kSlotYaw[2], 1e-6f);
    ps.push_back(mkPanel(kSlotYaw[2]));
    // ring full: the answer drops into the widest free arc and must still
    // clear every panel by the minimum gap rather than stack on the centre
    {
        const float fy = freeSlotYaw(ps, 0.0f);
        for (auto& p : ps)
            CHECK(fabsf(wrapPi(p.yaw - fy)) >= kPanelMinGap - 1e-4f);
    }

    // a panel that drifted off the slot grid still blocks the slots it
    // overlaps - the centre moves with the gaze between adoptions, so an
    // exact-slot check lets a new window land on top of it
    ps.clear();
    ps.push_back(mkPanel(0.52f));
    CHECK_F(freeSlotYaw(ps, 0.43f), 0.43f + kSlotYaw[1], 1e-6f);
    ps.push_back(mkPanel(0.43f + kSlotYaw[1]));
    {
        const float fy = freeSlotYaw(ps, 0.43f);
        for (auto& p : ps)
            CHECK(fabsf(wrapPi(p.yaw - fy)) >= kPanelMinGap - 1e-4f);
    }

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

    // the library panel is found by package, exactly once
    CHECK(libraryIndex(ps) == -1);
    ps.push_back(mkPanel(0.0f));
    ps.push_back(mkPanel(kSlotYaw[1], kLibraryPkg));
    CHECK(libraryIndex(ps) == 1);
    ps.push_back(mkPanel(kSlotYaw[2], kLibraryPkg));
    CHECK(libraryIndex(ps) == 1);

    // recenter snaps each panel to its nearest slot around the new centre
    ps.clear();
    ps.push_back(mkPanel(1.02f));   // near the centre slot -> stays
    ps.push_back(mkPanel(1.0f + kSlotYaw[1] * 0.6f));  // nearer left slot
    recenterSlots(ps, 1.0f, 0.0f);
    CHECK_F(ps[0].yaw, 1.0f + kSlotYaw[0], 1e-5f);
    CHECK_F(ps[1].yaw, 1.0f + kSlotYaw[1], 1e-5f);
    // recenter also pulls the ring to the given elevation, clamped
    recenterSlots(ps, 1.0f, 0.4f);
    CHECK_F(ps[0].pitch, 0.4f, 1e-6f);
    recenterSlots(ps, 1.0f, 9.0f);
    CHECK_F(ps[0].pitch, kPitchMax, 1e-6f);

    // two panels nearest the same slot must not collapse onto each other:
    // windows keep their left-to-right order but each takes its own slot
    ps.clear();
    ps.push_back(mkPanel(1.02f));
    ps.push_back(mkPanel(0.95f));
    recenterSlots(ps, 1.0f, 0.0f);
    CHECK(fabsf(wrapPi(ps[0].yaw - ps[1].yaw)) >= kPanelMinGap - 1e-4f);
    CHECK_F(ps[0].yaw, 1.0f + kSlotYaw[0], 1e-5f);
    CHECK_F(ps[1].yaw, 1.0f + kSlotYaw[1], 1e-5f);

    // a lone panel lands dead ahead, not pushed onto a side slot
    ps.clear();
    ps.push_back(mkPanel(1.5f));
    recenterSlots(ps, 1.0f, 0.0f);
    CHECK_F(ps[0].yaw, 1.0f, 1e-5f);

    // gaze pick: identity head looks down -z, hits the centre panel
    Mat4 I = identity();
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    Pick pk = pickPanel(ps, I, o0, o0);
    CHECK(pk.idx == 0);
    CHECK_F(pk.u, 0.0f, 1e-5f);
    CHECK_F(pk.v, -kPanelY / (kPanelH / 2), 1e-4f);  // centre sits slightly up

    // panel off-axis is missed when looking straight ahead
    ps.clear();
    ps.push_back(mkPanel(kSlotYaw[1]));
    pk = pickPanel(ps, I, o0, o0);
    CHECK(pk.idx == -1);

    // head yawed to the left slot: quat about +Y is a view-space yaw
    const float a = kSlotYaw[1];   // turn toward the panel
    Mat4 v = quatToMat((const float[]){0, sinf(a / 2), 0, cosf(a / 2)},
                       false);
    pk = pickPanel(ps, v, o0, o0);
    CHECK(pk.idx == 0);
    CHECK_F(pk.u, 0.0f, 1e-3f);

    // two panels both partly under the ray: nearest wins
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    ps.push_back(mkPanel(0.05f));
    pk = pickPanel(ps, I, o0, o0);
    CHECK(pk.idx >= 0);

    // dragPoint: gaze centred on the panel -> display centre-ish coords
    ps.clear();
    ps.push_back(mkPanel(0.0f));
    float dx, dy;
    CHECK(dragPoint(ps[0], I, o0, o0, &dx, &dy));
    CHECK_F(dx, kVdW * 0.5f, 1e-3f);
    // the tilted plane shifts the hit a hair vs the flat kPanelY estimate
    CHECK_F(dy, (0.5f + kPanelY / kPanelH) * kVdH, 0.1f);

    // gaze yawed onto a side panel: drag point lands on its centre column
    Panel pr2 = mkPanel(kSlotYaw[1]);
    Mat4 v2 = quatToMat((const float[]){0, sinf(kSlotYaw[1] / 2), 0,
                        cosf(kSlotYaw[1] / 2)}, false);
    CHECK(dragPoint(pr2, v2, o0, o0, &dx, &dy));
    CHECK_F(dx, kVdW * 0.5f, 1e-3f);

    // gaze way past the panel edge clamps inside the window, not outside
    Mat4 far = quatToMat((const float[]){0, sinf(-0.30f), 0,
                         cosf(-0.30f)}, false);
    CHECK(dragPoint(ps[0], far, o0, o0, &dx, &dy));
    CHECK_F(dx, 0.0f, 1e-3f);

    // facing away entirely: the ray can't reach the plane
    Mat4 back = quatToMat((const float[]){0, 1.0f, 0, 0}, false);
    CHECK(!dragPoint(ps[0], back, o0, o0, &dx, &dy));

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
    CHECK(rayPanel(ps[0], o0, o0, fwd, &ru, &rv, &rt));
    CHECK_F(ru, 0.0f, 1e-5f);
    // the kPanelY lift tilts the plane, so the ray lands a touch past the
    // ring distance: t = |c|^2 / kPanelDist
    CHECK_F(rt, kPanelDist + kPanelY * kPanelY / kPanelDist, 1e-4f);
    float away[3] = {0, 0, 1};
    CHECK(!rayPanel(ps[0], o0, o0, away, &ru, &rv, &rt));

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
    pk = pickPanel(ps, aim, o0, o0);   // dead centre of the pill: the label
    CHECK(pk.idx == 0 && pk.zone == ZONE_LABEL);
    aim.m[2] = -pillCloseX(phw);   // right end: the close button
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == 0 && pk.zone == ZONE_CLOSE);
    aim.m[2] = -pillMinX(phw);     // next to it: minimize
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == 0 && pk.zone == ZONE_MIN);
    aim.m[6] = 0.60f;              // way under the pill: nothing
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == -1 && pk.zone == ZONE_NONE);
    // centred under the pill: the drag handle
    const float handleY = kPanelY - handleDrop();
    aim.m[2] = -0.0f; aim.m[6] = -handleY;
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == 0 && pk.zone == ZONE_HANDLE);
    aim.m[2] = -(kHandleW + kHandlePad + 0.03f);   // past the line: nothing
    pk = pickPanel(ps, aim, o0, o0);
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
    pk = pickPanel(ps, side, o0, o0);
    CHECK(pk.idx == -1 && pk.zone == ZONE_NONE);

    grabRing(ps);
    dragRing(ps, 0.30f, 0.0f);
    CHECK_F(ps[0].yaw, kSlotYaw[0] + 0.30f, 1e-6f);
    CHECK_F(ps[1].yaw, kSlotYaw[1] + 0.30f, 1e-6f);
    CHECK_F(ps[2].yaw, kSlotYaw[2] + 0.30f, 1e-6f);
    // the next tick re-applies the snapshot, not accumulated yaw
    dragRing(ps, -0.10f, 0.0f);
    CHECK_F(ps[0].yaw, -0.10f, 1e-6f);
    CHECK_F(ps[2].yaw, kSlotYaw[2] - 0.10f, 1e-6f);
    // pitch delta elevates the whole ring together, clamped at the poles
    dragRing(ps, 0.0f, 0.35f);
    CHECK_F(ps[0].pitch, 0.35f, 1e-6f);
    CHECK_F(ps[1].pitch, 0.35f, 1e-6f);
    CHECK_F(ps[2].pitch, 0.35f, 1e-6f);
    dragRing(ps, 0.0f, 9.0f);
    CHECK_F(ps[0].pitch, kPitchMax, 1e-6f);
    dragRing(ps, 0.0f, -9.0f);
    CHECK_F(ps[0].pitch, -kPitchMax, 1e-6f);
    // delta wraps across +-pi instead of throwing panels off the ring
    ps.clear();
    ps.push_back(mkPanel((float)M_PI - 0.05f));
    grabRing(ps);
    dragRing(ps, 0.20f, 0.0f);
    CHECK_F(ps[0].yaw, -(float)M_PI + 0.15f, 1e-5f);

    // the library pill has no buttons: its whole band picks as label
    ps.clear();
    ps.push_back(mkPanel(0.0f, kLibraryPkg));
    aim.m[2] = -pillCloseX(kBarH); aim.m[6] = -pillY;
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == 0 && pk.zone == ZONE_LABEL);

    // minimized windows vanish from picking and stay findable for restore
    ps[0].minimized = true;
    pk = pickPanel(ps, aim, o0, o0);
    CHECK(pk.idx == -1);
    CHECK(minimizedIndex(ps, kLibraryPkg) == 0);
    CHECK(minimizedIndex(ps, "com.x.app") == -1);
    ps[0].minimized = false;
    CHECK(minimizedIndex(ps, kLibraryPkg) == -1);
}
