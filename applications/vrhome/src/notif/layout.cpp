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
    if ((int)out.size() > kShadeMax) out.resize(kShadeMax);
    return out;
}

std::vector<NotifItem> visibleNotifs(const std::vector<NotifItem>& in,
                                     long long nowMs) {
    std::vector<NotifItem> out;
    for (const auto& it : in) {
        // postMs <= 0 means the platform gave no time: keep it visible
        if (it.postMs <= 0 || nowMs - it.postMs <= kNotifShowMs)
            out.push_back(it);
        if ((int)out.size() >= kNotifMax) break;
    }
    return out;
}

float notifStackHH(int count) {
    if (count <= 0) return 0.0f;
    return (count * kNotifCardH + (count - 1) * kNotifGap) * 0.5f;
}

float notifLift(int count) {
    return notifLiftAbove(count, kDockBarH * 0.5f);
}

float notifLiftAbove(int count, float clear) {
    return clear + kNotifGap + notifStackHH(count);
}

void notifCenter(float yaw, float pitch, float lift, float dist,
                 const float origin[3],
                 float c[3], float r[3], float up[3]) {
    dockCenter(yaw, pitch, dist, origin, c, r, up);
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

bool rayNotif(float yaw, float pitch, float lift, float dist,
              const float origin[3],
              const float o[3], const float d[3], int count,
              float* u, float* v, float* t) {
    float c[3], r[3], up[3];
    notifCenter(yaw, pitch, lift, dist, origin, c, r, up);
    return rayQuad(c, r, up, origin, o, d, kNotifCardW * 0.5f,
                   notifStackHH(count), u, v, t);
}

NotifPick pickNotifRay(const std::vector<NotifItem>& items,
                       float yaw, float pitch, float lift, float dist,
                       const float origin[3], const float o[3],
                       const float d[3]) {
    NotifPick pk;
    if (items.empty()) return pk;
    float u, v, t;
    if (!rayNotif(yaw, pitch, lift, dist, origin, o, d,
                  (int)items.size(), &u, &v, &t))
        return pk;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return pk;
    pk.stack = true;
    pk.t = t;
    pk.idx = notifAt(u, v, (int)items.size(), &pk.zone);
    return pk;
}

NotifPick pickNotif(const std::vector<NotifItem>& items,
                    float yaw, float pitch, float lift, float dist,
                    const Mat4& head, const float origin[3],
                    const float o[3]) {
    float d[3];
    gazeDir(head, d);
    return pickNotifRay(items, yaw, pitch, lift, dist, origin, o, d);
}

// --------------------------------------------------------- the shade

void shadeCenter(float yaw, float pitch, float dist, const float origin[3],
                 float c[3], float r[3], float up[3]) {
    // same anchor cylinder the windows and the other cards ride, at the
    // card's share of the ring's radius
    ringPoint(yaw, pitch, dist * (kShadeDist / kPanelDist), kPanelY,
              origin, c, r, up);
}

void shadeClipBand(float* lo, float* hi) {
    *lo = -kShadeHH;
    *hi = kShadeHH - kShadeHeadH;
}

float shadeRowY(int i, float scroll) {
    float lo, hi;
    shadeClipBand(&lo, &hi);
    return hi - kShadeRowH * 0.5f -
           i * (kShadeRowH + kShadeRowGap) + scroll;
}

bool shadeRowShown(float ry, float gs) {
    float lo, hi;
    shadeClipBand(&lo, &hi);
    const float half = kShadeRowH * 0.5f * gs;
    return ry + half >= lo * gs - 0.02f && ry - half <= hi * gs + 0.02f;
}

float shadeScrollMax(int n) {
    if (n <= 0) return 0.0f;
    const float content = n * kShadeRowH + (n - 1) * kShadeRowGap;
    float lo, hi;
    shadeClipBand(&lo, &hi);
    const float over = content - (hi - lo);
    return over > 0.0f ? over : 0.0f;
}

float shadeClampScroll(float scroll, int n) {
    const float mx = shadeScrollMax(n);
    return scroll < 0.0f ? 0.0f : scroll > mx ? mx : scroll;
}

float shadeScrollBy(float scroll, float delta, int n) {
    return shadeClampScroll(scroll + delta * kShadeWheelStep, n);
}

bool shadeSlopPassed(float grabV, float v) {
    return fabsf(v - grabV) * kShadeHH > kShadeDragSlop;
}

void shadeBadgeAt(float* bx, float* by) {
    *bx = kShadeHW - kShadePad - kShadeBadgeR - 0.010f;
    *by = 0.0f;
}

float shadeTitleY() {
    return kShadeHH - kShadeHeadH * 0.5f;
}

float shadeCloseX() {
    return kShadeHW - kShadePad - kShadeCloseR;
}

void shadeClearPill(float labelHW, float* x, float* y,
                    float* hw, float* hh) {
    *hw = labelHW + kShadeClearPadX;
    *hh = kShadeClearH;
    *x = shadeCloseX() - kShadeCloseR - 0.016f - *hw;
    *y = shadeTitleY();
}

bool shadeClearable(const std::vector<NotifItem>& items) {
    for (auto& it : items) if (it.clearable) return true;
    return false;
}

int shadeZoneAt(const std::vector<NotifItem>& items, float u, float v,
                float scroll, float clearHW, int* idx) {
    *idx = -1;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return SHZONE_NONE;
    const float x = u * kShadeHW, y = v * kShadeHH;
    if (y > kShadeHH - kShadeHeadH) {
        const float ty = shadeTitleY();
        const float cdx = x - shadeCloseX(), cdy = y - ty;
        const float cr = kShadeCloseR + 0.012f;
        if (cdx * cdx + cdy * cdy <= cr * cr) return SHZONE_CLOSE;
        float px, py, phw, phh;
        shadeClearPill(clearHW, &px, &py, &phw, &phh);
        if (fabsf(x - px) <= phw + 0.006f &&
                fabsf(y - py) <= phh + 0.006f)
            return SHZONE_CLEAR;
        return SHZONE_BODY;
    }
    float lo, hi;
    shadeClipBand(&lo, &hi);
    if (y >= lo && fabsf(x) <= kShadeHW - kShadePad) {
        for (int i = 0; i < (int)items.size(); ++i) {
            const float ry = shadeRowY(i, scroll);
            if (fabsf(y - ry) > kShadeRowH * 0.5f) continue;
            *idx = i;
            if (items[i].clearable) {
                float bx, by;
                shadeBadgeAt(&bx, &by);
                const float dx = x - bx, dy = y - ry - by;
                const float rr = kShadeBadgeR + 0.012f;
                if (dx * dx + dy * dy <= rr * rr) return SHZONE_DISMISS;
            }
            return SHZONE_ROW;
        }
    }
    return SHZONE_BODY;
}

ShadePick pickShadeRay(const std::vector<NotifItem>& items, float scroll,
                       float clearHW, float yaw, float pitch, float dist,
                       const float origin[3], const float o[3],
                       const float d[3]) {
    ShadePick pk;
    float c[3], r[3], up[3];
    shadeCenter(yaw, pitch, dist, origin, c, r, up);
    if (!rayQuad(c, r, up, origin, o, d, kShadeHW, kShadeHH,
                 &pk.u, &pk.v, &pk.t)) return pk;
    if (fabsf(pk.u) > 1.0f || fabsf(pk.v) > 1.0f) return pk;
    pk.hit = true;
    pk.zone = shadeZoneAt(items, pk.u, pk.v, scroll, clearHW, &pk.idx);
    return pk;
}
