#include "input.h"

#include "keys.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../grid/grid.h"
#include "../grid/layout.h"
#include "../kbd/kbd.h"
#include "../notif/notif.h"
#include "../sysmsg/sysmsg.h"
#include "../hud/engine.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../panels/panels.h"

#include <android/input.h>
#include <android/keycodes.h>

#include <cmath>
#include <ctime>

// KeyEvent action/motion constants come from android/input.h; the events
// themselves arrive from the java window, never the NDK input queue
void hudKey(HudEngine* e, int code, int action, int repeat) {
    if (isConfirm(code)) {
        const bool down = action == AKEY_EVENT_ACTION_DOWN && repeat == 0;
        if (down) {
            LOGI("confirm down, hover %d zone %d dock %d", e->hover,
                 e->hoverZone, e->dockHover);
            e->confirmHeld = true;
            e->moveHeld = false;
            e->dragDisp = -1;
            e->pressDisp = -1;
            e->pressZone = e->hoverZone;
            e->dockPress = -1;
            e->dockPressZone = DZONE_NONE;
            e->dockPinP = 0.0f;
            e->dockPinDone = false;
            e->sysPress = false;
            e->notifPress = -1;
            e->notifPressZone = NZONE_NONE;
            e->shelfPress = -1;
            e->shelfPressDisp = -1;
            e->sysMsgPress = -1;
            e->sysMsgPressZone = MZONE_NONE;
            e->sysMsgPressBtn = -1;
            e->sysMsgPressId = 0;
            e->kbd.pressed = false;
            e->kbd.moveHeld = false;
            e->grid.press = -1;
            e->grid.pressZone = GZONE_NONE;
            e->grid.scrollHeld = false;
            if (e->sysMsgHover >= 0 && !e->sysMsgs.empty()) {
                e->sysMsgPress = e->sysMsgHover;
                e->sysMsgPressZone = e->sysMsgZone;
                e->sysMsgPressBtn = e->sysMsgBtn;
                e->sysMsgPressId = e->sysMsgs.front().id;
            }
            if (e->shelfHover >= 0 &&
                    e->shelfHover < (int)e->shelf.size()) {
                const ShelfItem& si = e->shelf[e->shelfHover];
                if (si.panelIdx >= 0 &&
                        si.panelIdx < (int)e->panels.size()) {
                    e->shelfPress = e->shelfHover;
                    e->shelfPressDisp = e->panels[si.panelIdx].displayId;
                }
            }
            if (e->notifHover >= 0 &&
                    e->notifHover < (int)e->notifs.size()) {
                e->notifPress = e->notifHover;
                e->notifPressZone = e->notifZone;
                e->notifPressKey = e->notifs[e->notifHover].key;
            }
            if (e->grid.zone == GZONE_ITEM && e->grid.hover >= 0) {
                // a cell press: arms the launch, fires on a same-cell
                // release
                e->grid.press = e->grid.hover;
                e->grid.pressZone = GZONE_ITEM;
                e->grid.pressPkg = e->grid.items[e->grid.hover].pkg;
            } else if (e->grid.zone == GZONE_CLOSE) {
                e->grid.pressZone = GZONE_CLOSE;
            } else if (e->grid.zone == GZONE_BODY) {
                // the card body holds a scroll drag: grab the aim's
                // card-local v and the current scroll so moveTick can
                // reapply them each frame
                e->grid.scrollHeld = true;
                e->grid.grabV = e->grid.v;
                e->grid.grabScroll = e->grid.scroll;
            }
            if (e->dockHover >= 0 && e->dockHover < (int)e->dock.size()) {
                e->dockPress = e->dockHover;
                e->dockPressZone = e->dockZone;
                e->dockPressPkg = e->dock[e->dockHover].pkg;
                timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                e->dockPressMs = (uint64_t)ts.tv_sec * 1000 +
                                 (uint64_t)ts.tv_nsec / 1000000;
            } else if (e->dockZone == DZONE_HANDLE) {
                // ring drag: the held handle under the dock tracks the
                // aim and every panel follows, so the windows stay in
                // formation. No dockPress: releasing only ends the drag
                e->moveHeld = true;
                e->moveGrabYaw = e->aimYaw;
                e->moveGrabPitch = e->aimPitch;
                e->dockGrabYaw = e->dockYaw;
                e->dockGrabPitch = e->dockPitch;
                e->ringGrabPitch = ringPitch(e->panels);
                grabRing(e->panels);
                LOGI("ring drag grab @ yaw %.2f", e->aimYaw);
            } else if (e->dockZone == DZONE_SYS) {
                // the status pill: the quick panel fires on a release that
                // lands back on the pill
                e->sysPress = true;
            }
            if (e->hover >= 0 && e->hover < (int)e->panels.size()) {
                Panel& p = e->panels[e->hover];
                e->pressDisp = p.displayId;
                if (e->hoverZone == ZONE_PILL) {
                    // a floating window's own move pill: grab the aim and
                    // the panel's spot so moveTick can apply the delta
                    p.grabYaw = p.yaw;
                    p.grabPitch = p.pitch;
                    e->moveGrabYaw = e->aimYaw;
                    e->moveGrabPitch = e->aimPitch;
                    LOGI("pill drag grab disp %d", p.displayId);
                } else if (e->hoverZone == ZONE_LABEL) {
                    // a docked window's pill body is its drag handle too:
                    // the same grab snapshot drives moveTick's slot hop,
                    // and an undragged release still focuses the task
                    p.grabYaw = p.yaw;
                    e->moveGrabYaw = e->aimYaw;
                    LOGI("slot drag grab disp %d", p.displayId);
                } else if (e->hoverZone == ZONE_RESIZE) {
                    // corner grip: grab the scale and the hit's distance
                    // from centre so the drag's radial gain drives resize.
                    // The radius is in the window's unscaled metres - the
                    // grip would chase its own growth otherwise
                    p.grabScale = p.scale;
                    const float rx = e->hitU * kPanelW * 0.5f,
                                ry = e->hitV * kPanelH * 0.5f;
                    p.grabR = sqrtf(rx * rx + ry * ry);
                    LOGI("resize grab disp %d r %.3f", p.displayId,
                         p.grabR);
                }
                // a press on the window surface starts a real gesture:
                // DOWN here, MOVEs while held, UP on release - a quick press
                // still lands as a plain tap. a press on the top bar only
                // arms its chrome action, fired if the release lands on the
                // same spot
                if (e->bridge && e->hoverZone == ZONE_WINDOW) {
                    JNIEnv* env = threadEnv(e->vm);
                    e->dragDisp = p.displayId;
                    e->dragX = e->grabX = e->hitX;
                    e->dragY = e->grabY = e->hitY;
                    // the window taking a tap owns the text field the IME
                    // types into - the quad hangs under this panel
                    e->kbd.hostDisp = p.displayId;
                    LOGI("drag start disp %d @ %.0f,%.0f",
                         p.displayId, e->hitX, e->hitY);
                    env->CallVoidMethod(e->bridge, e->mInjectTouch, p.displayId,
                                        e->hitX, e->hitY,
                                        AMOTION_EVENT_ACTION_DOWN);
                    if (env->ExceptionCheck()) env->ExceptionClear();
                }
            }
            if (e->kbd.hover && e->kbd.zone == KZONE_HANDLE) {
                // the pill under the quad: holding it drags the keyboard
                // alone - grab the aim so moveTick can apply the delta
                e->kbd.moveHeld = true;
                e->kbd.grabAimYaw = e->aimYaw;
                e->kbd.grabAimPitch = e->aimPitch;
                e->kbd.grabOffYaw = e->kbd.offYaw;
                e->kbd.grabOffY = e->kbd.offY;
                LOGI("kbd drag grab @ yaw %.2f", e->aimYaw);
            } else if (e->kbd.hover && e->kbd.displayId >= 0 && e->bridge) {
                // a key tap on the floating quad: injected straight onto
                // the IME's own display, no panel gesture involved
                e->kbd.pressed = true;
                kbdHitPx(e->kbd.u, e->kbd.v, &e->kbd.px, &e->kbd.py);
                JNIEnv* env = threadEnv(e->vm);
                env->CallVoidMethod(e->bridge, e->mInjectTouch,
                                    e->kbd.displayId, e->kbd.px, e->kbd.py,
                                    AMOTION_EVENT_ACTION_DOWN);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
        } else if (action == AKEY_EVENT_ACTION_UP && e->confirmHeld) {
            e->confirmHeld = false;
            e->moveHeld = false;
            e->kbd.moveHeld = false;
            e->grid.scrollHeld = false;
            JNIEnv* env = threadEnv(e->vm);
            if (e->grid.pressZone == GZONE_ITEM ||
                    e->grid.pressZone == GZONE_CLOSE) {
                // an app cell or the close disc: fires only when the
                // release lands back on the same spot and a package-list
                // rebuild hasn't moved the cell under the press
                const bool same = e->grid.press >= 0 &&
                    e->grid.hover == e->grid.press &&
                    e->grid.zone == e->grid.pressZone &&
                    e->grid.press < (int)e->grid.items.size() &&
                    e->grid.items[e->grid.press].pkg == e->grid.pressPkg;
                const bool sameClose = e->grid.pressZone == GZONE_CLOSE &&
                                       e->grid.zone == GZONE_CLOSE;
                if (same) gridActivate(e, e->grid.press);
                else if (sameClose) e->grid.shown = false;
                e->grid.press = -1;
                e->grid.pressZone = GZONE_NONE;
                e->grid.pressPkg.clear();
            } else if (e->kbd.pressed) {
                // key tap: the UP lands where the DOWN did - the aim may
                // have drifted off the key while the button was held
                if (e->bridge && e->kbd.displayId >= 0) {
                    env->CallVoidMethod(e->bridge, e->mInjectTouch,
                                        e->kbd.displayId, e->kbd.px,
                                        e->kbd.py,
                                        AMOTION_EVENT_ACTION_UP);
                    if (env->ExceptionCheck()) env->ExceptionClear();
                }
                e->kbd.pressed = false;
            } else if (e->sysMsgPress >= 0) {
                // a button press fires only when the release lands back on
                // the same pill and the card underneath hasn't swapped -
                // the id guard catches a dismissal mid-press
                const bool same = e->sysMsgHover == e->sysMsgPress &&
                    e->sysMsgZone == e->sysMsgPressZone &&
                    e->sysMsgBtn == e->sysMsgPressBtn &&
                    !e->sysMsgs.empty() &&
                    e->sysMsgs.front().id == e->sysMsgPressId;
                if (same && e->sysMsgPressZone == MZONE_BTN &&
                        e->sysMsgPressBtn >= 0)
                    sysMsgBtnClick(e, e->sysMsgPressBtn);
                e->sysMsgPress = -1;
                e->sysMsgPressZone = MZONE_NONE;
                e->sysMsgPressBtn = -1;
                e->sysMsgPressId = 0;
            } else if (e->notifPress >= 0) {
                // a card press fires only when the release lands back on
                // the same card: the badge dismisses, the body does
                // nothing - a stray release shouldn't swallow the post
                const bool same = e->notifHover == e->notifPress &&
                    e->notifZone == e->notifPressZone &&
                    e->notifPress < (int)e->notifs.size() &&
                    e->notifs[e->notifPress].key == e->notifPressKey;
                if (same && e->notifPressZone == NZONE_CLOSE)
                    notifDismiss(e, e->notifPress);
                e->notifPress = -1;
                e->notifPressZone = NZONE_NONE;
                e->notifPressKey.clear();
            } else if (e->shelfPress >= 0) {
                // release back on the same parked icon restores the
                // window; the displayId guard catches the panel list
                // shifting under a held press
                const bool same = e->shelfHover == e->shelfPress &&
                    e->shelfPress < (int)e->shelf.size() &&
                    e->shelf[e->shelfPress].panelIdx >= 0 &&
                    e->shelf[e->shelfPress].panelIdx <
                        (int)e->panels.size() &&
                    e->panels[e->shelf[e->shelfPress].panelIdx].displayId ==
                        e->shelfPressDisp;
                if (same) shelfActivate(e, e->shelfPress);
                e->shelfPress = -1;
                e->shelfPressDisp = -1;
            } else if (e->sysPress) {
                // a release still on the status pill opens the quick
                // panel; drifting off drops the press like the items do
                if (e->dockZone == DZONE_SYS) dockSysActivate(e);
                e->sysPress = false;
            } else if (e->dockPress >= 0) {
                // release over the same dock item (and zone) fires its
                // action; a completed pin-hold suppresses the tap
                const bool same = e->dockHover == e->dockPress &&
                    e->dockZone == e->dockPressZone &&
                    e->dockPress < (int)e->dock.size() &&
                    e->dock[e->dockPress].pkg == e->dockPressPkg;
                if (same && !e->dockPinDone) {
                    if (e->dockPressZone == DZONE_CLOSE)
                        dockClose(e, e->dockPress);
                    else
                        dockActivate(e, e->dockPress);
                }
                e->dockPress = -1;
                e->dockPressZone = DZONE_NONE;
                e->dockPressPkg.clear();
                e->dockPinP = 0.0f;
                e->dockPinDone = false;
            } else if (e->bridge && e->dragDisp >= 0) {
                env->CallVoidMethod(e->bridge, e->mInjectTouch, e->dragDisp,
                                    e->dragX, e->dragY, AMOTION_EVENT_ACTION_UP);
                if (env->ExceptionCheck()) env->ExceptionClear();
                for (auto& p : e->panels)
                    if (p.displayId == e->dragDisp && p.taskId >= 0) {
                        env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        break;
                    }
            } else if (e->pressDisp >= 0 && e->hoverZone == e->pressZone) {
                // bar press: fire only when the release is still on the
                // same panel and the same zone it started on
                for (int i = 0; i < (int)e->panels.size(); ++i) {
                    Panel& p = e->panels[i];
                    if (p.displayId != e->pressDisp || e->hover != i)
                        continue;
                    if (e->pressZone == ZONE_CLOSE) {
                        LOGI("bar close disp %d", p.displayId);
                        if (e->bridge && p.displayId >= 0) {
                            env->CallVoidMethod(e->bridge, e->mRemoveDisp,
                                                p.displayId);
                            if (env->ExceptionCheck()) env->ExceptionClear();
                        }
                        closePanel(e, i);
                    } else if (e->pressZone == ZONE_MIN) {
                        LOGI("bar minimize disp %d", p.displayId);
                        p.minimized = true;
                        e->hover = -1;
                        e->hoverZone = ZONE_NONE;
                    } else if (e->pressZone == ZONE_FLOAT) {
                        // float unpins the window from the slot grid and
                        // hands it its own move pill; un-float snaps it
                        // back onto the slot nearest where it floats
                        p.floating = !p.floating;
                        LOGI("float %s disp %d",
                             p.floating ? "on" : "off", p.displayId);
                        if (!p.floating) {
                            p.yaw = dockSlotYaw(e->panels, i, e->dockYaw);
                            p.pitch = dashRingPitch(e->panels,
                                                    e->dockPitch);
                        }
                    } else if (e->pressZone == ZONE_LABEL && e->bridge &&
                               p.taskId >= 0 &&
                               fabsf(wrapPi(p.yaw - p.grabYaw)) < 0.02f) {
                        // a still pill release focuses the task; a slot
                        // drag that moved the window ends here instead
                        env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                        if (env->ExceptionCheck()) env->ExceptionClear();
                    }
                    break;
                }
            }
            e->dragDisp = -1;
            e->pressDisp = -1;
            e->pressZone = ZONE_NONE;
        }
        return;
    }
    if (code == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP) {
        // the floating keyboard owns BACK first: drop the quad, not the
        // window or card behind it
        if (e->kbd.shown && e->bridge && e->mKbdHide) {
            JNIEnv* env = threadEnv(e->vm);
            env->CallVoidMethod(e->bridge, e->mKbdHide);
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }
        // a live system message is modal: BACK drops the front card, not
        // the window behind it
        if (!e->sysMsgs.empty()) {
            sysMsgDismiss(e);
            return;
        }
        // the app grid drops before the windows under it do
        if (e->grid.shown) {
            e->grid.shown = false;
            return;
        }
        // close the newest panel; over a covered app the service consumes
        // BACK itself to dismiss the menu, so this only ever runs in home
        // space
        if (e->bridge && !e->panels.empty()) {
            JNIEnv* env = threadEnv(e->vm);
            Panel& p = e->panels.back();
            if (p.displayId >= 0) {
                env->CallVoidMethod(e->bridge, e->mRemoveDisp, p.displayId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            closePanel(e, (int)e->panels.size() - 1);
        }
        return;
    }
}

// streams MOVEs to the display a confirm-press started on; the aim point is
// clamped inside the window so the drag survives the ray leaving the edges
void dragTick(HudEngine* e, const float o[3], const float d[3]) {
    if (!e->confirmHeld || e->dragDisp < 0 || !e->bridge) return;
    for (auto& p : e->panels) {
        if (p.displayId != e->dragDisp) continue;
        float rx, ry;
        if (dragPointRay(p, e->ringPos, o, d, &rx, &ry)) {
            const float px = dragBoost(e->grabX, rx, kVdW);
            const float py = dragBoost(e->grabY, ry, kVdH);
            if (fabsf(px - e->dragX) <= 1.0f && fabsf(py - e->dragY) <= 1.0f)
                return;
            e->dragX = px; e->dragY = py;
            JNIEnv* env = threadEnv(e->vm);
            env->CallVoidMethod(e->bridge, e->mInjectTouch, p.displayId,
                                px, py, AMOTION_EVENT_ACTION_MOVE);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        return;
    }
    e->dragDisp = -1;   // window went away mid-drag
}

// held on a drag handle: every panel keeps its slot offset and swings around
// the viewer with the aim, up and down as well as side to side, and the dock
// rides along - the whole dash moves as one piece. The keyboard's own pill
// instead shifts just the quad's offset from its host anchor, so the dash
// drag still carries it (the offset rides the ring) while the pill moves it
// alone
void moveTick(HudEngine* e) {
    // a floating window's own pill drags it alone: its yaw/pitch chase the
    // aim delta from the grab, the rest of the dash stays put
    if (e->pressZone == ZONE_PILL && e->confirmHeld && e->pressDisp >= 0) {
        for (auto& p : e->panels) {
            if (p.displayId != e->pressDisp || !p.floating) continue;
            p.yaw = wrapPi(p.grabYaw + wrapPi(e->aimYaw - e->moveGrabYaw));
            const float np = p.grabPitch + (e->aimPitch - e->moveGrabPitch);
            p.pitch = np > kPitchMax ? kPitchMax
                      : np < -kPitchMax ? -kPitchMax : np;
            break;
        }
    }
    // a docked window's pill drags it through the ring's three slots: the
    // aim delta picks the space, whoever sits there trades places with it
    if (e->pressZone == ZONE_LABEL && e->confirmHeld && e->pressDisp >= 0) {
        for (int i = 0; i < (int)e->panels.size(); ++i) {
            Panel& p = e->panels[i];
            if (p.displayId != e->pressDisp || p.floating) continue;
            slotDrag(e->panels, i, e->dockYaw,
                     wrapPi(p.grabYaw + wrapPi(e->aimYaw - e->moveGrabYaw)));
            break;
        }
    }
    // the corner grip drags the window's scale: the hit's distance from
    // centre over its grab distance drives the gain
    if (e->pressZone == ZONE_RESIZE && e->confirmHeld &&
            e->pressDisp >= 0) {
        for (auto& p : e->panels) {
            if (p.displayId != e->pressDisp) continue;
            float u, v;
            if (rayPanel(p, e->ringPos, e->aimO, e->aimD, &u, &v)) {
                const float rx = u * kPanelW * 0.5f,
                            ry = v * kPanelH * 0.5f;
                p.scale = resizeScale(p.grabScale, p.grabR,
                                      sqrtf(rx * rx + ry * ry));
            }
            break;
        }
    }
    // a held drag on the grid's body scrolls it: the card-local v delta
    // from the grab slides the cells
    if (e->grid.scrollHeld && e->confirmHeld && e->grid.shown) {
        float c[3], r[3], up[3];
        gridCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos,
                   c, r, up);
        float u, v, t;
        if (rayQuad(c, r, up, e->ringPos, e->aimO, e->aimD,
                    kGridHW, kGridHH, &u, &v, &t)) {
            e->grid.scroll = gridClampScroll(
                e->grid.grabScroll + (v - e->grid.grabV) * kGridHH,
                (int)e->grid.items.size());
        }
    }
    if (e->kbd.moveHeld) {
        e->kbd.offYaw = wrapPi(e->kbd.grabOffYaw +
                               wrapPi(e->aimYaw - e->kbd.grabAimYaw));
        e->kbd.offY = e->kbd.grabOffY +
                      (e->aimPitch - e->kbd.grabAimPitch) * kKbdDist;
    }
    if (!e->moveHeld) return;
    const float dYaw = wrapPi(e->aimYaw - e->moveGrabYaw);
    dragRing(e->panels, dYaw, e->aimPitch - e->moveGrabPitch);
    // the dock is tied to the ring: yaw takes the aim delta, pitch takes the
    // elevation the ring actually gained so a pole clamp can't tear the
    // strip off the windows
    e->dockYaw = wrapPi(e->dockGrabYaw + dYaw);
    e->dockPitch = dockDragPitch(e->dockGrabPitch, e->ringGrabPitch,
                                 ringPitch(e->panels));
}
