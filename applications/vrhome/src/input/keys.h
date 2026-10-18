#pragma once

// Key codes we treat as the headset's confirm button. Values mirror the
// Android AKEYCODE_* constants but are defined here so this header stays
// usable on the host.
constexpr int kKeyEnter      = 66;   // AKEYCODE_ENTER
constexpr int kKeyDpadCenter = 23;   // AKEYCODE_DPAD_CENTER
constexpr int kKeyButtonA    = 96;   // AKEYCODE_BUTTON_A
constexpr int kKeyBack       = 4;    // AKEYCODE_BACK
// kPicoConfirm (1001) lives in common/config.h

// semantic-only codes hudKey routes like the AKEYCODE_* ones above: no
// hardware ever emits these
constexpr int kKeyMinimize   = 9003; // park the top window on the shelf

// what the desktop ` emits: the input grab toggles by default, shift+`
// always releases it
constexpr int kGrabToggle    = 1;
constexpr int kGrabOff       = 2;

bool isConfirm(int code);

// what the desktop ESC emits: a parked window by default, a full close
// (back) while shift is held
int escAction(bool shiftHeld);

// what the desktop ` emits on press: toggles the focused app's input
// grab by default, always releases while shift is held
int grabAction(bool shiftHeld);
