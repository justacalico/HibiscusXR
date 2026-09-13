#include "input.h"

#include "keys.h"
#include "../engine.h"
#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../panels/panels.h"

#include <android/keycodes.h>

#include <cmath>

int32_t onInputEvent(android_app* app, AInputEvent* ev) {
    Engine* e = (Engine*)app->userData;
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_KEY)
        return 0;
    const int32_t code = AKeyEvent_getKeyCode(ev);
    const int32_t action = AKeyEvent_getAction(ev);

    if (isConfirm(code)) {
        const bool down = action == AKEY_EVENT_ACTION_DOWN &&
                          AKeyEvent_getRepeatCount(ev) == 0;
        if (down) {
            LOGI("confirm down, hover %d", e->hover);
            e->confirmHeld = true;
            e->dragDisp = -1;
            // press starts a real gesture: DOWN here, MOVEs while held, UP on
            // release - a quick press still lands as a plain tap
            if (e->bridge && e->hover >= 0 && e->hover < (int)e->panels.size()) {
                JNIEnv* env = threadEnv(app);
                const Panel& p = e->panels[e->hover];
                e->dragDisp = p.displayId;
                e->dragX = e->grabX = e->hitX;
                e->dragY = e->grabY = e->hitY;
                LOGI("drag start disp %d @ %.0f,%.0f",
                     p.displayId, e->hitX, e->hitY);
                env->CallVoidMethod(e->bridge, e->mInjectTouch, p.displayId,
                                    e->hitX, e->hitY, AMOTION_EVENT_ACTION_DOWN);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
        } else if (action == AKEY_EVENT_ACTION_UP && e->confirmHeld) {
            e->confirmHeld = false;
            if (e->bridge && e->dragDisp >= 0) {
                JNIEnv* env = threadEnv(app);
                env->CallVoidMethod(e->bridge, e->mInjectTouch, e->dragDisp,
                                    e->dragX, e->dragY, AMOTION_EVENT_ACTION_UP);
                if (env->ExceptionCheck()) env->ExceptionClear();
                for (auto& p : e->panels)
                    if (p.displayId == e->dragDisp && p.taskId >= 0) {
                        env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        break;
                    }
            }
            e->dragDisp = -1;
        }
        return 1;
    }
    if (code == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP) {
        // display-0 focus: close the newest panel; when a panel app has
        // focus the key never reaches us - the app handles it natively
        if (e->bridge && !e->panels.empty()) {
            JNIEnv* env = threadEnv(app);
            Panel& p = e->panels.back();
            if (p.taskId >= 0) {
                env->CallVoidMethod(e->bridge, e->mRemoveTask, p.taskId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            closePanel(e, (int)e->panels.size() - 1);
        }
        return 1;
    }
    if (code == AKEYCODE_HOME && action == AKEY_EVENT_ACTION_UP) {
        // recenter: the ring's slot layout recentres on the current gaze yaw
        recenterSlots(e->panels, e->gazeYaw);
        return 1;
    }
    return 0;
}

// streams MOVEs to the display a confirm-press started on; the gaze point is
// clamped inside the window so the drag survives the gaze leaving the edges
void dragTick(Engine* e, const Mat4& head) {
    if (!e->confirmHeld || e->dragDisp < 0 || !e->bridge) return;
    for (auto& p : e->panels) {
        if (p.displayId != e->dragDisp) continue;
        float rx, ry;
        if (dragPoint(p, head, &rx, &ry)) {
            const float px = dragBoost(e->grabX, rx, kVdW);
            const float py = dragBoost(e->grabY, ry, kVdH);
            if (fabsf(px - e->dragX) <= 1.0f && fabsf(py - e->dragY) <= 1.0f)
                return;
            e->dragX = px; e->dragY = py;
            JNIEnv* env = threadEnv(e->app);
            env->CallVoidMethod(e->bridge, e->mInjectTouch, p.displayId,
                                px, py, AMOTION_EVENT_ACTION_MOVE);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        return;
    }
    e->dragDisp = -1;   // window went away mid-drag
}
