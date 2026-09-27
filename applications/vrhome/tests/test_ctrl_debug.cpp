#include "test.h"

#include "input/ctrl_debug.h"

#include <cstring>

void testCtrlDebug() {
    InputState in;
    ctrl_state st[2] = {};
    float pos[2][3] = {{0}};
    float dir[2][3] = {{0, 0, -1}, {0, 0, -1}};
    char buf[128];

    // both dark
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strcmp(buf, "L --   R --") == 0);

    // left live, posed and owning the pointer
    in.leftConnected = true;
    in.active = CTRL_LEFT;
    st[0].pose_ok = st[0].keys_ok = 1;
    st[0].fuse.status = 3;
    st[0].keys.battery = 87;
    pos[0][0] = 0.30f; pos[0][1] = -0.25f; pos[0][2] = -0.45f;
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strcmp(buf,
        "L →0.30 ↓0.25 ↗0.45 Y  +0 P  +0 ST3 B87*   R --") == 0);

    // pose block missing: still shows the hand is up, battery survives
    st[0].pose_ok = 0;
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strcmp(buf, "L nopose B87   R --") == 0);
    st[0].pose_ok = 1;

    // no key block yet: no battery field at all
    st[0].keys_ok = 0;
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strcmp(buf,
        "L →0.30 ↓0.25 ↗0.45 Y  +0 P  +0 ST3*   R --") == 0);
    st[0].keys_ok = 1;

    // pointer moved to the right controller
    in.rightConnected = true;
    in.active = CTRL_RIGHT;
    st[1].pose_ok = st[1].keys_ok = 1;
    st[1].fuse.status = 1;
    st[1].keys.battery = 40;
    pos[1][0] = -0.10f; pos[1][1] = -0.20f; pos[1][2] = -0.30f;
    dir[1][0] = 1.0f; dir[1][2] = -1.0f;   // ~45 deg yaw
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strcmp(buf,
        "L →0.30 ↓0.25 ↗0.45 Y  +0 P  +0 ST3 B87"
        "   R ←0.10 ↓0.20 ↗0.30 Y +45 P  +0 ST1 B40*") == 0);

    // aim straight down: yaw is undefined, pitch still prints
    dir[1][0] = 0.0f; dir[1][1] = -1.0f; dir[1][2] = 0.0f;
    fmtCtrlLine(buf, sizeof(buf), in, st, pos, dir);
    CHECK(strstr(buf, "R ←0.10 ↓0.20 ↗0.30 Y--- P -90 ST1 B40*") != nullptr);
}
