#pragma once

// The overlay draws a full stereo + warp pass every frame. When a covered
// app owns the screen and nothing on the overlay is moving, most of those
// draws only redraw a static strip and cursor - a fixed GPU tax the app
// underneath pays for nothing. This is the pure rule for when the render
// thread may coast at a low cadence; the caller feeds it the current
// state each tick and the ms since the last busy signal.

struct HudQuietIn {
    bool panels = false;        // windows on the ring
    bool grid = false;          // app grid open or animating
    bool notifs = false;        // live notification cards
    bool sysmsgs = false;       // crash/ANR cards
    bool toastOnly = false;     // heads-up over the covered app
    bool sysMsgOnly = false;    // modal over the covered app
    bool debugOnly = false;     // debug status window
    bool kbd = false;           // floating keyboard up or animating
    bool holdRing = false;      // summon-key fill animating
    bool held = false;          // a confirm/drag is held down
    bool presses = false;       // any press gesture in flight
    bool inputPending = false;  // unconsumed controller or key events
    bool aimMoved = false;      // aim ray moved past the deadzone
    bool recentSummon = false;  // window came up within the grace period
};

// busy means something is on screen that can still change next frame.
inline bool hudQuietBusy(const HudQuietIn& q) {
    return q.panels || q.grid || q.notifs || q.sysmsgs || q.toastOnly ||
           q.sysMsgOnly || q.debugOnly || q.kbd || q.holdRing || q.held ||
           q.presses || q.inputPending || q.aimMoved || q.recentSummon;
}

// quiet once nothing has been busy for the grace period: the cooldown
// keeps fade-outs and settles from being cut off mid-animation.
inline bool hudQuiet(const HudQuietIn& q, long long msSinceBusy,
                     long long graceMs) {
    if (hudQuietBusy(q)) return false;
    return msSinceBusy >= graceMs;
}
