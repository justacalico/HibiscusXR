#include "qvr.h"

#include "../engine.h"
#include "../math/head.h"

#include <android/log.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

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

void qvrPoll(Engine* e) {
    if (!e->qvrClient) {
        e->qvrClient = qvrOpen();
        if (!e->qvrClient) { e->headPosValid = false; return; }
    }
    QvrClient* c = (QvrClient*)e->qvrClient;
    QvrPose* p = nullptr;
    if (c->getPose(c->impl, &p) != 0 || !p || p->state == 0) {
        e->headPosValid = false;
        return;
    }
    qvrFoldPose(e->quat, e->headPos, &e->quatFromQvr, e->sensorHz,
                p->quat, p->pos);
    e->haveQuat = true;
    e->headPosValid = true;
}
