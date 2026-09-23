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
// QVR's device frame is mounted -90 deg about the head's forward axis vs
// the frame the view expects: with no correction the world reads as
// permanently rolled. A sensor-mount roll, not a world tilt - VIO's world
// is already gravity-aligned, so worldX stays 0
constexpr float kQvrSensRoll = -90.0f, kQvrWorldX = 0.0f;

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

// window chrome: a top bar bound to each panel's top edge, holding the app
// name. It sits flush on the surface - square bottom corners on a square
// top edge - so the pair reads as one rounded shape
constexpr float kBarH = 0.085f;
constexpr float kDragGain = 1.5f;     // drag point runs ahead of the gaze
constexpr float kBarPadX = 0.070f;    // label padding inside the bar's left end
// minimize + close circles on the bar's right end; the library panel is the
// shell's own launcher and gets the close disc only - it leaves the ring
// instead of parking on the shelf
constexpr float kBarBtnR = 0.028f;    // button disc radius
constexpr float kBarBtnGap = 0.014f;  // between the two discs
constexpr float kBarBtnPad = 0.014f;  // close disc's margin to the bar edge
// total strip the buttons reserve on the bar's right end
constexpr float kBarBtnW = kBarBtnPad + 4.0f * kBarBtnR + kBarBtnGap;
constexpr float kCornerR = 0.028f;

// drag handle: a short white line centred under the dock strip. Holding
// confirm on it drags the whole ring - every window keeps its slot offset
// and follows the gaze yaw together
constexpr float kHandleW = 0.085f;    // visible line half-width
constexpr float kHandleT = 0.0085f;   // visible line half-thickness
constexpr float kHandleGap = 0.026f;  // gap between bar bottom and line top
constexpr float kHandlePad = 0.014f;  // extra hit slack around the line

// the dock: a persistent strip hanging under the panel ring - pinned apps
// left, running tasks right, quick-panel button on the end. It rides the
// same anchor cylinder as the windows, slightly closer so it reads as the
// dash's foreground edge
constexpr float kDockDist = 1.35f;    // metres; panels sit at 1.5
constexpr float kDockIconW = 0.125f;  // icon square edge
constexpr float kDockIconHW = kDockIconW * 0.5f;
constexpr float kDockIconY = 0.006f;  // icon centre above bar centre
// icon corner radius as a fraction of the icon's half-width: 0.44 is about
// 22% of the edge - the macOS squircle, squarer than a circle crop
constexpr float kIconRad = 0.44f;
constexpr float kDockGap = 0.030f;    // between icons
constexpr float kDockPad = 0.045f;    // bar end padding
constexpr float kDockSepW = 0.035f;   // extra gap at a group separator
constexpr float kDockBarH = 0.16f;    // strip height: icon + running dot
// status cluster pinned to the strip's left end: clock, battery and wifi
// grouped in one pill, the notification bell in a second, then the same
// separator the app groups use
constexpr float kSysIconW = 0.10f;    // status slot width
constexpr float kSysGap = 0.020f;     // between slots inside a pill
constexpr float kSysPx = 0.0013f;     // status text metres per font px
constexpr float kSysPillPad = 0.014f; // pill inset around its slots
constexpr float kSysPillGap = 0.022f; // between the two pills
constexpr float kSysPillHH = 0.056f;  // pill half-height
constexpr float kDockBadgeR = 0.024f; // XR close badge radius
constexpr int   kDockPinMs = 600;     // confirm hold that toggles a pin
// dock elevation: scaled with the recenter pitch but clamped so the strip
// always lands below eye level - never over the windows, never overhead
constexpr float kDockPitchScale = 0.35f, kDockPitchDrop = 0.55f;
constexpr float kDockPitchMin = -0.95f, kDockPitchMax = -0.20f;
constexpr float kDockPitchRest = -0.55f;   // before the first anchor

// minimized-window shelf: hidden panels park as a row of small icons on a
// pill floating just above the dock bar, so a minimized app stays visible
// instead of vanishing. It rides the dock's anchor plane, lifted along its
// up; the notification stack clears it via shelfTop()
constexpr float kShelfIconHW = 0.042f;  // icon half-width
constexpr float kShelfGap = 0.020f;     // between icons
constexpr float kShelfPad = 0.014f;     // pill inset around the icon row
constexpr float kShelfHH = kShelfIconHW + kShelfPad;  // pill half-height
constexpr float kShelfGapY = 0.016f;    // between bar top and pill bottom

// notification cards: a stack floating above the dock bar (and the
// minimized shelf when one is up) while the dash is up; over a covered app
// the toast window draws it alone for kNotifToastMs. The stack rides the
// same anchor cylinder as the strip
constexpr float kNotifCardW = 0.56f;    // card width
constexpr float kNotifCardH = 0.115f;   // card height
constexpr float kNotifGap = 0.014f;     // between stacked cards, and the
                                        // lift between the bar and stack
constexpr float kNotifPad = 0.022f;     // card inner side padding
constexpr float kNotifIconHW = 0.030f;  // app icon half-width on a card
constexpr float kNotifBadgeR = 0.020f;  // dismiss badge radius
constexpr int   kNotifMax = 1;          // cards shown at once
constexpr int   kNotifToastMs = 5000;   // heads-up duration over an app
constexpr long long kNotifShowMs = 5000; // dash card lifetime from postMs;
                                        // the shade record outlives it
// toast stack elevation: slightly above eye level, anchored on the gaze
// yaw at the moment the toast pops so it never hides behind the user
constexpr float kNotifToastPitch = 0.14f;

// package the dock's quick-panel button launches
constexpr const char* kQuickPanelPkg = "gitlab.neosalsa.quicksettings";

// Pico's custom keycodes, installed via the patched libinput + gpio-keys.kl.
// 1003 is the headset home button remapped off HOME (system_server eats
// KEYCODE_HOME before anything else can see it, so the HUD's summon key
// must not be HOME at all)
constexpr int kPicoConfirm = 1001;
constexpr int kPicoHome = 1003;

// summon-key hold: how long before the recenter fires, and the progress
// ring's size as a screen-space overlay
constexpr int   kHoldMs = 600;
constexpr float kHoldSize = 0.10f;  // quad half height in clip space

// package adopted by the app-library panel: the standalone Flutter app,
// hosted on its own virtual display like every other panel window
constexpr const char* kLibraryPkg = "gitlab.neosalsa.library";
