#include "test.h"

#include "common/config.h"
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

static void testVrFovProj() {
    // a symmetric fov must match perspective()'s output exactly: the two
    // feed the same shaders and the same clip space
    const float a = 0.65f;
    const Mat4 s = vrFovProj(-a, a, -a, a, 0.05f, 100.0f);
    const Mat4 p = perspective(2.0f * a * 180.0f / (float)M_PI, 1.0f,
                               0.05f, 100.0f);
    for (int i = 0; i < 16; ++i)
        CHECK_F(s.m[i], p.m[i], 1e-5f);

    // an asymmetric fov shifts the image centre off the axis
    const Mat4 o = vrFovProj(-0.4f, 0.9f, -0.5f, 0.6f, 0.05f, 100.0f);
    CHECK_F(o.m[0], 2.0f / (tanf(0.9f) + tanf(0.4f)), 1e-4f);
    CHECK(o.m[8] > 0.0f);   // right angle wider: frustum skews +X
    CHECK(o.m[9] > 0.0f);
}

static void testVrEyesFallback() {
    // identity head facing -Z: eyes straddle on world X
    VrPose eyes[2];
    vrEyesFallback(poseAt(0.0f, 1.6f, 0.0f), 0.064f, eyes);
    CHECK_F(eyes[0].p[0], -0.032f, 1e-4f);
    CHECK_F(eyes[1].p[0], 0.032f, 1e-4f);
    CHECK_F(eyes[0].p[1], 1.6f, 1e-4f);
    CHECK_F(eyes[1].p[1], 1.6f, 1e-4f);
    CHECK(eyes[0].ok && eyes[1].ok);

    // head turned 90deg left (facing -X): the split axis turns with it
    VrPose h = poseAt(0.0f, 1.6f, 0.0f);
    const float s = sinf((float)M_PI / 4.0f);
    h.q[1] = s; h.q[3] = s;      // rotY(90deg): right axis -> -Z
    vrEyesFallback(h, 0.064f, eyes);
    CHECK_F(eyes[0].p[2], 0.032f, 1e-4f);
    CHECK_F(eyes[1].p[2], -0.032f, 1e-4f);
    CHECK_F(eyes[0].p[0], 0.0f, 1e-4f);
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
    VrSummon sum;
    VrEvents ev = vrInputTick(a, b, -1, &sum, 0);
    const InputEvent* k = findKey(ev, kKeyEnter);
    CHECK(k != nullptr && k->action == kKeyDown);
    // the pressing hand takes the pointer
    CHECK(ev.active == CTRL_RIGHT);
    // held -> no new edge next frame
    ev = vrInputTick(b, b, CTRL_RIGHT, &sum, 0);
    CHECK(findKey(ev, kKeyEnter) == nullptr);
    // release edge emits up
    VrInput c = b;
    c.sel[1] = false;
    ev = vrInputTick(b, c, CTRL_RIGHT, &sum, 0);
    const InputEvent* up = findKey(ev, kKeyEnter);
    CHECK(up != nullptr && up->action == kKeyUp);
}

static void testVrInputArb() {
    VrInput a{};
    a.aim[0] = a.aim[1] = true;
    VrSummon sum;
    // both connected, nobody pressed: keep the last active hand
    VrEvents ev = vrInputTick(a, a, CTRL_LEFT, &sum, 0);
    CHECK(ev.active == CTRL_LEFT);
    // left goes away: arbitration falls to the connected hand
    VrInput c = a;
    c.aim[0] = false;
    ev = vrInputTick(a, c, CTRL_LEFT, &sum, 0);
    CHECK(ev.active == CTRL_RIGHT);
    CHECK(ev.conn[0] == false && ev.conn[1] == true);
    // neither connected: no pointer
    VrInput z{};
    ev = vrInputTick(c, z, CTRL_RIGHT, &sum, 0);
    CHECK(ev.active == -1);
}

static void testVrInputButtons() {
    VrInput a{}, b{};
    a.aim[0] = true;
    b.aim[0] = true;
    b.sq[0] = true;
    b.aux[0] = true;
    VrSummon sum;
    VrEvents ev = vrInputTick(a, b, -1, &sum, 0);
    CHECK(findKey(ev, kKeyMenu) != nullptr &&
          findKey(ev, kKeyMenu)->action == kKeyDown);
    CHECK(findKey(ev, kKeyMinimize) != nullptr &&
          findKey(ev, kKeyMinimize)->action == kKeyDown);
    // a disconnected hand never emits keys
    VrInput d{};
    ev = vrInputTick(d, d, -1, &sum, 0);
    CHECK(ev.nKeys == 0);
}

static void testVrInputSummon() {
    VrInput off{}, on{};
    off.aim[0] = on.aim[0] = true;
    on.menu[0] = true;
    VrSummon sum;

    // the down edge only starts the hold clock
    VrEvents ev = vrInputTick(off, on, -1, &sum, 1000);
    CHECK(ev.summon == 0 && sum.shown && sum.downMs == 1000);
    // a short press releases before the fill: the layer drops
    ev = vrInputTick(on, off, -1, &sum, 1100);
    CHECK(ev.summon == kSumHide && !sum.shown);
    // while the dash is down nothing else reaches it
    VrInput b = off;
    b.sel[0] = true;
    b.stick[0][1] = 1.0f;
    ev = vrInputTick(off, b, -1, &sum, 1200);
    CHECK(ev.nKeys == 0 && ev.scroll == 0.0f);
    // another tap raises it again
    vrInputTick(off, on, -1, &sum, 2000);
    ev = vrInputTick(on, off, -1, &sum, 2100);
    CHECK(ev.summon == kSumShow && sum.shown);
    // a hold fills the ring and recentres once; the release after it is
    // inert, not a second toggle
    vrInputTick(off, on, -1, &sum, 3000);
    ev = vrInputTick(on, on, -1, &sum, 3000 + kHoldMs);
    CHECK(ev.summon == kSumRecenter && sum.shown);
    ev = vrInputTick(on, on, -1, &sum, 3000 + kHoldMs + 200);
    CHECK(ev.summon == 0);
    ev = vrInputTick(on, off, -1, &sum, 4000);
    CHECK(ev.summon == 0 && sum.shown);
    // holding the key while the dash is down raises it with the recenter
    vrInputTick(off, on, -1, &sum, 5000);
    vrInputTick(on, off, -1, &sum, 5100);
    CHECK(!sum.shown);
    vrInputTick(off, on, -1, &sum, 6000);
    ev = vrInputTick(on, on, -1, &sum, 6000 + kHoldMs);
    CHECK(ev.summon == (kSumShow | kSumRecenter) && sum.shown);
    ev = vrInputTick(on, off, -1, &sum, 7000);
    CHECK(ev.summon == 0);
    // either hand's start button counts as the same key
    VrInput on2{};
    on2.aim[1] = true;
    on2.menu[1] = true;
    vrInputTick(off, on2, -1, &sum, 8000);
    ev = vrInputTick(on2, off, -1, &sum, 8100);
    CHECK(ev.summon == kSumHide && !sum.shown);
    // a dropped hand mid-press doesn't leave the hold clock stuck
    VrInput z{};
    vrInputTick(off, on, -1, &sum, 9000);
    vrInputTick(on, on, -1, &sum, 9100);
    ev = vrInputTick(on, z, -1, &sum, 9200);
    CHECK(ev.summon == kSumShow && sum.shown && sum.downMs == 0);
}

static void testVrInputScroll() {
    VrInput a{}, b{};
    a.aim[0] = true;
    b.aim[0] = true;
    b.stick[0][1] = 1.0f;
    VrSummon sum;
    VrEvents ev = vrInputTick(a, b, CTRL_LEFT, &sum, 0);
    CHECK(ev.scroll < 0.0f);
    // inside the deadzone: still
    b.stick[0][1] = 0.1f;
    ev = vrInputTick(a, b, CTRL_LEFT, &sum, 0);
    CHECK(ev.scroll == 0.0f);
    // disconnected hands don't scroll
    VrInput z{};
    z.stick[0][1] = 1.0f;
    ev = vrInputTick(a, z, CTRL_LEFT, &sum, 0);
    CHECK(ev.scroll == 0.0f);
}

void testVr() {
    testVrFovProj();
    testVrEyesFallback();
    testVrView();
    testVrRay();
    testVrInputClick();
    testVrInputArb();
    testVrInputButtons();
    testVrInputSummon();
    testVrInputScroll();
}
