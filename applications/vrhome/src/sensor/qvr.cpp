#include "qvr.h"

#include "../engine.h"
#include "../common/props.h"
#include "../math/head.h"

#include <android/log.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "vrhome-qvr", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "vrhome-qvr", __VA_ARGS__)

// qvrservice_head_tracking_data_t, 128 bytes - layout verified against
// /system/lib64/libqvrservice_client.so on the stock image
struct QvrPose {
    float quat[4];          // 0
    float pos[3];           // 16
    uint32_t _p0;           // 28
    uint64_t timestamp;     // 32
    uint8_t _data[68];      // 40: velocities etc, unused
    uint32_t state;         // 108: 0 none, 3 tracking
    uint32_t warn;          // 112
    float poseQ, sensorQ, camQ; // 116..127
};

struct QvrClient {
    void* lib = nullptr;
    void* obj = nullptr;    // QVRServiceClient instance
    void* impl = nullptr;   // *obj: QVRServiceClientImpl
    void (*dtor)(void*) = nullptr;
    int (*stopVR)(void*) = nullptr;
    int (*getPose)(void*, QvrPose**) = nullptr;
    uint64_t lastTs = 0;
    int stall = 0;
    int empty = 0;
};

// QVRSERVICE_TRACKING_MODE_POSITIONAL: verified on device, mode 2 switches
// the head pose to fused camera+IMU tracking
static const int kModePositional = 2;

static void* qvrSym(void* lib, const char* name) {
    void* s = dlsym(lib, name);
    if (!s) LOGW("missing symbol %s", name);
    return s;
}

static QvrClient* qvrOpen() {
    QvrClient* c = (QvrClient*)calloc(1, sizeof(QvrClient));
    c->lib = dlopen("libqvrservice_client.so", RTLD_NOW);
    if (!c->lib) c->lib = dlopen("/system/lib64/libqvrservice_client.so", RTLD_NOW);
    if (!c->lib) c->lib = dlopen("/vendor/lib64/libqvrservice_client.so", RTLD_NOW);
    if (!c->lib) { free(c); return nullptr; }

    void (*ctor)(void*) = (void (*)(void*))qvrSym(c->lib, "_ZN16QVRServiceClientC1Ev");
    c->dtor = (void (*)(void*))qvrSym(c->lib, "_ZN16QVRServiceClientD1Ev");
    int (*startVR)(void*) = (int (*)(void*))qvrSym(c->lib, "_ZN16QVRServiceClient11StartVRModeEv");
    int (*setMode)(void*, int) = (int (*)(void*, int))qvrSym(c->lib,
        "_ZN16QVRServiceClient15SetTrackingModeE24QVRSERVICE_TRACKING_MODE");
    c->stopVR = (int (*)(void*))qvrSym(c->lib, "_ZN16QVRServiceClient10StopVRModeEv");
    c->getPose = (int (*)(void*, QvrPose**))qvrSym(c->lib,
        "_ZN20QVRServiceClientImpl19GetHeadTrackingDataEPP31qvrservice_head_tracking_data_t");
    if (!ctor || !c->dtor || !startVR || !setMode || !c->stopVR || !c->getPose) {
        dlclose(c->lib); free(c); return nullptr;
    }

    c->obj = calloc(1, 64);
    ctor(c->obj);
    c->impl = *(void**)c->obj;
    if (!c->impl) { c->dtor(c->obj); free(c->obj); dlclose(c->lib); free(c); return nullptr; }

    setMode(c->obj, kModePositional);
    // VR mode is service-global: the second process to ask gets an error but
    // the pose ring buffer is already running, so keep the client either way
    // and let getPose prove whether data flows
    if (startVR(c->obj) != 0)
        LOGW("StartVRMode rejected, reading poses anyway");
    LOGI("client connected");
    return c;
}

// no stopVR: VR mode is service-global, another process may still need it
static void qvrClose(QvrClient* c) {
    if (c->dtor && c->obj) c->dtor(c->obj);
    free(c->obj);
    if (c->lib) dlclose(c->lib);
    free(c);
}

// GetHeadTrackingData dereferences the dead service's binder state and
// segfaults: it must never run while qvrd is down. An unset property means
// a differently-named service on another setup - don't gate those.
static bool qvrServiceUp() {
    char v[PROP_VALUE_MAX] = {0};
    __system_property_get("init.svc.pn2_qvrd", v);
    return v[0] == '\0' || strcmp(v, "running") == 0;
}

void qvrPoll(Engine* e) {
    if (!qvrServiceUp()) {
        if (e->qvrClient) {
            LOGW("qvrd down, dropping client");
            qvrClose((QvrClient*)e->qvrClient);
            e->qvrClient = nullptr;
        }
        e->qvrState = -1;
        e->headPosValid = false;
        e->quatFromQvr = false;
        return;
    }
    static int s_retryWait = 0;
    if (!e->qvrClient) {
        if (s_retryWait > 0) {
            --s_retryWait;
            e->qvrState = -1;
            e->headPosValid = false;
            e->quatFromQvr = false;
            return;
        }
        e->qvrClient = qvrOpen();
        if (!e->qvrClient) {
            s_retryWait = 72;
            e->headPosValid = false;
            e->quatFromQvr = false;
            e->qvrState = -1;
            return;
        }
    }
    QvrClient* c = (QvrClient*)e->qvrClient;
    QvrPose* p = nullptr;
    if (c->getPose(c->impl, &p) != 0 || !p) {
        e->qvrState = -1;
        e->headPosValid = false;
        e->quatFromQvr = false;   // dead client: let rot-vec take over again
        return;
    }
    // a dead service keeps the last pose readable at st=3 forever: repeated
    // timestamps are the only liveness signal left. a long empty stretch
    // (state 0 while the service is up) also earns a reconnect - the fresh
    // client re-requests positional mode, which kicks a wedged tracker
    c->stall = qvrStallTick(c->stall, c->lastTs, p->timestamp);
    c->lastTs = p->timestamp;
    c->empty = p->state == 0 ? c->empty + 1 : 0;
    if (c->stall > 90 || c->empty > 72 * 15) {
        LOGW("pose stream %s, reconnecting", c->stall > 90 ? "stalled" : "idle");
        qvrClose(c);
        e->qvrClient = nullptr;
        e->qvrState = -1;
        e->headPosValid = false;
        e->quatFromQvr = false;
        return;
    }
    e->qvrState = (int)p->state;
    switch (qvrClassify(p->state)) {
    case QVR_TRACKED:
        qvrFoldPose(e->quat, e->headPos, &e->quatFromQvr, p->quat, p->pos);
        e->haveQuat = true;
        e->headPosValid = true;
        break;
    case QVR_DEGRADED:
        // degraded samples carry an identity pose: hold the last real
        // position and hand orientation back to rot-vec so the view keeps
        // moving instead of snapping to origin or freezing
        e->quatFromQvr = false;
        break;
    default:
        e->headPosValid = false;
        e->quatFromQvr = false;
        break;
    }
    static int s_n = 0;
    if (propI("debug.vrhome.qvrlog", 0) && ++s_n % 72 == 0)
        LOGI("st %d rv %.3f %.3f %.3f %.3f qvr %.3f %.3f %.3f %.3f pos %.3f %.3f %.3f w %.3f %.3f %.3f hz %d",
             e->qvrState,
             e->quat[0], e->quat[1], e->quat[2], e->quat[3],
             p->quat[0], p->quat[1], p->quat[2], p->quat[3],
             p->pos[0], p->pos[1], p->pos[2],
             e->headPos[0], e->headPos[1], e->headPos[2], e->sensorHz);
}
