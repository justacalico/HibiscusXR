#pragma once

#include "ctrl_state.h"

#include <cstdint>

// Input arbitration: which devices are live and who is pointing. Pure logic
// over ctrl_state frames - the host tests drive it directly.
//
// Connection is freshness-based: the service rewrites a controller's block
// per packet, so a block whose hash stops moving is a controller that went
// away. `active` is the main pointer - whichever controller last pressed a
// button wins, so picking up the other controller and clicking takes over,
// same as Oculus dash.

// a block counts as live while it kept changing inside this window
constexpr long long kCtrlLiveMs = 800;

struct InputState {
    bool leftConnected = false;
    bool rightConnected = false;
    int active = -1;    // CTRL_LEFT / CTRL_RIGHT, -1 = no controller

    uint64_t hash[CTRL_COUNT] = {0, 0};        // last block hash
    long long changeMs[CTRL_COUNT] = {0, 0};   // last time the hash moved
    uint32_t buttons[CTRL_COUNT] = {0, 0};     // last packed button mask
    bool seen[CTRL_COUNT] = {false, false};    // ever seen a live block
};

// headset-native pointing: gaze ray + head button. Only while neither
// controller is connected - any live controller takes over the pointer
inline bool hmdInput(const InputState& s) {
    return !s.leftConnected && !s.rightConnected;
}

inline bool ctrlConnected(const InputState& s, int which) {
    return which == CTRL_LEFT ? s.leftConnected : s.rightConnected;
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

// per-frame update for one controller. `hash` is the block's freshness
// stamp; `st` the decoded frame (only trusted when the block was live).
// Edges in the button mask emit events into `ev` (capacity cap); every edge
// also switches `active` to this controller. Returns event count.
int inputTick(InputState& s, int which, uint64_t hash,
              const ctrl_state& st, long long nowMs,
              InputEvent* ev, int cap);
