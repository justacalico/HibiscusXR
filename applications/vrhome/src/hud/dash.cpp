#include "dash.h"

#include "engine.h"

#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../grid/grid.h"
#include "../grid/layout.h"
#include "../input/ctrl.h"
#include "../input/input.h"
#include "../input/pick.h"
#include "../kbd/kbd.h"
#include "../notif/layout.h"
#include "../notif/notif.h"
#include "../notif/shade.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../pill/pill.h"
#include "../quick/layout.h"
#include "../quick/quick.h"
#include "../set/set.h"
#include "../set/layout.h"
#include "../render/chrome.h"
#include "../render/ctrl_render.h"
#include "../sysmsg/layout.h"
#include "../sysmsg/sysmsg.h"

#include <cstring>
#include <ctime>

static long long epochNowMs() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void anchorDash(HudEngine* e, const Mat4& head) {
    if (!e->backend || !e->haveQuat || e->dockAnchored) return;
    e->dockAnchored = true;
    float gy = 0.0f, gp = 0.0f;
    recenterAngles(head, &gy, &gp);
    memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
    e->dockYaw = gy;
    e->dockPitch = dockPitchFor(gp);
}

void drawDashScene(Engine* e, const Mat4& vp) {
    HudEngine* h = (HudEngine*)e;
    float mYaw, mPitch, mLift;
    sysMsgAnchor(h->sysMsgOnly, h->sysMsgYaw, h->dockYaw, h->dockPitch,
                 &mYaw, &mPitch, &mLift);
    if (sysMsgModal(h->sysMsgOnly, h->sysMsgs)) {
        if (h->toastOnly)
            drawNotifStack(h, vp, h->toastYaw, kNotifToastPitch, 0.0f);
        drawSysMsg(h, vp, mYaw, mPitch, mLift);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->toastOnly) {
        drawNotifStack(h, vp, h->toastYaw, kNotifToastPitch, 0.0f);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->kbd.only) {
        drawKbd(h, vp);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->debugOnly) {
        drawHoldRing(h);
        return;
    }
    drawPanels(h, vp);
    // the dash is an overlay: the windows get their own depth pass, then
    // everything shell-side draws on top so a window swung in close can't
    // bury the strip, the shelf, the grid or a notification under itself
    glClear(GL_DEPTH_BUFFER_BIT);
    drawKbd(h, vp);
    drawGrid(h, vp);
    drawQuick(h, vp);
    drawSet(h, vp);
    drawShade(h, vp);
    drawDock(h, vp);
    drawShelf(h, vp);
    drawNotifStack(h, vp, h->dockYaw, h->dockPitch,
                   notifLiftAbove((int)h->notifs.size(),
                                  h->shelf.empty() ? kDockBarH * 0.5f
                                                   : shelfTop()));
    drawControllers(h, vp);
    drawCursor(h, vp);
    drawHoldRing(h);
}

static void pickDash(HudEngine* e, const Mat4& head, long long now) {
    const float nYaw = e->toastOnly ? e->toastYaw : e->dockYaw;
    const float nPitch = e->toastOnly ? kNotifToastPitch : e->dockPitch;
    const float nClear = e->shelf.empty() ? kDockBarH * 0.5f : shelfTop();
    const float nLift = e->toastOnly ? 0.0f
                                     : notifLiftAbove((int)e->notifs.size(), nClear);
    float mYaw, mPitch, mLift;
    sysMsgAnchor(e->sysMsgOnly, e->sysMsgYaw, e->dockYaw, e->dockPitch,
                 &mYaw, &mPitch, &mLift);
    const bool msgModal = sysMsgModal(e->sysMsgOnly, e->sysMsgs);
    const SysMsgPick mp = pickSysMsgRay(e->sysMsgs, mYaw, mPitch, mLift,
                                      e->dashDist, e->ringPos, e->aimO,
                                      e->aimD);
    const Pick pk = pickPanelRay(e->panels, e->ringPos, e->aimO, e->aimD);
    float kU = 0.0f, kV = 0.0f, kT = -1.0f;
    int kZone = KZONE_KEY;
    if (e->kbd.shown && e->kbd.displayId >= 0) {
        float kc[3], kr[3], kup[3];
        kbdFrame(e->panels, e->kbd.hostDisp, e->kbd.only,
                 e->kbd.only ? e->kbd.yaw : e->dockYaw,
                 e->kbd.offYaw, e->kbd.offY, e->ringPos, kc, kr, kup);
        if (rayQuad(kc, kr, kup, e->ringPos, e->aimO, e->aimD,
                    kKbdHW, kKbdHH, &kU, &kV, &kT)) {
            if (onMovePill(kU, kV, kKbdHW, kKbdHH)) kZone = KZONE_HANDLE;
            else if (kU < -1.0f || kU > 1.0f || kV < -1.0f || kV > 1.0f)
                kT = -1.0f;
        }
    }
    e->kbd.hover = false;
    e->kbd.zone = KZONE_KEY;
    const float dDrop = dashDrop(progT(e->summonMs, now, kDashMs));
    const DockPick dp = pickDockRay(e->dock, e->dockSys, e->dockHW,
                                    e->dockYaw, e->dockPitch, dDrop,
                                    e->dashDist, e->ringPos, e->aimO,
                                    e->aimD);
    const ShelfPick sp = pickShelfRay(e->shelf, e->shelfHW, e->dockYaw,
                                      e->dockPitch, dDrop, e->dashDist,
                                      e->ringPos, e->aimO, e->aimD);
    const NotifPick np = pickNotifRay(e->notifs, nYaw, nPitch, nLift,
                                      e->dashDist, e->ringPos, e->aimO,
                                      e->aimD);
    const GridPick gp = e->grid.shown
        ? pickGridRay(e->grid, e->dockYaw, ringPitchFor(e->dockPitch),
                      e->dashDist, e->ringPos, e->aimO, e->aimD)
        : GridPick{};
    const QsPick qp = e->quick.shown
        ? pickQuickRay(e->dockYaw, ringPitchFor(e->dockPitch),
                       e->dashDist, e->ringPos, e->aimO, e->aimD)
        : QsPick{};
    const SetHit s5 = e->set.shown
        ? pickSetRay(e->set, e->dockYaw, ringPitchFor(e->dockPitch),
                     e->dashDist, e->ringPos, e->aimO, e->aimD)
        : SetHit{};
    const ShadePick hp = e->shade.shown
        ? pickShadeRay(e->notifsAll, e->shade.scroll, e->shade.clearHW,
                       e->dockYaw, ringPitchFor(e->dockPitch),
                       e->dashDist, e->ringPos, e->aimO, e->aimD)
        : ShadePick{};
    e->grid.hover = -1;
    e->grid.zone = GZONE_NONE;
    e->quick.hover = -1;
    e->quick.zone = QZONE_NONE;
    e->set.hover = -1;
    e->set.zone = SZONE_NONE;
    e->set.aux = -1;
    e->shade.hover = -1;
    e->shade.zone = SHZONE_NONE;
    e->quick.slider = -1;
    e->sysMsgHover = -1; e->sysMsgZone = MZONE_NONE; e->sysMsgBtn = -1;
    e->notifHover = -1; e->notifZone = NZONE_NONE;
    e->shelfHover = -1;
    e->dockHover = -1; e->dockZone = DZONE_NONE;
    e->hover = -1; e->hoverZone = ZONE_NONE;
    e->aimHitT = -1.0f;
    switch (dashSel(msgModal, mp, np, gp, qp, s5, hp, sp, dp, kT)) {
    case DSEL_SYSMSG:
        e->sysMsgHover = mp.hit ? 0 : -1;
        e->sysMsgZone = mp.zone;
        e->sysMsgBtn = mp.btn;
        e->aimHitT = mp.hit ? mp.t : -1.0f;
        break;
    case DSEL_NOTIF:
        e->notifHover = np.idx; e->notifZone = np.zone;
        e->aimHitT = np.t;
        break;
    case DSEL_GRID:
        e->grid.hover = gp.idx; e->grid.zone = gp.zone;
        e->grid.u = gp.u; e->grid.v = gp.v;
        e->aimHitT = gp.t;
        break;
    case DSEL_QUICK:
        e->quick.hover = qp.idx; e->quick.zone = qp.zone;
        e->quick.slider = qp.slider;
        e->quick.u = qp.u; e->quick.v = qp.v;
        e->aimHitT = qp.t;
        break;
    case DSEL_SET:
        e->set.hover = s5.idx; e->set.zone = s5.zone;
        e->set.aux = s5.aux;
        e->set.u = s5.u; e->set.v = s5.v;
        e->aimHitT = s5.t;
        break;
    case DSEL_SHADE:
        e->shade.hover = hp.idx; e->shade.zone = hp.zone;
        e->shade.u = hp.u; e->shade.v = hp.v;
        e->aimHitT = hp.t;
        break;
    case DSEL_SHELF:
        e->shelfHover = sp.idx;
        e->aimHitT = sp.t;
        break;
    case DSEL_DOCK:
        e->dockHover = dp.idx; e->dockZone = dp.zone;
        e->dockU = dp.u; e->dockV = dp.v;
        e->aimHitT = dp.t;
        break;
    case DSEL_KBD:
        e->kbd.hover = true; e->kbd.zone = kZone;
        e->kbd.u = kU; e->kbd.v = kV;
        e->aimHitT = kT;
        break;
    default:
        e->hover = pk.idx;
        e->hoverZone = pk.idx >= 0 ? pk.zone : ZONE_NONE;
        e->aimHitT = pk.idx >= 0 ? pk.t : -1.0f;
        if (pk.idx >= 0) {
            const Panel& hp = e->panels[pk.idx];
            e->hitU = pk.u; e->hitV = pk.v;
            e->hitX = (pk.u * 0.5f + 0.5f) * hp.dispW;
            e->hitY = (0.5f - pk.v * 0.5f) * hp.dispH;
        }
        break;
    }
    float gy;
    if (gazeYaw(head, &gy)) e->gazeYaw = gy;
    e->gazePitch = gazePitch(head);
    if (e->toastOnly && !e->toastWas) e->toastYaw = e->gazeYaw;
    e->toastWas = e->toastOnly;
    if (e->sysMsgOnly && !e->sysMsgWas) e->sysMsgYaw = e->gazeYaw;
    e->sysMsgWas = e->sysMsgOnly;
    if (e->kbd.only && !e->kbd.was) e->kbd.yaw = e->gazeYaw;
    e->kbd.was = e->kbd.only;
}

void tickDash(HudEngine* e, const Mat4& head, long long now) {
    e->frameMs = now;
    anchorDash(e, head);
    if (e->holdStartMs > 0) {
        const float p = (float)(now - e->holdStartMs) / (float)kHoldMs;
        e->holdP = p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p;
    } else {
        e->holdP = 0.0f;
    }
    if (takeWantRecenter()) {
        if (e->backend) e->backend->signalRecenter(now);
        memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
        float yaw, pitch;
        recenterAngles(head, &yaw, &pitch);
        const float prevCentre = e->dockYaw;
        e->dockYaw = yaw;
        e->dockPitch = dockPitchFor(pitch);
        recenterSlots(e->panels, yaw, ringPitchFor(e->dockPitch), prevCentre);
        e->dockAnchored = true;
    }
    pumpBridge(e);
    syncUiStrings(e);
    syncDock(e);
    syncGrid(e);
    syncNotifs(e);
    syncSysMsgs(e);
    syncQuick(e);
    syncSet(e);
    syncShade(e);
    e->notifs = visibleNotifs(e->notifsAll, epochNowMs());
    pickDash(e, head, now);
    ctrlFlush(e);
    dragTick(e, e->aimO, e->aimD);
    moveTick(e);
    setDragTick(e);
    dockTick(e);
    const float dtMs = e->animMs ? (float)(now - e->animMs) : 0.0f;
    e->animMs = now;
    if (dtMs > 0.0f) {
        tickPanels(e->panels, dtMs);
        tickDockHover(e->dock, e->dockHover, dtMs);
        tickShelfHover(e->shelf, e->shelfHover, dtMs);
        // hover on a popup row or pill isn't a cell: keep the cell the
        // press started on from magnifying under the menu
        tickGridHover(e->grid.items,
            e->grid.zone == GZONE_ITEM || e->grid.zone == GZONE_DOTS
                ? e->grid.hover : -1, dtMs);
        e->grid.openT = stepT(e->grid.openT, e->grid.shown, dtMs, kGridMs);
        for (int i = 0; i < QS_COUNT; ++i)
            e->quick.hs[i] = dampMs(e->quick.hs[i],
                i == e->quick.hover ? kQuickHoverScale : 1.0f,
                dtMs, kHoverTauMs);
        e->quick.openT = stepT(e->quick.openT, e->quick.shown, dtMs,
                               kQuickMs);
        e->set.openT = stepT(e->set.openT, e->set.shown, dtMs, kSetMs);
        e->shade.openT = stepT(e->shade.openT, e->shade.shown, dtMs,
                               kShadeMs);
        // released scroll drags coast on their tracked speed; a held one
        // belongs to the drag ticks above and stays put here
        if (!e->grid.scrollHeld)
            e->grid.scroll = flingScroll(e->grid.scroll,
                &e->grid.scrollVel, dtMs,
                gridScrollMax((int)e->grid.items.size()));
        if (!e->shade.scrollHeld)
            e->shade.scroll = flingScroll(e->shade.scroll,
                &e->shade.scrollVel, dtMs,
                shadeScrollMax((int)e->notifsAll.size()));
        if (!e->set.scrollHeld)
            e->set.scroll = flingScroll(e->set.scroll, &e->set.scrollVel,
                                        dtMs, setScrollMax(e->set));
    }
    updatePanels(e);
    kbdTick(e);
}
