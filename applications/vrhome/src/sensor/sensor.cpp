#include "sensor.h"

#include "../engine.h"
#include "../common/config.h"
#include "../common/props.h"
#include "../math/head.h"

#include <cmath>
#include <cstring>
#include <ctime>

void drainSensor(Engine* e) {
    if (!e->sensorQueue) return;
    ASensorEvent ev;
    while (ASensorEventQueue_getEvents(e->sensorQueue, &ev, 1) > 0) {
        if (ev.type == ASENSOR_TYPE_GAME_ROTATION_VECTOR ||
            ev.type == ASENSOR_TYPE_ROTATION_VECTOR) {
            ++e->sensorEv;
            // QVR owns the pose while it tracks; rot-vec is fallback only,
            // so it must not clobber e->quat then
            if (!e->quatFromQvr) {
                e->quat[0] = ev.data[0]; e->quat[1] = ev.data[1];
                e->quat[2] = ev.data[2];
                e->quat[3] = quatW(ev.data);
                e->haveQuat = true;
            }
            if (fabsf(ev.data[0] - e->lastQ[0]) > 1e-5f ||
                fabsf(ev.data[1] - e->lastQ[1]) > 1e-5f ||
                fabsf(ev.data[2] - e->lastQ[2]) > 1e-5f ||
                fabsf(ev.data[3] - e->lastQ[3]) > 1e-5f) {
                ++e->sensorNew;
                e->lastQ[0]=ev.data[0]; e->lastQ[1]=ev.data[1];
                e->lastQ[2]=ev.data[2]; e->lastQ[3]=ev.data[3];
            }
        }
    }
}

void smoothPose(Engine* e, bool useSensor) {
    if (!useSensor || !propI("debug.vrhome.posefilt", 1)) {
        poseFiltReset(&e->viewPose);
        e->viewPoseMs = 0;
        return;
    }
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    const float dt = e->viewPoseMs ? (float)(now - e->viewPoseMs) / 1000.0f
                                   : 0.0f;
    e->viewPoseMs = now;
    poseFiltTick(&e->viewPose, e->quat,
                 e->headPosValid ? e->headPos : nullptr, dt);
    memcpy(e->quat, e->viewPose.quat, sizeof(e->quat));
    if (e->quatFromQvr) {
        // the sampled pose reaches photons a frame or two late: roll it
        // forward by the live angular velocity so the displayed view
        // matches where the head actually is. Tunable lead for on-device
        // verification; 0 disables.
        const float lead = propF("debug.vrhome.predms", kPredMs) / 1000.0f;
        if (lead != 0.0f) predictQuat(e->quat, e->headAv, lead, e->quat);
    }
    if (e->headPosValid)
        memcpy(e->headPos, e->viewPose.pos, sizeof(e->headPos));
}
