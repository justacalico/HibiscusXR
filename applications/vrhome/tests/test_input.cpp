#include "test.h"

#include "input/ctrl_state.h"
#include "input/input_state.h"
#include "input/aim.h"
#include "input/keys.h"
#include "render/mesh.h"

#include <cstring>

// build a big-endian sharemem snapshot like the service writes it
static void putBE32(uint8_t* p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
static void putF32(uint8_t* p, float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    putBE32(p, u);
}
static void putI64(uint8_t* p, int64_t v) {
    for (int i = 7; i >= 0; --i) { p[i] = v & 0xff; v >>= 8; }
}

static void mkBlock(uint8_t* buf, int which, float x, int bat, int trig) {
    const int base = which * 512;
    buf[base] = CTRL_FLAG_DONE;
    putF32(buf + base + 4, x);
    putF32(buf + base + 16, 1.0f);            // fixed q0
    putBE32(buf + base + 32, 3);              // fixed status
    putI64(buf + base + 36, 12345);           // fixed ts
    putF32(buf + base + 44, x + 1.0f);        // fuse x
    putF32(buf + base + 56, 1.0f);            // fuse q0
    buf[base + 100] = CTRL_FLAG_DONE;
    putBE32(buf + base + 104, 128);           // touch x
    putBE32(buf + base + 108, 64);            // touch y
    putBE32(buf + base + 124, trig);          // trigger
    putBE32(buf + base + 128, bat);           // battery
}

void testInput() {
    // --- decoder on a synthetic snapshot ------------------------------
    uint8_t buf[CTRL_SHARE_SIZE];
    memset(buf, 0, sizeof(buf));
    mkBlock(buf, CTRL_LEFT, -100.0f, 74, 1);
    mkBlock(buf, CTRL_RIGHT, 100.0f, 93, 0);

    ctrl_state st;
    ctrl_state_decode(buf, CTRL_LEFT, &st);
    CHECK(st.pose_ok && st.keys_ok);
    CHECK_F(st.fixed.x, -100.0f, 1e-5f);
    CHECK_F(st.fixed.q0, 1.0f, 1e-5f);
    CHECK(st.fixed.status == 3);
    CHECK(st.fixed.timestamp == 12345);
    CHECK_F(st.fuse.x, -99.0f, 1e-5f);
    CHECK(st.keys.battery == 74);
    CHECK(st.keys.trigger == 1);
    CHECK(st.keys.touch_x == 128 && st.keys.touch_y == 64);

    ctrl_state_decode(buf, CTRL_RIGHT, &st);
    CHECK_F(st.fixed.x, 100.0f, 1e-5f);
    CHECK(st.keys.battery == 93 && st.keys.trigger == 0);

    // mid-write flags leave the decoded block empty
    buf[0] = CTRL_FLAG_WRITING;
    ctrl_state_decode(buf, CTRL_LEFT, &st);
    CHECK(!st.pose_ok && st.keys_ok);

    // the hash reacts to any byte change inside the block
    memset(buf, 0, sizeof(buf));
    mkBlock(buf, CTRL_LEFT, -100.0f, 74, 0);
    const uint64_t h1 = ctrl_state_hash(buf, CTRL_LEFT);
    buf[CTRL_LEFT * 512 + 124] = 1;
    CHECK(ctrl_state_hash(buf, CTRL_LEFT) != h1);
    CHECK(ctrl_state_hash(buf, CTRL_RIGHT) != h1);

    // --- arbitration ---------------------------------------------------
    InputState s;
    InputEvent ev[16];
    ctrl_state live;
    memset(&live, 0, sizeof(live));

    // nothing yet: hmd input is the pointer
    CHECK(hmdInput(s));
    CHECK(s.active == -1);

    // a block that keeps changing marks the controller connected and makes
    // it the pointer straight away
    int n = inputTick(s, CTRL_RIGHT, 100, live, 0, ev, 16);
    n += inputTick(s, CTRL_RIGHT, 101, live, 10, ev + n, 16 - n);
    CHECK(s.rightConnected && s.active == CTRL_RIGHT);
    CHECK(!hmdInput(s));

    // a trigger press on the other controller steals the pointer and emits
    // the confirm-down edge
    live.keys.trigger = 1;
    n = inputTick(s, CTRL_LEFT, 200, live, 20, ev, 16);
    n += inputTick(s, CTRL_LEFT, 201, live, 30, ev + n, 16 - n);
    CHECK(s.leftConnected && s.active == CTRL_LEFT);
    CHECK(n == 1 && ev[0].code == kKeyEnter && ev[0].action == 1);

    // release emits the up edge; the button map turns B/App into BACK
    live.keys.trigger = 0;
    live.keys.b = 1;
    n = inputTick(s, CTRL_LEFT, 202, live, 40, ev, 16);
    CHECK(n == 2);   // trigger up + b down
    live.keys.b = 0;

    // the frozen controller drops off after the window, and the pointer
    // falls back to the live one - or back to the hmd with none left
    for (int i = 0; i < 10; ++i)
        inputTick(s, CTRL_LEFT, 202, live, 50 + i * 100, ev, 16);
    CHECK(!s.leftConnected);
    CHECK(s.active == CTRL_RIGHT);
    inputTick(s, CTRL_RIGHT, 101, live, 2000, ev, 16);
    CHECK(!s.rightConnected && s.active == -1 && hmdInput(s));

    // the sharemem channel itself dying drops everything at once: no new
    // frames arrive to age out, so waiting on freshness would leave the
    // dead controller owning the pointer and freeze the gaze pick
    inputTick(s, CTRL_RIGHT, 300, live, 3000, ev, 16);
    inputTick(s, CTRL_RIGHT, 301, live, 3010, ev, 16);
    CHECK(s.rightConnected && s.active == CTRL_RIGHT && !hmdInput(s));
    ctrlDropAll(s);
    CHECK(!s.leftConnected && !s.rightConnected);
    CHECK(s.active == -1 && hmdInput(s));

    // --- aim ------------------------------------------------------------
    const float qi[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    const float eye[3] = {0.0f, 0.0f, 0.0f};
    float o[3], d[3];
    Mat4 model;

    // identity orientation points down -z; a plausible sensor position in
    // mm scales down to metres
    const float pos[3] = {150.0f, -300.0f, -250.0f};
    ctrlAim(qi, pos, CTRL_RIGHT, 0.0f, 0.0f, 0.0f, true, 0.001f, eye, o, d,
            &model);
    CHECK_F(d[2], -1.0f, 1e-5f);
    CHECK_F(d[0], 0.0f, 1e-5f);
    CHECK_F(o[0], 0.15f, 1e-5f);
    CHECK_F(o[1], -0.30f, 1e-5f);

    // yaw 90deg about +y swings the ray to -x
    const float half = 0.70710678f;
    const float qy[4] = {0.0f, half, 0.0f, half};
    ctrlAim(qy, pos, CTRL_RIGHT, 0.0f, 0.0f, 0.0f, true, 0.001f, eye, o, d,
            nullptr);
    CHECK_F(d[0], -1.0f, 1e-5f);
    CHECK_F(d[2], 0.0f, 1e-5f);

    // a dead/sentinel position falls back to a hand offset under the eye
    const float dead[3] = {0.0f, 0.0f, 0.0f};
    ctrlAim(qi, dead, CTRL_LEFT, 0.0f, 0.0f, 0.0f, true, 0.001f, eye, o, d,
            nullptr);
    CHECK(o[0] < 0.0f && o[1] < 0.0f);   // left hand, lowered

    // direction yaw/pitch helpers agree with the gaze math convention
    float dy;
    const float df[3] = {0.0f, 0.0f, -1.0f};
    CHECK(dirYaw(df, &dy) && fabsf(dy) < 1e-6f);
    const float dx[3] = {1.0f, 0.0f, 0.0f};
    CHECK(dirYaw(dx, &dy) && fabsf(dy - (float)M_PI / 2) < 1e-4f);
    CHECK_F(dirPitch(df), 0.0f, 1e-6f);

    // --- mesh ------------------------------------------------------------
    const char* obj =
        "v 0 0 0\n"
        "v 2 0 0\n"
        "v 0 1 0\n"
        "v 0 0 4\n"
        "f 1 2 3\n"
        "f 1/1/1 3/2/2 4/3/3\n";   // v/vt/vn form
    Mesh m;
    CHECK(meshFromObj(obj, strlen(obj), &m));
    CHECK(m.v.size() == 18);
    CHECK_F(m.v[3], 2.0f, 1e-6f);  // second vertex of tri 1 is v2.x
    meshFit(&m, 0.14f);
    float mx = 0.0f;
    for (size_t i = 0; i < m.v.size(); i += 3)
        if (fabsf(m.v[i]) > mx) mx = fabsf(m.v[i]);
    // longest axis (z, 4 units) fits 0.14 -> scale 0.035, so the 2-unit x
    // span centres at +-0.035
    CHECK_F(mx, 0.14f * 2.0f / 4.0f / 2.0f, 1e-5f);

    // junk in, empty out
    CHECK(!meshFromObj("not an obj", 11, &m) || m.v.empty());
}
