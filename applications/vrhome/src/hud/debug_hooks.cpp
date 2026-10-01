#include "debug_hooks.h"

#include "engine.h"

#include "../bridge/bridge.h"
#include "../common/jni.h"
#include "../dock/dock.h"
#include "../notif/notif.h"
#include "../sysmsg/sysmsg.h"

#include <sys/system_properties.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// the prop's current value when it changed since the hook last saw it,
// else false; most props can't be cleared from this uid, so change
// detection is what makes them retriggerable
static bool propChanged(const char* name, char* last, char* out) {
    if (__system_property_get(name, out) <= 0) return false;
    if (strcmp(out, last) == 0) return false;
    strncpy(last, out, PROP_VALUE_MAX - 1);
    return true;
}

// calls a no-arg (or int-arg) debug method on the service object
static void callCtx(HudEngine* e, const char* name, const char* sig,
                    jint arg = 0) {
    JNIEnv* env = threadEnv(e->vm);
    jclass c = env->GetObjectClass(e->ctx);
    jmethodID m = env->GetMethodID(c, name, sig);
    if (m) env->CallVoidMethod(e->ctx, m, arg);
    if (env->ExceptionCheck()) env->ExceptionClear();
}

// setprop debug.vrhome.launch <pkg> queues a panel launch
static void debugLaunchHook(HudEngine* e) {
    static bool fired = false;
    char tb[PROP_VALUE_MAX];
    if (!fired && __system_property_get("debug.vrhome.launch", tb) > 0
            && e->frames > 30) {
        fired = true;
        queueLaunch(tb);
    }
}

// setprop debug.vrhome.tap "disp,x,y" injects a tap there
static void debugTapHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.tap", last, tb)) {
        int d, x, y;
        if (sscanf(tb, "%d,%d,%d", &d, &x, &y) == 3 && e->bridge) {
            JNIEnv* env = threadEnv(e->vm);
            env->CallVoidMethod(e->bridge, e->mInjectTap, d,
                                (float)x, (float)y);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }
}

// setprop debug.vrhome.summon <n> toggles the dash over the covered app,
// same as a short summon press
static void debugSummonHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (e->ctx && propChanged("debug.vrhome.summon", last, tb))
        callCtx(e, "debugSummon", "()V");
}

// setprop debug.vrhome.hold 1|0 feeds a summon-key down/up through the
// real onSummon path, so the hold ring and long-press recenter are
// drivable from adb - injected keyevents never reach the key filter
static void debugHoldHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (e->ctx && propChanged("debug.vrhome.hold", last, tb))
        callCtx(e, "debugSummonKey", "(I)V", tb[0] == '1' ? 0 : 1);
}

// setprop debug.vrhome.docktap <n> activates dock item n, same as a gaze
// tap landing on it
static void debugDockTapHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.docktap", last, tb))
        dockActivate(e, atoi(tb));
}

// setprop debug.vrhome.dockclose <n> hits the close badge on dock item n
static void debugDockCloseHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.dockclose", last, tb))
        dockClose(e, atoi(tb));
}

// setprop debug.vrhome.dockpin <pkg> runs the pin toggle the confirm-hold
// gesture ends in
static void debugDockPinHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.dockpin", last, tb))
        dockTogglePin(e, tb);
}

// setprop debug.vrhome.notifclose <n> hits the dismiss badge on card n
static void debugNotifCloseHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.notifclose", last, tb))
        notifDismiss(e, atoi(tb));
}

// setprop debug.vrhome.sysmsg <n> clicks button n on the front
// system-message card (0 = Close, 1 = Restart)
static void debugSysMsgHook(HudEngine* e) {
    static char last[PROP_VALUE_MAX] = "";
    char tb[PROP_VALUE_MAX];
    if (propChanged("debug.vrhome.sysmsg", last, tb))
        sysMsgBtnClick(e, atoi(tb));
}

void runDebugHooks(HudEngine* e) {
    debugLaunchHook(e);
    debugTapHook(e);
    debugSummonHook(e);
    debugHoldHook(e);
    debugDockTapHook(e);
    debugDockCloseHook(e);
    debugDockPinHook(e);
    debugNotifCloseHook(e);
    debugSysMsgHook(e);
}
