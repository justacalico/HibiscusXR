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
// no write on the channel for this long means CVService's SPI thread died;
// the java side gets poked to send the start broadcast. Kept infrequent -
// every start also stops and restarts a live thread, so it must only fire
// while the channel is actually quiet
constexpr long long kCtrlQuietMs = 3000;
constexpr long long kCtrlPokeMs = 30000;

// asks HudService to (re)start CVService's controller thread
static void pokeCtrlThread(HudEngine* e) {
    if (!e->ctx || !e->vm) return;
    JNIEnv* env = threadEnv(e->vm);
    jclass c = env->GetObjectClass(e->ctx);
    jmethodID m = env->GetMethodID(c, "pokeCtrlThread", "()V");
    if (m) env->CallVoidMethod(e->ctx, m);
    if (env->ExceptionCheck()) env->ExceptionClear();
}

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
            const uint64_t nowNs = (uint64_t)nowMs * 1000000ull;
            // a quiet hash isn't a dead link - a parked controller
            // produces byte-identical frames while the service keeps
            // writing. The write itself is the heartbeat: one channel-wide
            // probe watches the flag bytes from a throwaway thread (the
            // ~30ms write period would stall the render loop otherwise)
            int wire = ctrl_probe_poll(&e->ctrlProbe);
            if (wire == 0 && nowNs >= e->ctrlProbeNs &&
                e->ctrlProbe.state == 0) {
                e->ctrlProbeNs = nowNs + CTRL_PROBE_GAP_NS;
                ctrl_probe_start(&e->ctrlProbe, e->ctrlMem.path, -1,
                                 CTRL_PROBE_SPIN_NS);
            }
            bool wrote = wire > 0;
            for (int w = 0; w < CTRL_COUNT; ++w) {
                const bool was = ctrlConnected(e->input, w);
                ctrl_state_decode(buf, w, &e->ctrl[w]);
                InputEvent ev[BTN_COUNT * 2];
                const uint64_t hash = ctrl_state_hash(buf, w);
                wrote = wrote || hash != e->ctrlLive[w].hash;
                const bool live =
                    ctrl_live_feed(&e->ctrlLive[w], hash, wire > 0, nowNs) &&
                    ctrl_block_has_data(buf, w);
                int n = inputTick(e->input, w, live, e->ctrl[w],
                                  ev, BTN_COUNT * 2);
                const bool conn = ctrlConnected(e->input, w);
                if (conn != was)
                    LOGI("ctrl %d %s bat=%d", w, conn ? "connected" : "lost",
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

            // nothing on this build starts CVService's controller thread -
            // stock VRShell used to - so when the wire goes quiet (thread
            // never started, or RemoteService crashed and restarted bare)
            // poke the service to spin it up again. A live channel never
            // reaches this: writes keep flushing through every few ms
            if (wrote) {
                e->ctrlQuietMs = 0;
            } else if (e->ctrlQuietMs == 0) {
                e->ctrlQuietMs = nowMs;
            }
            if (e->ctrlQuietMs != 0 &&
                nowMs - e->ctrlQuietMs > kCtrlQuietMs &&
                nowMs - e->ctrlPokeMs > kCtrlPokeMs) {
                e->ctrlPokeMs = nowMs;
                LOGI("ctrl channel quiet, poking cvservice");
                pokeCtrlThread(e);
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
