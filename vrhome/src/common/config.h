#pragma once

// Shared constants. All values are compile-time; the runtime overrides for
// head tracking live behind the debug.vrhome.* system properties.

// Neo 2 lens polynomial from the stock /vendor/etc/qvr/svrapi_config.txt:
// scale(r) = K0 + K2 r^2 + K4 r^4 + K6 r^6, r the tan-angle radius off the
// lens axis (r = 1 at 45 deg). The old hand-tuned 1 + 0.22 r2 + 0.24 r2^2
// stayed near 1 across the field, so the lens's own distortion showed
// through and the image stretched toward the screen edges.
constexpr float kLensK0 = 0.740740741f, kLensK2 = 0.192360375f;
constexpr float kLensK4 = -0.020400088f, kLensK6 = 0.216338258f;
// chromatic-aberration channel scales, multiplied into the warp
constexpr float kLensChr = 0.992f, kLensChb = 1.012f;
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
constexpr float kPanelDist = 1.5f;    // metres
constexpr float kPanelW = 1.30f, kPanelH = 0.73f;
constexpr float kPanelY = 0.05f;      // metres above horizon
constexpr int   kMaxPanels = 3;
// yaw offsets of the ring slots, relative to ring centre. 0.88 rad apart:
// a 1.3 m panel at 1.5 m spans ~0.82 rad, so neighbours can no longer overlap
constexpr float kSlotYaw[kMaxPanels] = {0.0f, -0.88f, 0.88f};

// window chrome: label pill under each panel holding the app name
constexpr float kBarH = 0.085f, kBarGap = 0.012f;
constexpr float kBarInset = 0.030f;   // horizontal margin vs the window edges
constexpr float kDragGain = 1.5f;     // drag point runs ahead of the gaze
constexpr float kPillPadX = 0.070f;   // pill side padding around the label
constexpr float kCornerR = 0.028f;

// Pico's custom keycode, installed via the patched libinput + gpio-keys.kl
constexpr int kPicoConfirm = 1001;

// pseudo-package adopted by the app-library panel
constexpr const char* kLibraryPkg = "gitlab.neosalsa.home.library";
