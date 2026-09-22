#include "input_state.h"

#include "keys.h"
#include "../common/config.h"

uint32_t ctrlButtonMask(const ctrl_keys& k) {
    uint32_t m = 0;
    if (k.trigger) m |= 1u << BTN_TRIGGER;
    if (k.a)       m |= 1u << BTN_A;
    if (k.b)       m |= 1u << BTN_B;
    if (k.app)     m |= 1u << BTN_APP;
    if (k.home)    m |= 1u << BTN_HOME;
    if (k.rocker)  m |= 1u << BTN_ROCKER;
    if (k.grip_l)  m |= 1u << BTN_GRIPL;
    if (k.grip_r)  m |= 1u << BTN_GRIPR;
    return m;
}

// what each button emits; summon/recenter route in ctrl.cpp, the rest are
// real keycodes hudKey already understands
static int codeFor(int btn) {
    switch (btn) {
    case BTN_TRIGGER:
    case BTN_A:
    case BTN_GRIPL:
    case BTN_GRIPR:  return kKeyEnter;
    case BTN_B:
    case BTN_APP:    return kKeyBack;
    case BTN_HOME:   return kCtrlSummon;
    case BTN_ROCKER: return kCtrlRecenter;
    default:         return 0;
    }
}

static void emit(InputEvent* ev, int* n, int cap, int code, int action) {
    if (code && *n < cap) {
        ev[*n].code = code;
        ev[*n].action = action;
        ++*n;
    }
}

int inputTick(InputState& s, int which, uint64_t hash,
              const ctrl_state& st, long long nowMs,
              InputEvent* ev, int cap) {
    int n = 0;

    // freshness: a streaming controller's block never holds still for a
    // whole window; once it does the link is gone. The first read is just
    // a baseline - the file can sit stale for hours, so a block only counts
    // live once it changed after we started watching it
    if (hash != s.hash[which]) {
        s.hash[which] = hash;
        s.changeMs[which] = s.seen[which] ? nowMs : nowMs - kCtrlLiveMs;
        s.seen[which] = true;
    }
    const bool live = s.seen[which] && nowMs - s.changeMs[which] < kCtrlLiveMs;
    bool& conn = which == CTRL_LEFT ? s.leftConnected : s.rightConnected;
    if (live && !conn) {
        conn = true;
        // first controller up becomes the pointer immediately; a second
        // one waits for a button press so it doesn't steal the aim
        if (s.active < 0) s.active = which;
    } else if (!live && conn) {
        conn = false;
        if (s.active == which)
            s.active = ctrlConnected(s, 1 - which) ? 1 - which : -1;
    }

    // button edges: every press promotes this controller to the pointer,
    // even while the other one is live - last to click owns the dash
    const uint32_t mask = conn ? ctrlButtonMask(st.keys) : 0;
    const uint32_t prev = s.buttons[which];
    s.buttons[which] = mask;
    if (!conn) return n;
    for (int b = 0; b < BTN_COUNT; ++b) {
        const uint32_t bit = 1u << b;
        const bool was = prev & bit, is = mask & bit;
        if (is && !was) {
            s.active = which;
            emit(ev, &n, cap, codeFor(b), 1);
        } else if (!is && was) {
            emit(ev, &n, cap, codeFor(b), 0);
        }
    }
    return n;
}
