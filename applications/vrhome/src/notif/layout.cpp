#include "layout.h"

#include "../common/config.h"
#include "../dock/layout.h"
#include "../math/head.h"
#include "../panels/layout.h"

#include <algorithm>
#include <cmath>

std::vector<NotifItem> buildNotifs(const std::vector<NotifItem>& in) {
    std::vector<NotifItem> out = in;
    std::stable_sort(out.begin(), out.end(),
                     [](const NotifItem& a, const NotifItem& b) {
                         return a.postMs > b.postMs;
                     });
    if ((int)out.size() > kNotifMax) out.resize(kNotifMax);
    return out;
}

std::vector<NotifItem> visibleNotifs(const std::vector<NotifItem>& in,
                                     long long nowMs) {
    std::vector<NotifItem> out;
    for (const auto& it : in) {
        // postMs <= 0 means the platform gave no time: keep it visible
        if (it.postMs <= 0 || nowMs - it.postMs <= kNotifShowMs)
            out.push_back(it);
    }
    return out;
}

float notifStackHH(int count) {
    if (count <= 0) return 0.0f;
    return (count * kNotifCardH + (count - 1) * kNotifGap) * 0.5f;
}

float notifLift(int count) {
    return kDockBarH * 0.5f + kNotifGap + notifStackHH(count);
}

void notifCenter(float yaw, float pitch, float lift, const float origin[3],
                 float c[3], float r[3], float up[3]) {
    dockCenter(yaw, pitch, origin, c, r, up);
    for (int i = 0; i < 3; ++i) c[i] += up[i] * lift;
}

float notifCardY(int i, int count) {
    return notifStackHH(count) - kNotifCardH * 0.5f
           - i * (kNotifCardH + kNotifGap);
}

void notifBadgeAt(float* bx, float* by) {
    *bx = kNotifCardW * 0.5f - kNotifPad - kNotifBadgeR;
    *by = kNotifCardH * 0.5f - kNotifBadgeR - 0.008f;
}

int notifAt(float u, float v, int count, int* zone) {
    *zone = NZONE_NONE;
    const float sh = notifStackHH(count);
    if (sh <= 0.0f || fabsf(u) > 1.0f || fabsf(v) > 1.0f) return -1;
    const float x = u * kNotifCardW * 0.5f;
    const float y = v * sh;
    for (int i = 0; i < count; ++i) {
        const float yc = notifCardY(i, count);
        if (fabsf(y - yc) > kNotifCardH * 0.5f) continue;
        float bx, by;
        notifBadgeAt(&bx, &by);
        const float dx = x - bx, dy = y - yc - by;
        const float rr = kNotifBadgeR + 0.012f;
        if (dx * dx + dy * dy <= rr * rr) {
            *zone = NZONE_CLOSE;
            return i;
        }
        *zone = NZONE_BODY;
        return i;
    }
    return -1;
}

bool rayNotif(float yaw, float pitch, float lift, const float origin[3],
              const float o[3], const float d[3], int count,
              float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    notifCenter(yaw, pitch, lift, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, kNotifCardW * 0.5f,
                   notifStackHH(count), u, v, t);
}

NotifPick pickNotifRay(const std::vector<NotifItem>& items,
                       float yaw, float pitch, float lift,
                       const float origin[3], const float o[3],
                       const float d[3]) {
    NotifPick pk;
    if (items.empty()) return pk;
    float u, v, t;
    if (!rayNotif(yaw, pitch, lift, origin, o, d, (int)items.size(),
                  &u, &v, &t))
        return pk;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return pk;
    pk.stack = true;
    pk.t = t;
    pk.idx = notifAt(u, v, (int)items.size(), &pk.zone);
    return pk;
}

NotifPick pickNotif(const std::vector<NotifItem>& items,
                    float yaw, float pitch, float lift,
                    const Mat4& head, const float origin[3],
                    const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickNotifRay(items, yaw, pitch, lift, origin, o, d);
}
