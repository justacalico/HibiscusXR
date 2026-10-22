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

void vrQuadPlace(const VrPose& head, float dist, float w, float h,
                 VrQuad* out) {
    const float fwd[3] = {0.0f, 0.0f, -1.0f};
    float f[3];
    quatRotate(head.q, fwd, f);
    float hl = sqrtf(f[0] * f[0] + f[2] * f[2]);
    // looking straight up or down leaves no usable heading: face -Z in
    // stage space rather than recentering on noise
    if (hl < 1e-4f) {
        f[0] = 0.0f;
        f[2] = -1.0f;
        hl = 1.0f;
    }
    f[0] /= hl;
    f[2] /= hl;
    // the quad's +Z (its front face in XR quad-layer terms) points back at
    // the user, so yaw = atan2(-f.x, -f.z) for the horizontal look dir f
    out->yaw = atan2f(-f[0], -f[2]);
    out->p[0] = head.p[0] + f[0] * dist;
    out->p[1] = head.p[1] - 0.2f;
    out->p[2] = head.p[2] + f[2] * dist;
    out->w = w;
    out->h = h;
}

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

bool vrQuadHit(const VrQuad& qd, const float o[3], const float d[3],
               float* u, float* v, float* t) {
    const float cy = cosf(qd.yaw), sy = sinf(qd.yaw);
    // front-face normal is the quad's local +Z - it points at the user, so
    // an incoming ray must travel against it
    const float nx = sy, nz = cy;
    const float denom = d[0] * nx + d[2] * nz;
    if (denom > -1e-6f) return false;
    const float dx = qd.p[0] - o[0], dz = qd.p[2] - o[2];
    const float ht = (dx * nx + dz * nz) / denom;
    if (ht < 0.0f) return false;
    const float hx = o[0] + d[0] * ht - qd.p[0];
    const float hy = o[1] + d[1] * ht - qd.p[1];
    const float hz = o[2] + d[2] * ht - qd.p[2];
    // local right = (cos yaw, 0, -sin yaw) for a yaw-only quad
    const float lu = hx * cy - hz * sy;
    if (lu < -qd.w * 0.5f || lu > qd.w * 0.5f) return false;
    if (hy < -qd.h * 0.5f || hy > qd.h * 0.5f) return false;
    *u = lu / qd.w + 0.5f;
    *v = 0.5f - hy / qd.h;
    *t = ht;
    return true;
}

void vrUvRay(float u, float v, float fovYDeg, float aspect,
             float o[3], float d[3]) {
    const float t = tanf(fovYDeg * (float)M_PI / 360.0f);
    const float nx = (u * 2.0f - 1.0f) * t * aspect;
    const float ny = (1.0f - v * 2.0f) * t;
    const float il = 1.0f / sqrtf(nx * nx + ny * ny + 1.0f);
    o[0] = o[1] = o[2] = 0.0f;
    d[0] = nx * il;
    d[1] = ny * il;
    d[2] = -il;
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
