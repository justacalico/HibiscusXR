#include "input.h"

#include "keys.h"
#include "../dock/dock.h"
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
            if (e->dockHover >= 0 && e->dockHover < (int)e->dock.size()) {
                e->dockPress = e->dockHover;
                e->dockPressZone = e->dockZone;
                e->dockPressPkg = e->dock[e->dockHover].pkg;
                timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                e->dockPressMs = (uint64_t)ts.tv_sec * 1000 +
                                 (uint64_t)ts.tv_nsec / 1000000;
            }
            if (e->hover >= 0 && e->hover < (int)e->panels.size()) {
                const Panel& p = e->panels[e->hover];
                e->pressDisp = p.displayId;
                // a press on the window surface starts a real gesture:
                // DOWN here, MOVEs while held, UP on release - a quick press
                // still lands as a plain tap. a press on the pill only arms
                // its chrome action, fired if the release lands on the same
                // spot
                if (e->bridge && e->hoverZone == ZONE_WINDOW) {
                    JNIEnv* env = threadEnv(e->vm);
                    e->dragDisp = p.displayId;
                    e->dragX = e->grabX = e->hitX;
                    e->dragY = e->grabY = e->hitY;
                    LOGI("drag start disp %d @ %.0f,%.0f",
                         p.displayId, e->hitX, e->hitY);
                    env->CallVoidMethod(e->bridge, e->mInjectTouch, p.displayId,
                                        e->hitX, e->hitY,
                                        AMOTION_EVENT_ACTION_DOWN);
                    if (env->ExceptionCheck()) env->ExceptionClear();
                } else if (e->hoverZone == ZONE_HANDLE) {
                    // ring drag: the held handle tracks the gaze and every
                    // panel follows, so the windows stay in formation
                    e->moveHeld = true;
                    e->moveGrabYaw = e->gazeYaw;
                    e->moveGrabPitch = e->gazePitch;
                    e->dockGrabYaw = e->dockYaw;
                    grabRing(e->panels);
                    LOGI("ring drag grab @ yaw %.2f", e->gazeYaw);
                }
            }
        } else if (action == AKEY_EVENT_ACTION_UP && e->confirmHeld) {
            e->confirmHeld = false;
            e->moveHeld = false;
            JNIEnv* env = threadEnv(e->vm);
            if (e->dockPress >= 0) {
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
                // pill press: fire only when the release is still on the
                // same panel and the same zone it started on
                for (int i = 0; i < (int)e->panels.size(); ++i) {
                    Panel& p = e->panels[i];
                    if (p.displayId != e->pressDisp || e->hover != i)
                        continue;
                    if (e->pressZone == ZONE_CLOSE) {
                        LOGI("pill close disp %d", p.displayId);
                        if (e->bridge && p.taskId >= 0) {
                            env->CallVoidMethod(e->bridge, e->mRemoveTask,
                                                p.taskId);
                            if (env->ExceptionCheck()) env->ExceptionClear();
                        }
                        closePanel(e, i);
                    } else if (e->pressZone == ZONE_MIN) {
                        LOGI("pill minimize disp %d", p.displayId);
                        p.minimized = true;
                        e->hover = -1;
                        e->hoverZone = ZONE_NONE;
                    } else if (e->pressZone == ZONE_LABEL && e->bridge &&
                               p.taskId >= 0) {
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
        // close the newest panel; over a covered app the service consumes
        // BACK itself to dismiss the menu, so this only ever runs in home
        // space
        if (e->bridge && !e->panels.empty()) {
            JNIEnv* env = threadEnv(e->vm);
            Panel& p = e->panels.back();
            if (p.taskId >= 0) {
                env->CallVoidMethod(e->bridge, e->mRemoveTask, p.taskId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            closePanel(e, (int)e->panels.size() - 1);
        }
        return;
    }
}

// streams MOVEs to the display a confirm-press started on; the gaze point is
// clamped inside the window so the drag survives the gaze leaving the edges
void dragTick(HudEngine* e, const Mat4& head) {
    if (!e->confirmHeld || e->dragDisp < 0 || !e->bridge) return;
    for (auto& p : e->panels) {
        if (p.displayId != e->dragDisp) continue;
        float rx, ry;
        if (dragPoint(p, head, e->ringPos, e->eyePos, &rx, &ry)) {
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
// the viewer with the gaze, up and down as well as side to side
void moveTick(HudEngine* e) {
    if (!e->moveHeld) return;
    const float dYaw = wrapPi(e->gazeYaw - e->moveGrabYaw);
    dragRing(e->panels, dYaw, e->gazePitch - e->moveGrabPitch);
    // the dock rides the same ring and follows a handle drag
    e->dockYaw = wrapPi(e->dockGrabYaw + dYaw);
}
