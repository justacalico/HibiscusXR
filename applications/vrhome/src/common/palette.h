#pragma once

// The OS palette: one surface ramp and one accent shared by the HUD
// chrome and the Flutter panel apps. Every value mirrors a constant in
// applications/{library,settings,quick-panel}/lib/src/ui/theme.dart -
// the hex on each line is that Dart Color(0xAARRGGBB), the floats are
// its sRGB channels / 255 so a chrome pixel blends to the same code
// value a panel pixel shows.
//
//   background  void behind the dash, the apps' scaffold
//   panel       chrome furniture: dock strip, shelf, window title bar
//   surface     cards and pills raised off that furniture
//   surfaceHigh controls sitting on a card; also the divider colour
//   accent      the one interactive hue: beam, progress rings, actions
//   danger      destructive hovers and alert marks
//   warn, good  status hues shared with the quick-panel battery bands
//   textDim     secondary labels; primary text is kPalText
constexpr float kPalBackground[3] = {0x14 / 255.0f, 0x1A / 255.0f, 0x21 / 255.0f};
constexpr float kPalPanel[3]      = {0x1B / 255.0f, 0x23 / 255.0f, 0x2D / 255.0f};
constexpr float kPalSurface[3]    = {0x23 / 255.0f, 0x2D / 255.0f, 0x38 / 255.0f};
constexpr float kPalSurfaceHigh[3]= {0x2E / 255.0f, 0x3A / 255.0f, 0x47 / 255.0f};
constexpr float kPalAccent[3]     = {0x4E / 255.0f, 0x9C / 255.0f, 0xFF / 255.0f};
constexpr float kPalDanger[3]     = {0xFF / 255.0f, 0x5E / 255.0f, 0x5E / 255.0f};
constexpr float kPalWarn[3]       = {0xF5 / 255.0f, 0xC5 / 255.0f, 0x42 / 255.0f};
constexpr float kPalGood[3]       = {0x3D / 255.0f, 0xD6 / 255.0f, 0x8C / 255.0f};
constexpr float kPalText[3]       = {0xF2 / 255.0f, 0xF5 / 255.0f, 0xF8 / 255.0f};
constexpr float kPalTextDim[3]    = {0x9A / 255.0f, 0xA7 / 255.0f, 0xB4 / 255.0f};

// battery fill hue: the quick-panel's bands - >=80 good, >=60 text,
// >=20 warn, below that danger - with charging pinned to warn, the
// amber the dock always showed while plugged in
inline const float* batteryTint(int level, bool charging) {
    if (charging) return kPalWarn;
    if (level >= 80) return kPalGood;
    if (level >= 60) return kPalText;
    if (level >= 20) return kPalWarn;
    return kPalDanger;
}
