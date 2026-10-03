#include "layout.h"

#include "../common/config.h"
#include "../dock/layout.h"
#include "../math/head.h"
#include "../panels/layout.h"

#include <cmath>

void sysMsgCenter(float yaw, float pitch, float lift, const float origin[3],
                  float c[3], float r[3], float up[3]) {
    dockCenter(yaw, pitch, origin, c, r, up);
    for (int i = 0; i < 3; ++i) c[i] += up[i] * lift;
}

bool sysMsgModal(bool coveredOnly, const std::vector<SysMsgItem>& items) {
    return coveredOnly || !items.empty();
}

void sysMsgAnchor(bool coveredOnly, float ownYaw, float dashYaw,
                  float dashPitch, float* yaw, float* pitch, float* lift) {
    *yaw = coveredOnly ? ownYaw : dashYaw;
    *pitch = coveredOnly ? kSysMsgPitch : dashPitch;
    *lift = coveredOnly ? 0.0f : kSysMsgLift;
}

float sysMsgBtnHW(int n) {
    const float iw = kSysMsgW * 0.5f - kSysMsgPad;
    if (n <= 0) return iw;
    return (iw - (n - 1) * kSysMsgBtnGap * 0.5f) / n;
}

float sysMsgBtnX(int i, int n) {
    const float iw = kSysMsgW * 0.5f - kSysMsgPad;
    const float hw = sysMsgBtnHW(n);
    return -iw + hw + i * (2.0f * hw + kSysMsgBtnGap);
}

float sysMsgBtnY() {
    return -kSysMsgH * 0.5f + kSysMsgPad + kSysMsgBtnHH;
}

int sysMsgAt(float u, float v, int nbtn, int* btn) {
    *btn = -1;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return MZONE_NONE;
    const float x = u * kSysMsgW * 0.5f;
    const float y = v * kSysMsgH * 0.5f;
    if (nbtn > 0 && fabsf(y - sysMsgBtnY()) <=
            kSysMsgBtnHH + kSysMsgBtnSlack) {
        const float hw = sysMsgBtnHW(nbtn) + kSysMsgBtnSlack;
        for (int i = 0; i < nbtn; ++i) {
            if (fabsf(x - sysMsgBtnX(i, nbtn)) <= hw) {
                *btn = i;
                return MZONE_BTN;
            }
        }
    }
    return MZONE_BODY;
}

bool raySysMsg(float yaw, float pitch, float lift, const float origin[3],
               const float o[3], const float d[3],
               float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    sysMsgCenter(yaw, pitch, lift, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, kSysMsgW * 0.5f,
                   kSysMsgH * 0.5f, u, v, t);
}

SysMsgPick pickSysMsgRay(const std::vector<SysMsgItem>& items,
                         float yaw, float pitch, float lift,
                         const float origin[3], const float o[3],
                         const float d[3]) {
    SysMsgPick pk;
    if (items.empty()) return pk;
    float u, v, t;
    if (!raySysMsg(yaw, pitch, lift, origin, o, d, &u, &v, &t)) return pk;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return pk;
    pk.hit = true;
    pk.t = t;
    pk.zone = sysMsgAt(u, v, (int)items[0].buttons.size(), &pk.btn);
    return pk;
}

SysMsgPick pickSysMsg(const std::vector<SysMsgItem>& items,
                      float yaw, float pitch, float lift,
                      const Mat4& head, const float origin[3],
                      const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickSysMsgRay(items, yaw, pitch, lift, origin, o, d);
}
