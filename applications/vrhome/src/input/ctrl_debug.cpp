#include "ctrl_debug.h"

#include "aim.h"
#include "../math/head.h"

#include <math.h>
#include <stdio.h>

static int fmtOne(char* out, size_t n, int which, const InputState& in,
                  const ctrl_state& st, const float pos[3],
                  const float dir[3]) {
    const char hand = which == CTRL_LEFT ? 'L' : 'R';
    if (!ctrlConnected(in, which))
        return snprintf(out, n, "%c --", hand);

    // battery only means something once a key block has landed
    char bat[8] = "";
    if (st.keys_ok) snprintf(bat, sizeof(bat), " B%d", st.keys.battery);

    if (!st.pose_ok)
        return snprintf(out, n, "%c nopose%s", hand, bat);

    char posStr[48];
    fmtPosArrows(pos, posStr, sizeof(posStr));
    float yaw;
    char yawStr[16];
    if (dirYaw(dir, &yaw))
        snprintf(yawStr, sizeof(yawStr), " Y%+4.0f",
                 yaw * 180.0f / (float)M_PI);
    else
        snprintf(yawStr, sizeof(yawStr), " Y---");
    return snprintf(out, n, "%c%s%s P%+4.0f ST%d%s%s", hand, posStr, yawStr,
                    dirPitch(dir) * 180.0f / (float)M_PI, st.fuse.status, bat,
                    in.active == which ? "*" : "");
}

void fmtCtrlLine(char* out, size_t outSize, const InputState& in,
                 const ctrl_state st[2], const float pos[2][3],
                 const float dir[2][3]) {
    char left[64], right[64];
    fmtOne(left, sizeof(left), CTRL_LEFT, in, st[CTRL_LEFT],
           pos[CTRL_LEFT], dir[CTRL_LEFT]);
    fmtOne(right, sizeof(right), CTRL_RIGHT, in, st[CTRL_RIGHT],
           pos[CTRL_RIGHT], dir[CTRL_RIGHT]);
    snprintf(out, outSize, "%s   %s", left, right);
}
