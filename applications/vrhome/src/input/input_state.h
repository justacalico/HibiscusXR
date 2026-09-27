#pragma once

#include "ctrl_state.h"

#include <cstdint>

// Input arbitration: which devices are live and who is pointing. Pure logic
// over ctrl_state frames - the host tests drive it directly.
//
// The caller decides liveness per block (ctrl_live over in ctrl_state.h -
// write activity on the wire, not content change). `active` is the main
// pointer - whichever controller last pressed a button wins, so picking
// up the other controller and clicking takes over, same as Oculus dash.

struct InputState {
    bool leftConnected = false;
    bool rightConnected = false;
    int active = -1;    // CTRL_LEFT / CTRL_RIGHT, -1 = no controller

    uint32_t buttons[CTRL_COUNT] = {0, 0};     // last packed button mask
};

// headset-native pointing: gaze ray + head button. Only while neither
// controller is connected - any live controller takes over the pointer
inline bool hmdInput(const InputState& s) {
    return !s.leftConnected && !s.rightConnected;
}

inline bool ctrlConnected(const InputState& s, int which) {
    return which == CTRL_LEFT ? s.leftConnected : s.rightConnected;
}

// the sharemem channel itself is gone (file deleted or never existed): no
// new frames arrive so the freshness window can never age anything out -
// drop every controller at once so the hmd pointer resumes immediately
inline void ctrlDropAll(InputState& s) {
    s.leftConnected = s.rightConnected = false;
    s.active = -1;
}

// button bits packed out of a key block; order matches ctrl_btn_index
enum CtrlBtn {
    BTN_TRIGGER = 0, BTN_A, BTN_B, BTN_APP, BTN_HOME, BTN_ROCKER,
    BTN_GRIPL, BTN_GRIPR, BTN_COUNT
};

uint32_t ctrlButtonMask(const ctrl_keys& k);

// one input event to feed onward: a KeyEvent-style code + action
// (0 = up, 1 = down)
struct InputEvent {
    int code;
    int action;
};

// pseudo codes ctrl.cpp routes itself - they never reach hudKey
constexpr int kCtrlSummon = 9001;
constexpr int kCtrlRecenter = 9002;

// per-frame update for one controller. `live` is the caller's link verdict
// for this frame; `st` the decoded frame (only trusted while live). Edges
// in the button mask emit events into `ev` (capacity cap); every edge also
// switches `active` to this controller. Returns event count.
int inputTick(InputState& s, int which, bool live,
              const ctrl_state& st, InputEvent* ev, int cap);
