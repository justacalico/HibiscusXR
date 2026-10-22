#include "vrmath.h"

#include "../input/keys.h"
#include "../shell/backend.h"

#include <cmath>
#include <cstring>

// stick deflection under this is treated as centered
constexpr float kStickDead = 0.25f;
// scroll delta per frame at full stick - one wheel notch is ~1.0 on the
// desktop path and a held stick should sit just under that cadence
constexpr float kStickScroll = 0.05f;

Mat4 vrView(const VrPose& p) {
    Mat4 v = quatToMat(p.q, true);
    for (int i = 0; i < 3; ++i)
        v.m[12 + i] = -(v.m[i] * p.p[0] + v.m[4 + i] * p.p[1] +
                        v.m[8 + i] * p.p[2]);
    return v;
}

void vrRay(const VrPose& p, float o[3], float d[3]) {
    const float fwd[3] = {0.0f, 0.0f, -1.0f};
    memcpy(o, p.p, sizeof(float) * 3);
    quatRotate(p.q, fwd, d);
}

Mat4 vrFovProj(float angL, float angR, float angD, float angU,
               float zn, float zf) {
    const float tl = tanf(angL), tr = tanf(angR);
    const float td = tanf(angD), tu = tanf(angU);
    const float w = tr - tl, h = tu - td;
    Mat4 r{};
    r.m[0] = 2.0f / w;
    r.m[5] = 2.0f / h;
    r.m[8] = (tr + tl) / w;
    r.m[9] = (tu + td) / h;
    r.m[10] = (zf + zn) / (zn - zf);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zf * zn) / (zn - zf);
    return r;
}

void vrEyesFallback(const VrPose& head, float ipd, VrPose eyes[2]) {
    const float right[3] = {1.0f, 0.0f, 0.0f};
    float r[3];
    quatRotate(head.q, right, r);
    for (int i = 0; i < 2; ++i) {
        eyes[i] = head;
        const float s = (i == 0 ? -0.5f : 0.5f) * ipd;
        eyes[i].p[0] += r[0] * s;
        eyes[i].p[1] += r[1] * s;
        eyes[i].p[2] += r[2] * s;
    }
}

static void pushKey(VrEvents* ev, int code, int action) {
    if (ev->nKeys < (int)(sizeof(ev->keys) / sizeof(ev->keys[0]))) {
        ev->keys[ev->nKeys].code = code;
        ev->keys[ev->nKeys].action = action;
        ++ev->nKeys;
    }
}

VrEvents vrInputTick(const VrInput& prev, const VrInput& cur,
                     int lastActive) {
    VrEvents ev;
    for (int w = 0; w < 2; ++w) {
        if (!cur.aim[w]) continue;
        ev.conn[w] = true;
        if (cur.sel[w] != prev.sel[w])
            pushKey(&ev, kKeyEnter, cur.sel[w] ? kKeyDown : kKeyUp);
        if (cur.sq[w] != prev.sq[w])
            pushKey(&ev, kKeyMenu, cur.sq[w] ? kKeyDown : kKeyUp);
        if (cur.aux[w] != prev.aux[w])
            pushKey(&ev, kKeyMinimize, cur.aux[w] ? kKeyDown : kKeyUp);
        if (cur.menu[w] && !prev.menu[w])
            pushKey(&ev, kCtrlRecenter, kKeyDown);
        ev.scroll += cur.stick[w][1];
    }
    // a fresh confirm edge hands the pointer to that controller, same as
    // the device arbitration - the last press wins
    for (int w = 0; w < 2; ++w)
        if (ev.conn[w] && cur.sel[w] != prev.sel[w]) ev.active = w;
    if (ev.active < 0) {
        if (lastActive >= 0 && ev.conn[lastActive])
            ev.active = lastActive;
        else if (ev.conn[CTRL_LEFT]) ev.active = CTRL_LEFT;
        else if (ev.conn[CTRL_RIGHT]) ev.active = CTRL_RIGHT;
    }
    if (fabsf(ev.scroll) < kStickDead)
        ev.scroll = 0.0f;
    else
        ev.scroll = -ev.scroll * kStickScroll;
    return ev;
}
