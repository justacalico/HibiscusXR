#include "test.h"

#include "desktop/vrmath.h"
#include "input/keys.h"
#include "input/input_state.h"
#include "shell/backend.h"

#include <cmath>
#include <cstring>

static VrPose poseAt(float x, float y, float z) {
    VrPose p;
    p.p[0] = x; p.p[1] = y; p.p[2] = z;
    p.ok = true;
    return p;
}

static void testVrQuadPlace() {
    // identity head at origin facing -Z: quad lands ahead, facing back
    VrQuad q;
    vrQuadPlace(poseAt(0.0f, 1.6f, 0.0f), 1.4f, 1.6f, 0.9f, &q);
    CHECK_F(q.p[0], 0.0f, 1e-4f);
    CHECK_F(q.p[1], 1.4f, 1e-4f);
    CHECK_F(q.p[2], -1.4f, 1e-4f);
    CHECK_F(q.yaw, 0.0f, 1e-4f);
    CHECK_F(q.w, 1.6f, 1e-4f);
    CHECK_F(q.h, 0.9f, 1e-4f);

    // yaw 90deg left (head facing -X): quad ahead in -X, +Z back at user
    VrPose h = poseAt(0.0f, 1.6f, 0.0f);
    const float s = sinf((float)M_PI / 4.0f);
    h.q[1] = s; h.q[3] = s;      // rotY(90deg): -Z fwd -> -X
    vrQuadPlace(h, 1.0f, 1.0f, 1.0f, &q);
    CHECK_F(q.p[0], -1.0f, 1e-4f);
    CHECK_F(q.p[2], 0.0f, 1e-4f);

    // straight-down gaze: fallback placement instead of NaN
    VrPose d = poseAt(0.0f, 1.6f, 0.0f);
    d.q[0] = -s; d.q[3] = s;     // pitch -90
    vrQuadPlace(d, 1.0f, 1.0f, 1.0f, &q);
    CHECK(q.p[2] < 0.0f);
}

static void testVrView() {
    VrPose p = poseAt(1.0f, 2.0f, 3.0f);
    const Mat4 v = vrView(p);
    CHECK_F(v.m[0], 1.0f, 1e-5f);
    CHECK_F(v.m[5], 1.0f, 1e-5f);
    CHECK_F(v.m[10], 1.0f, 1e-5f);
    CHECK_F(v.m[12], -1.0f, 1e-5f);
    CHECK_F(v.m[13], -2.0f, 1e-5f);
    CHECK_F(v.m[14], -3.0f, 1e-5f);
}

static void testVrRay() {
    VrPose p = poseAt(1.0f, 1.0f, 1.0f);
    float o[3], d[3];
    vrRay(p, o, d);
    CHECK_F(o[0], 1.0f, 1e-5f);
    CHECK_F(d[0], 0.0f, 1e-5f);
    CHECK_F(d[1], 0.0f, 1e-5f);
    CHECK_F(d[2], -1.0f, 1e-5f);
}

static void testVrQuadHit() {
    VrQuad q;
    vrQuadPlace(poseAt(0.0f, 0.0f, 0.0f), 1.0f, 1.0f, 1.0f, &q);
    // quad at z=-1 facing +Z; a ray straight at its centre hits u=v=0.5
    const float o[3] = {0.0f, 0.0f, 0.0f};
    float u = -1, v = -1, t = -1;
    const float d0[3] = {0.0f, -0.2f, -1.0f};
    CHECK(vrQuadHit(q, o, d0, &u, &v, &t));
    CHECK_F(u, 0.5f, 1e-3f);
    CHECK_F(v, 0.5f, 1e-3f);
    CHECK(t > 0.9f && t < 1.1f);
    // off the right edge: miss
    const float d1[3] = {2.0f, 0.0f, -1.0f};
    CHECK(!vrQuadHit(q, o, d1, &u, &v, &t));
    // behind the ray: miss
    const float d2[3] = {0.0f, 0.0f, 1.0f};
    CHECK(!vrQuadHit(q, o, d2, &u, &v, &t));
    // from behind the quad (ray travels with the normal): miss
    const float o3[3] = {0.0f, 0.0f, -2.0f};
    const float d3[3] = {0.0f, 0.0f, 1.0f};
    CHECK(!vrQuadHit(q, o3, d3, &u, &v, &t));
}

static void testVrUvRay() {
    float o[3], d[3];
    vrUvRay(0.5f, 0.5f, 50.0f, 16.0f / 9.0f, o, d);
    CHECK_F(o[0], 0.0f, 1e-5f);
    CHECK_F(d[0], 0.0f, 1e-4f);
    CHECK_F(d[1], 0.0f, 1e-4f);
    CHECK_F(d[2], -1.0f, 1e-4f);
    // uv (1,0) is the top-right corner: +x, +y
    vrUvRay(1.0f, 0.0f, 50.0f, 16.0f / 9.0f, o, d);
    CHECK(d[0] > 0.0f);
    CHECK(d[1] > 0.0f);
    CHECK(d[2] < 0.0f);
    // and the ray is normalised
    CHECK_F(d[0]*d[0] + d[1]*d[1] + d[2]*d[2], 1.0f, 1e-5f);
}

static const InputEvent* findKey(const VrEvents& ev, int code) {
    for (int i = 0; i < ev.nKeys; ++i)
        if (ev.keys[i].code == code) return &ev.keys[i];
    return nullptr;
}

static void testVrInputClick() {
    VrInput a{}, b{};
    a.aim[0] = a.aim[1] = true;
    b.aim[0] = b.aim[1] = true;
    b.sel[1] = true;
    VrEvents ev = vrInputTick(a, b, -1);
    const InputEvent* k = findKey(ev, kKeyEnter);
    CHECK(k != nullptr && k->action == kKeyDown);
    // the pressing hand takes the pointer
    CHECK(ev.active == CTRL_RIGHT);
    // held -> no new edge next frame
    ev = vrInputTick(b, b, CTRL_RIGHT);
    CHECK(findKey(ev, kKeyEnter) == nullptr);
    // release edge emits up
    VrInput c = b;
    c.sel[1] = false;
    ev = vrInputTick(b, c, CTRL_RIGHT);
    const InputEvent* up = findKey(ev, kKeyEnter);
    CHECK(up != nullptr && up->action == kKeyUp);
}

static void testVrInputArb() {
    VrInput a{};
    a.aim[0] = a.aim[1] = true;
    // both connected, nobody pressed: keep the last active hand
    VrEvents ev = vrInputTick(a, a, CTRL_LEFT);
    CHECK(ev.active == CTRL_LEFT);
    // left goes away: arbitration falls to the connected hand
    VrInput c = a;
    c.aim[0] = false;
    ev = vrInputTick(a, c, CTRL_LEFT);
    CHECK(ev.active == CTRL_RIGHT);
    CHECK(ev.conn[0] == false && ev.conn[1] == true);
    // neither connected: no pointer
    VrInput z{};
    ev = vrInputTick(c, z, CTRL_RIGHT);
    CHECK(ev.active == -1);
}

static void testVrInputButtons() {
    VrInput a{}, b{};
    a.aim[0] = true;
    b.aim[0] = true;
    b.sq[0] = true;
    b.aux[0] = true;
    b.menu[0] = true;
    VrEvents ev = vrInputTick(a, b, -1);
    CHECK(findKey(ev, kKeyMenu) != nullptr &&
          findKey(ev, kKeyMenu)->action == kKeyDown);
    CHECK(findKey(ev, kKeyMinimize) != nullptr &&
          findKey(ev, kKeyMinimize)->action == kKeyDown);
    CHECK(findKey(ev, kCtrlRecenter) != nullptr &&
          findKey(ev, kCtrlRecenter)->action == kKeyDown);
    // a disconnected hand never emits keys
    VrInput d{};
    ev = vrInputTick(d, d, -1);
    CHECK(ev.nKeys == 0);
}

static void testVrInputScroll() {
    VrInput a{}, b{};
    a.aim[0] = true;
    b.aim[0] = true;
    b.stick[0][1] = 1.0f;
    VrEvents ev = vrInputTick(a, b, CTRL_LEFT);
    CHECK(ev.scroll < 0.0f);
    // inside the deadzone: still
    b.stick[0][1] = 0.1f;
    ev = vrInputTick(a, b, CTRL_LEFT);
    CHECK(ev.scroll == 0.0f);
    // disconnected hands don't scroll
    VrInput z{};
    z.stick[0][1] = 1.0f;
    ev = vrInputTick(a, z, CTRL_LEFT);
    CHECK(ev.scroll == 0.0f);
}

void testVr() {
    testVrQuadPlace();
    testVrView();
    testVrRay();
    testVrQuadHit();
    testVrUvRay();
    testVrInputClick();
    testVrInputArb();
    testVrInputButtons();
    testVrInputScroll();
}
