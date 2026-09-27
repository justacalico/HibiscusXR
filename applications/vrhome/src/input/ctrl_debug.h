#pragma once

#include "ctrl_state.h"
#include "input_state.h"

#include <stddef.h>

// second line of the HUD debug text: one segment per controller, e.g.
//   L →0.30 ↓0.25 ↗0.45 Y+12 P-5 ST3 B87*   R --
// pos/dir are each controller's world aim ray (ctrlPos/ctrlDir) - the same
// frame the head position prints in. The fuse pose is tracked on its own,
// so positions are real 6-dof even while the head line reads 3DOF. The
// pointer-owning controller gets a trailing '*'; a live controller with
// no pose block prints "nopose", a dead one "--".
void fmtCtrlLine(char* out, size_t outSize, const InputState& in,
                 const ctrl_state st[2], const float pos[2][3],
                 const float dir[2][3]);
