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

// pose filter (math/head.cpp poseFiltTick): the rotation vector jitters a
// fraction of a degree per sample even with the head bolted still, and fed
// raw into the view matrix that noise renders as the whole world shaking.
// The filter low-passes the pose with a convergence rate that ramps on how
// far the raw pose has pulled ahead: under the noise floor only the still
// rate applies so jitter can't drag the view, past a real turn the move
// rate takes over so tracking stays 1:1. Tunable off via
// debug.vrhome.posefilt.
constexpr float kFiltStillHz = 1.5f;    // converge rate while still, 1/s
constexpr float kFiltMoveHz = 45.0f;    // rate once the head is turning, 1/s
constexpr float kFiltStillRad = 0.01f;  // ~0.6 deg lead: sensor noise floor
constexpr float kFiltMoveRad = 0.10f;   // ~6 deg lead: a real turn, max rate
constexpr float kFiltStillM = 0.004f;   // same band for position, metres
constexpr float kFiltMoveM = 0.10f;

constexpr int kSensorIdent = 3;
constexpr int kInputIdent  = 4;

// home environments: zips live in /data/local/tmp - the one spot on /data
// that is both shell-writable (adb push) and app-readable, same place the
// OpenXR runtime is staged. persist.hibiscus.environment carries the
// selection: unset or "passthrough" is the live camera (how the shell
// ships), "builtin" is the procedural sky+grid, anything else names
// <id>.zip inside kEnvDir. debug.vrhome.env overrides for tuning.
constexpr const char* kEnvProp = "persist.hibiscus.environment";
constexpr const char* kEnvDebugProp = "debug.vrhome.env";
constexpr const char* kEnvDir = "/data/local/tmp/hibiscus/envs";
// floor height the SpawnUser marker maps to: the same plane the built-in
// grid sits on, so switching scenes never moves the ground under the user
constexpr float kEnvFloorY = -1.2f;
// the selection prop is polled on a slow tick, not every frame
constexpr long long kEnvPollMs = 400;

// panel defaults: roughly Quest-size panels
constexpr int   kVdW = 1600, kVdH = 900, kVdDpi = 240;
constexpr float kPanelDist = 1.5f;    // metres
constexpr float kPanelW = 1.30f, kPanelH = 0.73f;
constexpr float kPanelY = 0.05f;      // metres above horizon
constexpr int   kMaxPanels = 3;
// panel records total, parked (minimized) windows included: the ring holds
// kMaxPanels visible windows but a parked one keeps its task and display
// alive on the shelf, so the list can outgrow the slot count
constexpr int   kMaxPanelRecs = kMaxPanels + 3;
// elevation clamp: panels ride a cylinder around the viewer and tilt to keep
// facing it, so past ~86 deg the centre panel would sit on your crown
constexpr float kPitchMax = 1.5f;
// yaw offsets of the ring slots, relative to ring centre. 0.88 rad apart:
// a 1.3 m panel at 1.5 m spans ~0.82 rad, so neighbours can no longer overlap
constexpr float kSlotYaw[kMaxPanels] = {0.0f, -0.88f, 0.88f};
// minimum centre-to-centre yaw between panels: just under the slot spacing
// so a window can never land on top of one that drifted off the slot grid
constexpr float kPanelMinGap = 0.82f;
// window scale limits for the corner-grip resize
constexpr float kScaleMin = 0.45f, kScaleMax = 1.9f;
// resize grip: a small hot zone on the window's bottom-right corner
constexpr float kResizeR = 0.055f;   // corner hit radius, world units

// floating keyboard: the IME app draws into a texture the HUD owns, shown
// as its own window under the one holding the text field. It rides nearer
// than the panels AND the dock (kDockDist 1.35) so nothing can occlude it -
// like Quest, the keyboard is the closest thing on screen. The px size
// rides the panels' pixels-per-metre (kPanelW/kVdW) so keys match window
// text size
constexpr int   kKbdW = 1400, kKbdH = 490, kKbdDpi = 240;
constexpr float kKbdHW = 0.56f, kKbdHH = 0.196f;  // world half extents
constexpr float kKbdDist = 1.15f;   // metres off the eye, in front of dock
constexpr float kKbdGap = 0.03f;    // view-space dip below the host window

// window chrome: a Quest-style pill floating under each panel's bottom
// edge, holding the app name on the left and the buttons on the right.
// The window itself keeps all four corners rounded - nothing bound to it
constexpr float kPillH = 0.085f;      // pill height
constexpr float kPillGap = 0.026f;    // window bottom edge to pill top
constexpr float kPillWFrac = 0.62f;   // pill half width vs window half width
// narrowest the pill gets: the button strip plus a sliver of label must
// always fit, even on a heavily shrunken window
constexpr float kPillMinHW = 0.36f;
constexpr float kDragGain = 1.5f;     // drag point runs ahead of the gaze
constexpr float kPillPadX = 0.070f;   // label padding inside the pill's left end
// float + minimize + close circles on the pill's right end
constexpr float kPillBtnR = 0.028f;   // button disc radius
constexpr float kPillBtnGap = 0.014f; // between the discs
constexpr float kPillBtnPad = 0.014f; // close disc's margin to the pill edge
constexpr float kCornerR = 0.028f;

// drag handle: a short white line centred under the dock strip. Holding
// confirm on it drags the whole dash - every window keeps its slot offset
// and the strip stays glued under them
constexpr float kHandleW = 0.085f;    // visible line half-width
constexpr float kHandleT = 0.0085f;   // visible line half-thickness
constexpr float kHandleGap = 0.026f;  // gap between bar bottom and line top
constexpr float kHandlePad = 0.014f;  // extra hit slack around the line

// the dock: a persistent strip hanging under the panel ring - the status
// cluster on the left (its pill opens the quick panel), pinned apps and
// running tasks after it, the app-grid button on the end. It rides the
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
// the windows ride a fixed lift above the strip: the dash is one assembly,
// so a window's elevation is the strip's pitch plus this gap - the strip
// sits just under the window row like the sketch ties them together
constexpr float kRingLift = 0.55f;

// the app-grid overlay: a big rounded card in front of the window slots,
// opened from the dock's app button - the library lives inside the dash
// now instead of being a window of its own
constexpr float kGridDist = 1.38f;    // metres; panels at 1.5, dock at 1.35
constexpr float kGridHW = 0.78f, kGridHH = 0.46f;   // card half extents
constexpr float kGridHeadH = 0.10f;   // title band height
constexpr int   kGridCols = 5;
constexpr float kGridCellH = 0.20f;   // row pitch: icon + label
constexpr float kGridIconHW = 0.055f; // icon half-width
constexpr float kGridSidePad = 0.04f; // inner left/right padding
constexpr float kGridCloseR = 0.024f; // close disc radius on the title row

// minimized-window shelf: hidden panels park as a row of small icons on a
// pill floating just above the dock bar, so a minimized app stays visible
// instead of vanishing. It rides the dock's anchor plane, lifted along its
// up; the notification stack clears it via shelfTop()
constexpr float kShelfIconHW = 0.042f;  // icon half-width
constexpr float kShelfGap = 0.020f;     // between icons
constexpr float kShelfPad = 0.014f;     // pill inset around the icon row
constexpr float kShelfHH = kShelfIconHW + kShelfPad;  // pill half-height
constexpr float kShelfGapY = 0.016f;    // between bar top and pill bottom

// HUD motion: every transition runs off these. The easing/interpolation
// itself lives in the pure anim/ module so make test covers it; the draw
// and pick paths only apply the results
constexpr float kSpawnMs = 240.0f;      // panel scale/fade-in
constexpr float kSpawnScale0 = 0.82f;   // launch size as a share of final
constexpr float kMinMs = 260.0f;        // minimize/restore flight, one way
constexpr float kDashMs = 220.0f;       // strip slide/fade-in on summon
constexpr float kDashDrop = 0.07f;      // metres the strip rises in from
constexpr float kHoverScale = 1.14f;    // dock/shelf icon magnification
constexpr float kHoverTauMs = 65.0f;    // hover-scale smoothing constant
constexpr float kGridMs = 200.0f;       // app-grid card open/close
constexpr float kGridScale0 = 0.94f;    // card's open-from size share
constexpr float kGridHoverScale = 1.12f; // cell icon magnification

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

// system-message cards: crash/ANR dropbox entries surfaced as a modal VR
// dialog - the image's hide_error_dialogs keeps the mono dialog off the
// panel, this card replaces it. In the dash the card rides the dock's
// anchor lifted to eye level; over a covered app it anchors on the gaze
// yaw when it popped, straight ahead at card pitch
constexpr float kSysMsgW = 0.62f;      // card width
constexpr float kSysMsgH = 0.30f;      // card height
constexpr float kSysMsgPad = 0.026f;   // inner side padding
constexpr float kSysMsgIconHW = 0.034f;// app icon half-width
constexpr float kSysMsgBtnHH = 0.024f; // button pill half-height
constexpr float kSysMsgBtnGap = 0.012f;// between the pills
constexpr float kSysMsgBtnSlack = 0.010f; // extra hit room around a pill
constexpr float kSysMsgLift = 0.85f;   // above the dock bar in dash mode
constexpr float kSysMsgPitch = 0.05f;  // over an app: just above eye level
constexpr int   kSysMsgMaxLines = 2;   // body lines drawn per card
constexpr int   kSysMsgMaxBtn = 4;

// package the dock's quick-panel button launches
constexpr const char* kQuickPanelPkg = "gitlab.neosalsa.quicksettings";
// the full settings app - dock's default pin
constexpr const char* kSettingsPkg = "gitlab.neosalsa.settings";

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
