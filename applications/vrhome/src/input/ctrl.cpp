#include "ctrl.h"

#include "aim.h"
#include "input.h"
#include "input_state.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../common/props.h"
#include "../hud/engine.h"
#include "../math/head.h"

#include <android/input.h>
#include <android/keycodes.h>

#include <errno.h>

// how often to retry the mmap while the service hasn't created the file
constexpr long long kCtrlRetryMs = 2000;

void ctrlTick(HudEngine* e, const Mat4& head, float sensRoll, float worldX,
              float roll, long long nowMs) {
    if (!e->ctrlOpen) {
        // the file only exists while the service runs - no channel means
        // nothing can be live, so the gaze pointer takes over right away
        ctrlDropAll(e->input);
        if (nowMs >= e->ctrlRetryMs) {
            e->ctrlRetryMs = nowMs + kCtrlRetryMs;
            const int rc = ctrl_share_open(&e->ctrlMem, nullptr);
            if (rc != 0) {
                if (!e->ctrlLogged) {
                    e->ctrlLogged = true;
                    LOGE("ctrl sharemem open failed rc=%d errno=%d",
                         rc, errno);
                }
            } else {
                e->ctrlOpen = true;
                LOGI("ctrl sharemem mapped");
            }
        }
    } else {
        uint8_t buf[CTRL_SHARE_SIZE];
        if (ctrl_share_snapshot(&e->ctrlMem, buf) != 0) {
            ctrl_share_close(&e->ctrlMem);
            e->ctrlOpen = false;
        } else {
            const float posScale = propF("debug.vrhome.ctrlscale", 0.001f);
            for (int w = 0; w < CTRL_COUNT; ++w) {
                const bool was = ctrlConnected(e->input, w);
                ctrl_state_decode(buf, w, &e->ctrl[w]);
                InputEvent ev[BTN_COUNT * 2];
                int n = inputTick(e->input, w, ctrl_state_hash(buf, w),
                                  e->ctrl[w], nowMs, ev, BTN_COUNT * 2);
                const bool now = ctrlConnected(e->input, w);
                if (now != was)
                    LOGI("ctrl %d %s bat=%d", w, now ? "connected" : "lost",
                         e->ctrl[w].keys.battery);
                for (int i = 0; i < n; ++i) e->ctrlEv.push_back(ev[i]);

                if (ctrlConnected(e->input, w) && e->ctrl[w].pose_ok) {
                    const ctrl_pose& p = e->ctrl[w].fuse;
                    const float q[4] = {p.qx, p.qy, p.qz, p.q0};
                    const float pp[3] = {p.x, p.y, p.z};
                    ctrlAim(q, pp, w, sensRoll, worldX, roll,
                            propI("debug.vrhome.tq", 1) != 0, posScale,
                            e->eyePos, e->ctrlPos[w], e->ctrlDir[w],
                            &e->ctrlMat[w]);
                }
            }
        }
    }

    // the aim ray: the active controller's beam when one is live, else the
    // gaze ray off the head - hmdInput() is that same condition. Runs every
    // frame: with no sharemem (no controllers exist yet) this is the only
    // thing keeping the gaze pick alive
    if (e->input.active >= 0) {
        memcpy(e->aimO, e->ctrlPos[e->input.active], sizeof(e->aimO));
        memcpy(e->aimD, e->ctrlDir[e->input.active], sizeof(e->aimD));
    } else {
        memcpy(e->aimO, e->eyePos, sizeof(e->aimO));
        gazeDir(head, e->aimD);
    }
    float ay;
    if (dirYaw(e->aimD, &ay)) e->aimYaw = ay;
    e->aimPitch = dirPitch(e->aimD);
}

void ctrlFlush(HudEngine* e) {
    while (!e->ctrlEv.empty()) {
        const InputEvent ev = e->ctrlEv.front();
        e->ctrlEv.pop_front();
        switch (ev.code) {
        case kCtrlSummon:
            if (ev.action == AKEY_EVENT_ACTION_DOWN && e->ctx) {
                JNIEnv* env = threadEnv(e->vm);
                jclass c = env->GetObjectClass(e->ctx);
                jmethodID m = env->GetMethodID(c, "debugSummon", "()V");
                if (m) env->CallVoidMethod(e->ctx, m);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            break;
        case kCtrlRecenter:
            if (ev.action == AKEY_EVENT_ACTION_DOWN) wantRecenter();
            break;
        default:
            hudKey(e, ev.code, ev.action, 0);
            break;
        }
    }
}
