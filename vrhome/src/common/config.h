#pragma once

// Shared constants. All values are compile-time; the runtime overrides for
// head tracking live behind the debug.vrhome.* system properties.

// tunables confirmed on the headset in the vrdemo: worldx 90
constexpr float kDistK1 = 0.22f, kDistK2 = 0.24f;
constexpr float kIPD  = 0.063f;
constexpr float kFovY = 90.0f;
// roll back to the vrdemo-confirmed 90: 270 only looked upright because
// quatToMat was returning the raw rotation instead of its transpose.
// Still live-tunable via debug.vrhome.roll.
constexpr float kRoll = 90.0f, kSensRoll = 0.0f, kWorldX = 90.0f;

constexpr int kSensorIdent = 3;
constexpr int kInputIdent  = 4;

// panel defaults: roughly Quest-size panels
constexpr int   kVdW = 1600, kVdH = 900, kVdDpi = 240;
constexpr float kPanelDist = 2.2f;    // metres
constexpr float kPanelW = 1.30f, kPanelH = 0.73f;
constexpr float kPanelY = 0.05f;      // metres above horizon
constexpr int   kMaxPanels = 3;
// yaw offsets of the ring slots, relative to ring centre. 0.62 rad apart:
// a 1.3 m panel at 2.2 m spans ~0.57 rad, so neighbours can no longer overlap
constexpr float kSlotYaw[kMaxPanels] = {0.0f, -0.62f, 0.62f};

// window chrome: bottom bar under each panel holding the app label
constexpr float kBarH = 0.085f, kBarGap = 0.012f;
constexpr float kBarInset = 0.030f;   // horizontal margin vs the window edges
constexpr float kCornerR = 0.028f;

// Pico's custom keycode, installed via the patched libinput + gpio-keys.kl
constexpr int kPicoConfirm = 1001;

// pseudo-package adopted by the app-library panel
constexpr const char* kLibraryPkg = "org.pn2.vrhome.library";
