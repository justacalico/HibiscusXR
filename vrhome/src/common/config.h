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
// elevation clamp: panels ride a cylinder around the viewer and tilt to keep
// facing it, so past ~86 deg the centre panel would sit on your crown
constexpr float kPitchMax = 1.5f;
// yaw offsets of the ring slots, relative to ring centre. 0.88 rad apart:
// a 1.3 m panel at 1.5 m spans ~0.82 rad, so neighbours can no longer overlap
constexpr float kSlotYaw[kMaxPanels] = {0.0f, -0.88f, 0.88f};
// minimum centre-to-centre yaw between panels: just under the slot spacing
// so a window can never land on top of one that drifted off the slot grid
constexpr float kPanelMinGap = 0.82f;

// window chrome: label pill under each panel holding the app name
constexpr float kBarH = 0.085f, kBarGap = 0.012f;
constexpr float kBarInset = 0.030f;   // horizontal margin vs the window edges
constexpr float kDragGain = 1.5f;     // drag point runs ahead of the gaze
constexpr float kPillPadX = 0.070f;   // pill side padding around the label
// minimize + close circles on the pill's right end; the library panel is the
// shell's own launcher and gets none
constexpr float kPillBtnR = 0.028f;   // button disc radius
constexpr float kPillBtnGap = 0.014f; // between the two discs
constexpr float kPillBtnPad = 0.014f; // close disc's margin to the pill edge
// total strip the buttons reserve on the pill's right end
constexpr float kPillBtnW = kPillBtnPad + 4.0f * kPillBtnR + kPillBtnGap;
constexpr float kCornerR = 0.028f;

// drag handle: a short white line centred under the pill. Holding confirm on
// it drags the whole ring - every window keeps its slot offset and follows
// the gaze yaw together
constexpr float kHandleW = 0.065f;    // visible line half-width
constexpr float kHandleT = 0.0055f;   // visible line half-thickness
constexpr float kHandleGap = 0.016f;  // gap between pill bottom and line top
constexpr float kHandlePad = 0.018f;  // extra hit slack around the line

// Pico's custom keycodes, installed via the patched libinput + gpio-keys.kl.
// 1003 is the headset home button remapped off HOME (system_server eats
// KEYCODE_HOME before anything else can see it, so the HUD's summon key
// must not be HOME at all)
constexpr int kPicoConfirm = 1001;
constexpr int kPicoHome = 1003;

// pseudo-package adopted by the app-library panel
constexpr const char* kLibraryPkg = "gitlab.neosalsa.hud.library";
