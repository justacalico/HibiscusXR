#include "input.h"

#include "keys.h"
#include "../engine.h"
#include "../bridge/bridge.h"
#include "../common/log.h"
#include "../common/config.h"
#include "../panels/layout.h"
#include "../panels/panels.h"

#include <android/keycodes.h>

int32_t onInputEvent(android_app* app, AInputEvent* ev) {
    Engine* e = (Engine*)app->userData;
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_KEY)
        return 0;
    const int32_t code = AKeyEvent_getKeyCode(ev);
    const int32_t action = AKeyEvent_getAction(ev);

    if (isConfirm(code)) {
        if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(ev) == 0)
            LOGI("confirm down, hover %d", e->hover);
        if (action == AKEY_EVENT_ACTION_UP && e->confirmHeld) {
            e->confirmHeld = false;
            if (e->bridge && e->hover >= 0 && e->hover < (int)e->panels.size()) {
                JNIEnv* env = threadEnv(app);
                const Panel& p = e->panels[e->hover];
                LOGI("tap disp %d @ %.0f,%.0f", p.displayId, e->hitX, e->hitY);
                env->CallVoidMethod(e->bridge, e->mInjectTap,
                                    p.displayId, (float)e->hitX, (float)e->hitY);
                if (env->ExceptionCheck()) env->ExceptionClear();
                if (p.taskId >= 0) {
                    env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                    if (env->ExceptionCheck()) env->ExceptionClear();
                }
            }
        } else if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(ev) == 0) {
            e->confirmHeld = true;
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
