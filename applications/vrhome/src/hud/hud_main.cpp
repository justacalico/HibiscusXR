// The HUD: panels, the app-grid overlay and the summonable menu, drawn
// into a fullscreen TYPE_SYSTEM_OVERLAY window owned by HudService.
// Virtual displays live in this process (ShellBridge is constructed on
// the java side), so every panel keeps running no matter which app owns
// the physical display underneath.
//
// Threading: HudService runs on the main thread and feeds this file through
// JNI - surface post/gone, key presses. One pthread runs the render loop and
// owns the sensor queue; all GL and bridge calls happen on it.

#include "engine.h"
#include "debug_hooks.h"
#include "status.h"
#include "quiet.h"

#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../grid/grid.h"
#include "../grid/layout.h"
#include "../notif/notif.h"
#include "../notif/layout.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../input/input.h"
#include "../input/ctrl.h"
#include "../input/ctrl_debug.h"
#include "../kbd/kbd.h"
#include "../math/head.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../pill/pill.h"
#include "../render/chrome.h"
#include "../render/ctrl_render.h"
#include "../render/egl.h"
#include "../render/frame.h"
#include "../render/warp.h"
#include "../sensor/sensor.h"
#include "../sensor/qvr.h"
#include "../sysmsg/layout.h"
#include "../sysmsg/sysmsg.h"

#include <android/looper.h>
#include <android/native_window.h>
#include <android/sensor.h>
#include <android/native_window_jni.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <pthread.h>

// postMs arrives on the wall-clock epoch, so expiry compares against
// CLOCK_REALTIME, not the monotonic clock the rest of the engine uses
static long long epochNowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static HudEngine gHud;
static pthread_t gThread;
static bool gStarted = false;

// the dash's first anchor: once tracking is live the dock claims the head's
// heading as its centre yaw, so the window slots and the app grid open dead
// ahead instead of wherever world yaw zero happened to leave them
static void anchorDash(HudEngine* e, const Mat4& head) {
    if (!e->bridge || !e->haveQuat || e->dockAnchored) return;
    e->dockAnchored = true;
    float gy = 0.0f, gp = 0.0f;
    recenterAngles(head, &gy, &gp);
    memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
    e->dockYaw = gy;
    e->dockPitch = dockPitchFor(gp);
}

// the HUD's world is just the chrome: panels plus the gaze cursor, over
// whatever surface sits underneath (env scenery or a running app)
static void hudScene(Engine* e, const Mat4& vp) {
    HudEngine* h = (HudEngine*)e;
    float mYaw, mPitch, mLift;
    sysMsgAnchor(h->sysMsgOnly, h->sysMsgYaw, h->dockYaw, h->dockPitch,
                 &mYaw, &mPitch, &mLift);
    if (sysMsgModal(h->sysMsgOnly, h->sysMsgs)) {
        // a crash/ANR card is modal wherever the window is up: the dialog
        // draws alone (plus the toast stack when one's up over a covered
        // app) until it's clicked away - the whole dash sits out
        if (h->toastOnly)
            drawNotifStack(h, vp, h->toastYaw, kNotifToastPitch, 0.0f);
        drawSysMsg(h, vp, mYaw, mPitch, mLift);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->toastOnly) {
        // heads-up over a covered app: only the card stack and the cursor
        // draw - the window never takes focus and no menu chrome shows,
        // so the app underneath keeps running undisturbed
        drawNotifStack(h, vp, h->toastYaw, kNotifToastPitch, 0.0f);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->kbd.only) {
        // the keyboard over a covered app: the quad draws alone, anchored
        // ahead of the gaze when it popped, until the IME hides again
        drawKbd(h, vp);
        drawCursor(h, vp);
        drawHoldRing(h);
        return;
    }
    if (h->debugOnly) {
        // the window is up over a covered app only for the status line:
        // the text itself draws after the scene, so the scene carries
        // just the hold ring for summon-key feedback
        drawHoldRing(h);
        return;
    }
    drawPanels(h, vp);
    drawKbd(h, vp);
    // the app grid hangs in front of the window slots: it draws over the
    // panels it covers like the system overlay it is
    drawGrid(h, vp);
    drawDock(h, vp);
    drawShelf(h, vp);
    drawNotifStack(h, vp, h->dockYaw, h->dockPitch,
                   notifLiftAbove((int)h->notifs.size(),
                                  h->shelf.empty() ? kDockBarH * 0.5f
                                                   : shelfTop()));
    drawControllers(h, vp);
    drawCursor(h, vp);
    // the hold ring is a flat overlay: it draws on top of the live scene and
    // ignores vp entirely, so it stays put while the world shifts around it
    drawHoldRing(h);
}

// gathers the live state into the POD the pure rule consumes; pose deltas
// are read off prevAim, which is refreshed on every call - busy or not
static void hudQuietSample(HudEngine* e, HudQuietIn* q) {
    *q = {};
    q->panels = !e->panels.empty();
    q->grid = e->grid.openT > 0.0f || e->grid.shown;
    q->notifs = !e->notifs.empty();
    q->sysmsgs = !e->sysMsgs.empty();
    q->toastOnly = e->toastOnly;
    q->sysMsgOnly = e->sysMsgOnly;
    q->debugOnly = e->debugOnly;
    q->kbd = e->kbd.only || e->kbd.shown || e->kbd.pressed || e->kbd.moveHeld;
    q->holdRing = e->holdStartMs != 0 || e->holdP > 0.0f;
    q->held = e->confirmHeld || e->moveHeld || e->sysPress;
    q->presses = e->dockPress >= 0 || e->shelfPress >= 0 ||
                 e->notifPress >= 0 || e->sysMsgPress >= 0 ||
                 e->grid.press >= 0 || e->grid.scrollHeld ||
                 e->pressDisp >= 0 || e->dragDisp >= 0 ||
                 e->dockPinP > 0.0f;
    q->recentSummon = e->frameMs - e->summonMs < 1500;
    {
        std::lock_guard<std::mutex> lk(e->keyMu);
        q->inputPending = !e->ctrlEv.empty() || !e->keyQ.empty();
    }
    const float dx = e->aimO[0] - e->prevAimO[0];
    const float dy = e->aimO[1] - e->prevAimO[1];
    const float dz = e->aimO[2] - e->prevAimO[2];
    const float ux = e->aimD[0] - e->prevAimD[0];
    const float uy = e->aimD[1] - e->prevAimD[1];
    const float uz = e->aimD[2] - e->prevAimD[2];
    q->aimMoved = dx * dx + dy * dy + dz * dz > 0.0001f ||
                  ux * ux + uy * uy + uz * uz > 0.0025f;
    memcpy(e->prevAimO, e->aimO, sizeof(e->prevAimO));
    memcpy(e->prevAimD, e->aimD, sizeof(e->prevAimD));
}

static void hudFrame(HudEngine* e) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000 +
                          ts.tv_nsec / 1000000;
    e->frameMs = now;

    // surface handoff from the SurfaceHolder callbacks
    ANativeWindow* nw = e->window.exchange(nullptr);
    if (nw) {
        // a fresh surface after being hidden is a summon: the strip plays
        // its slide/fade-in from this stamp
        const bool wasReady = e->ready;
        if (e->ready) termWindow(e);
        if (initWindow(e, nw) != 0) LOGE("initWindow failed");
        else if (!wasReady) e->summonMs = now;
        ANativeWindow_release(nw);
    }
    if (e->windowGone.exchange(false)) termWindow(e);

    if (e->context == EGL_NO_CONTEXT) { usleep(50000); return; }

    // rotation-vector samples land on this thread's looper; drain them into
    // e->quat the same way the env does
    int ident, events;
    void* data;
    while ((ident = ALooper_pollOnce(0, nullptr, &events, &data)) >= 0) {
        if (ident == kSensorIdent) drainSensor(e);
    }

    // queued key presses from the window
    for (;;) {
        KeyIn k;
        {
            std::lock_guard<std::mutex> l(e->keyMu);
            if (e->keyQ.empty()) break;
            k = e->keyQ.front(); e->keyQ.pop_front();
        }
        hudKey(e, k.code, k.action, k.repeat);
    }

    // dev hook: force the app grid open (1) or shut (0) for headless
    // testing; unset (-1) leaves the state to the UI
    const int gprop = propI("debug.vrhome.grid", -1);
    if (gprop == 1) e->grid.shown = true;
    else if (gprop == 0) e->grid.shown = false;

    qvrPoll(e);
    const bool useSensor = propI("debug.vrhome.sensor", 1) && e->haveQuat;
    smoothPose(e, useSensor);
    const float fakePos[3] = {propF("debug.vrhome.fpx", 0.0f),
                              propF("debug.vrhome.fpy", 0.0f),
                              propF("debug.vrhome.fpz", 0.0f)};
    const float sensRoll = e->quatFromQvr ? propF("debug.vrhome.qvrsensroll", kQvrSensRoll)
                                          : propF("debug.vrhome.sensroll", kSensRoll);
    const float worldX = e->quatFromQvr ? propF("debug.vrhome.qvrworldx", kQvrWorldX)
                                        : propF("debug.vrhome.worldx", kWorldX);
    const float* headPos = nullptr;
    float posGl[3];
    if (useSensor && e->headPosValid) {
        sensorPosToWorld(e->headPos, sensRoll, worldX, posGl);
        headPos = posGl;
    }
    if (useSensor && (fakePos[0] || fakePos[1] || fakePos[2])) {
        memcpy(e->headPos, fakePos, sizeof(fakePos));
        e->headPosValid = true;
        headPos = fakePos;
    }
    // the gaze ray starts where the head actually is: the mapped position
    // that went into the view matrix, or the world origin when position is
    // out - the pick must see the same eye the render does
    if (headPos) memcpy(e->eyePos, headPos, sizeof(e->eyePos));
    else memset(e->eyePos, 0, sizeof(e->eyePos));
    const Mat4 head = headMatrix(e->quat, propI("debug.vrhome.tq", 1) != 0,
        sensRoll, worldX,
        propF("debug.vrhome.roll",     kRoll), useSensor, headPos);

    ctrlTick(e, head, sensRoll, worldX, propF("debug.vrhome.roll", kRoll),
             now);

    runDebugHooks(e);
    anchorDash(e, head);

    // hold-to-recenter fill: the java side owns the threshold and fires the
    // recenter; native just turns the held time into the ring's 0..1
    if (e->holdStartMs > 0) {
        const float p = (float)(now - e->holdStartMs) / (float)kHoldMs;
        e->holdP = p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p;
    } else {
        e->holdP = 0.0f;
    }

    if (takeWantRecenter()) {
        // the dash re-anchors to where the head is right now: docked panels
        // snap back onto their slots around the strip's new yaw and the
        // ring's elevation follows the strip's; floating windows keep their
        // offsets and ride along
        memcpy(e->ringPos, e->eyePos, sizeof(e->ringPos));
        float yaw, pitch;
        recenterAngles(head, &yaw, &pitch);
        const float prevCentre = e->dockYaw;
        e->dockYaw = yaw;
        e->dockPitch = dockPitchFor(pitch);
        recenterSlots(e->panels, yaw, ringPitchFor(e->dockPitch),
                      prevCentre);
        e->dockAnchored = true;
    }
    pumpBridge(e);

    // sync first: the pick needs this frame's item list and strip width
    syncUiStrings(e);
    syncDock(e);
    syncGrid(e);
    syncNotifs(e);
    syncSysMsgs(e);
    // cards age out of the dash on postMs + kNotifShowMs; the record
    // itself stays live for the shade
    e->notifs = visibleNotifs(e->notifsAll, epochNowMs());

    // the card stack anchors over the dock bar in the dash, or on the
    // toast's own yaw while heads-up over an app; the yaw is captured when
    // the toast pops so the cards stay world-locked for its seconds
    const float nYaw = e->toastOnly ? e->toastYaw : e->dockYaw;
    const float nPitch = e->toastOnly ? kNotifToastPitch : e->dockPitch;
    const float nClear = e->shelf.empty() ? kDockBarH * 0.5f : shelfTop();
    const float nLift = e->toastOnly ? 0.0f
                                     : notifLiftAbove((int)e->notifs.size(),
                                                      nClear);

    // the system message mirrors the stack's anchor rule: its own yaw
    // over a covered app, the dash's centre in home space
    float mYaw, mPitch, mLift;
    sysMsgAnchor(e->sysMsgOnly, e->sysMsgYaw, e->dockYaw, e->dockPitch,
                 &mYaw, &mPitch, &mLift);
    const bool msgModal = sysMsgModal(e->sysMsgOnly, e->sysMsgs);

    // aim pick: the card stack floats in front of the dock plane so it
    // wins by distance; the dock wins ties against a panel edge so its
    // icons stay tappable even when one peeks out from behind a window.
    // A system message beats everything - it's modal
    const SysMsgPick mp = pickSysMsgRay(e->sysMsgs, mYaw, mPitch, mLift,
                                        e->ringPos, e->aimO, e->aimD);
    const Pick pk = pickPanelRay(e->panels, e->ringPos, e->aimO, e->aimD);
    // the keyboard quad hangs under its host panel (or free in the dash's
    // centre when the field lives in a covered app); the ray test shares
    // the draw path's frame so the pick always matches what is on screen.
    // The pill under the quad is part of the hit area even though it sits
    // outside the quad's v bounds
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
    // the strip's summon slide counts for the picks too: while the fade-in
    // runs the plane sits `dDrop` lower, and the aim must land on what's
    // drawn, not where the strip will rest
    const float dDrop = dashDrop(progT(e->summonMs, now, kDashMs));
    const DockPick dp = pickDockRay(e->dock, e->dockSys, e->dockHW,
                                    e->dockYaw, e->dockPitch, dDrop,
                                    e->ringPos, e->aimO, e->aimD);
    const ShelfPick sp = pickShelfRay(e->shelf, e->shelfHW, e->dockYaw,
                                      e->dockPitch, dDrop, e->ringPos,
                                      e->aimO, e->aimD);
    const NotifPick np = pickNotifRay(e->notifs, nYaw, nPitch, nLift,
                                      e->ringPos, e->aimO, e->aimD);
    const GridPick gp = e->grid.shown
        ? pickGridRay((int)e->grid.items.size(), e->grid.scroll,
                      e->dockYaw, ringPitchFor(e->dockPitch),
                      e->ringPos, e->aimO, e->aimD)
        : GridPick{};
    e->grid.hover = -1;
    e->grid.zone = GZONE_NONE;
    if (msgModal) {
        // the card owns the scene until it's dismissed: nothing hidden
        // can hover or take a press, the pick only lights its own zones
        e->sysMsgHover = mp.hit ? 0 : -1;
        e->sysMsgZone = mp.zone;
        e->sysMsgBtn = mp.btn;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = -1;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = mp.hit ? mp.t : -1.0f;
    } else if (mp.hit) {
        e->sysMsgHover = 0;
        e->sysMsgZone = mp.zone;
        e->sysMsgBtn = mp.btn;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = -1;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = mp.t;
    } else if (np.stack && (!dp.bar || np.t <= dp.t) &&
            (pk.idx < 0 || np.t <= pk.t) && (kT < 0.0f || np.t <= kT)) {
        e->sysMsgHover = -1;
        e->sysMsgZone = MZONE_NONE;
        e->sysMsgBtn = -1;
        e->notifHover = np.idx;
        e->notifZone = np.zone;
        e->shelfHover = -1;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = np.t;
    // the app grid hangs in front of the window plane but behind the dock:
    // a hit on its card wins over panels and loses to anything nearer by t
    } else if (gp.hit && (pk.idx < 0 || gp.t <= pk.t) &&
            (kT < 0.0f || gp.t <= kT) && (!dp.bar || gp.t <= dp.t) &&
            (!sp.hit || gp.t <= sp.t)) {
        e->sysMsgHover = -1;
        e->sysMsgZone = MZONE_NONE;
        e->sysMsgBtn = -1;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = -1;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        e->kbd.hover = false;
        e->grid.hover = gp.idx;
        e->grid.zone = gp.zone;
        e->grid.u = gp.u;
        e->grid.v = gp.v;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = gp.t;
    } else if (sp.hit && (pk.idx < 0 || sp.t <= pk.t) &&
            (kT < 0.0f || sp.t <= kT)) {
        e->sysMsgHover = -1;
        e->sysMsgZone = MZONE_NONE;
        e->sysMsgBtn = -1;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = sp.idx;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = sp.t;
    // the quad rides nearer than the dock and shelf, so it wins the aim
    // whenever they overlap on screen
    } else if (dp.bar && (pk.idx < 0 || dp.t <= pk.t) &&
            (kT < 0.0f || dp.t <= kT)) {
        e->sysMsgHover = -1;
        e->sysMsgZone = MZONE_NONE;
        e->sysMsgBtn = -1;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = -1;
        e->dockHover = dp.idx;
        e->dockZone = dp.zone;
        e->dockU = dp.u;
        e->dockV = dp.v;
        e->hover = -1;
        e->hoverZone = ZONE_NONE;
        e->aimHitT = dp.t;
    } else {
        e->sysMsgHover = -1;
        e->sysMsgZone = MZONE_NONE;
        e->sysMsgBtn = -1;
        e->notifHover = -1;
        e->notifZone = NZONE_NONE;
        e->shelfHover = -1;
        e->dockHover = -1;
        e->dockZone = DZONE_NONE;
        if (kT >= 0.0f && (pk.idx < 0 || kT <= pk.t)) {
            // the keyboard is the nearest surface: no panel hover at all,
            // the hit belongs to the quad or its pill
            e->kbd.hover = true;
            e->kbd.zone = kZone;
            e->kbd.u = kU;
            e->kbd.v = kV;
            e->hover = -1;
            e->hoverZone = ZONE_NONE;
            e->aimHitT = kT;
        } else {
            e->hover = pk.idx;
            e->hoverZone = pk.idx >= 0 ? pk.zone : ZONE_NONE;
            e->aimHitT = pk.idx >= 0 ? pk.t : -1.0f;
            if (pk.idx >= 0) {
                e->hitU = pk.u;
                e->hitV = pk.v;
                e->hitX = (pk.u * 0.5f + 0.5f) * kVdW;
                e->hitY = (0.5f - pk.v * 0.5f) * kVdH;
            }
        }
    }
    float gy;
    if (gazeYaw(head, &gy)) e->gazeYaw = gy;
    e->gazePitch = gazePitch(head);
    if (e->toastOnly && !e->toastWas) e->toastYaw = e->gazeYaw;
    e->toastWas = e->toastOnly;
    if (e->sysMsgOnly && !e->sysMsgWas) e->sysMsgYaw = e->gazeYaw;
    e->sysMsgWas = e->sysMsgOnly;
    // same anchor rule for the keyboard's free quad: lock on the gaze yaw
    // the frame it pops, not every frame
    if (e->kbd.only && !e->kbd.was) e->kbd.yaw = e->gazeYaw;
    e->kbd.was = e->kbd.only;

    // controller button edges land on this frame's fresh hover state
    ctrlFlush(e);

    dragTick(e, e->aimO, e->aimD);
    moveTick(e);
    dockTick(e);

    // every transition steps off the frame delta: park flights chase the
    // minimized flags, icon hovers chase the aim, the grid card chases
    // `shown` so it can fade out instead of vanishing
    const float dtMs = e->animMs ? (float)(now - e->animMs) : 0.0f;
    e->animMs = now;
    if (dtMs > 0.0f) {
        tickPanels(e->panels, dtMs);
        tickDockHover(e->dock, e->dockHover, dtMs);
        tickShelfHover(e->shelf, e->shelfHover, dtMs);
        tickGridHover(e->grid.items, e->grid.hover, dtMs);
        e->grid.openT = stepT(e->grid.openT, e->grid.shown, dtMs, kGridMs);
    }

    updatePanels(e);
    // keyboard surface + shown/display state: works while hidden too, the
    // hand-off must not depend on the overlay being up
    kbdTick(e);

    // hidden or no surface: management above must still run - pumpBridge is
    // what adopts strays and keeps the displays alive - but there is nothing
    // to present and no vsync to pace us
    if (!e->ready) { usleep(33000); return; }

    // covered and quiet: with no panels, cards, keyboard or gestures up the
    // only thing on screen is a static strip and cursor. A stereo+warp pass
    // every vsync then only starves the app underneath of GPU, so presents
    // drop to a low cadence; anything changing flips back on the next tick
    if (e->covered) {
        HudQuietIn q;
        hudQuietSample(e, &q);
        if (hudQuietBusy(q)) e->lastBusyMs = now;
        if (hudQuiet(q, now - e->lastBusyMs, 1000) &&
            ++e->quietDiv < 6) { usleep(8000); return; }
    }
    e->quietDiv = 0;

    char extra[48];
    snprintf(extra, sizeof(extra), "  PNL %zu%s", e->panels.size(),
             e->bridge ? "" : "  BRIDGE:OFF");
    updateHud(e, extra);
    fmtCtrlLine(e->hud2, sizeof(e->hud2), e->input, e->ctrl, e->ctrlPos,
                e->ctrlDir);
    const float aspect = (float)e->eye[0].w / (float)e->eye[0].h;
    const float fov = propF("debug.vrhome.fov", kFovY);
    drawEyes(e, head, perspective(fov, aspect, 0.05f, 100.0f), true,
             statusLineVisible(propI("debug.vrhome.hud", -1), e->debugHud),
             hudScene);
    warpPresent(e);
    updateFps(e);
}

static void* hudThread(void* arg) {
    HudEngine* e = (HudEngine*)arg;
    // own looper for the sensor queue: the java listener path never delivers
    // to a background service on this build, so the HUD reads the same NDK
    // event queue the env uses
    ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorManager* sensorMgr =
        ASensorManager_getInstanceForPackage("gitlab.neosalsa.hud");
    e->sensorMgr = sensorMgr;
    // pick any handle other than the env's default non-wakeup one: this HAL
    // only emits the first-flush meta event on a real activation, so a
    // second connection on the streaming handle stays "First flush pending"
    // forever and never sees a sample. Non-wakeup first - the wakeup handle
    // works but the vendor ack path spams sendAck errors per event
    const ASensor* def = ASensorManager_getDefaultSensor(sensorMgr,
        ASENSOR_TYPE_GAME_ROTATION_VECTOR);
    ASensorList list = nullptr;
    const int n = ASensorManager_getSensorList(sensorMgr, &list);
    const ASensor* rot = nullptr;
    for (int i = 0; i < n && !rot; ++i)
        if (ASensor_getType(list[i]) == ASENSOR_TYPE_GAME_ROTATION_VECTOR &&
                list[i] != def && !ASensor_isWakeUpSensor(list[i]))
            rot = list[i];
    for (int i = 0; i < n && !rot; ++i)
        if (ASensor_getType(list[i]) == ASENSOR_TYPE_ROTATION_VECTOR)
            rot = list[i];
    if (!rot) rot = def;
    e->rotSensor = rot;
    e->sensorQueue = ASensorManager_createEventQueue(sensorMgr,
        ALooper_forThread(), kSensorIdent, nullptr, nullptr);
    if (rot) {
        ASensorEventQueue_enableSensor(e->sensorQueue, rot);
        ASensorEventQueue_setEventRate(e->sensorQueue, rot, 2000);
    }
    // GL up front on the pbuffer: panels and display adoption must work even
    // before the overlay is shown for the first time
    initEglContext(e);
    while (e->running) hudFrame(e);
    termDisplay(e);
    return nullptr;
}

// ---------------------------------------------------------------- jni

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeInit(JNIEnv* env, jclass,
                                             jobject ctx, jobject bridge) {
    HudEngine* e = &gHud;
    env->GetJavaVM(&e->vm);
    e->ctx = env->NewGlobalRef(ctx);
    // bridge may be null if the java ctor threw: panels stay dead but the
    // service (and its window) still comes up
    if (bridge) initBridge(e, env, bridge);
    if (!gStarted) {
        gStarted = true;
        pthread_create(&gThread, nullptr, hudThread, e);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeWindow(JNIEnv* env, jclass,
                                                jobject surface) {
    ANativeWindow* nw = ANativeWindow_fromSurface(env, surface);
    ANativeWindow* old = gHud.window.exchange(nw);
    if (old) ANativeWindow_release(old);
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeWindowGone(JNIEnv*, jclass) {
    gHud.windowGone = true;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeKey(JNIEnv*, jclass,
                                            jint code, jint action,
                                            jint repeat) {
    HudEngine* e = &gHud;
    std::lock_guard<std::mutex> l(e->keyMu);
    e->keyQ.push_back({(int)code, (int)action, (int)repeat});
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeRecenter(JNIEnv*, jclass) {
    wantRecenter();
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeHoldStart(JNIEnv*, jclass) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    gHud.holdStartMs = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeHoldEnd(JNIEnv*, jclass) {
    gHud.holdStartMs = 0;
}

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_HudService_nativeShutdown(JNIEnv*, jclass) {
    gHud.running = false;
}
