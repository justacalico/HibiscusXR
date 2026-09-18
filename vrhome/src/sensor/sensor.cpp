#include "sensor.h"

#include "../engine.h"
#include "../math/head.h"

#include <cmath>

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
