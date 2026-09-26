#pragma once

// The OS palette: one surface ramp and one accent shared by the HUD
// chrome and the Flutter panel apps, in three tables - dark, light and
// the true-black OLED variant. The settings app writes the pick into
// the hibiscus_theme Settings.Global key, pn2-themed mirrors it onto
// persist.hibiscus.theme, and syncPalette() repoints the kPal* globals
// once a frame so a theme change lands on the next vsync.
//
// Every value mirrors a constant in
// applications/{library,settings,quick-panel}/lib/src/ui/theme.dart -
// the hex behind each table is that Dart Color(0xAARRGGBB), the floats
// are its sRGB channels / 255 so a chrome pixel blends to the same code
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
struct Palette {
    float background[3];
    float panel[3];
    float surface[3];
    float surfaceHigh[3];
    float accent[3];
    float danger[3];
    float warn[3];
    float good[3];
    float text[3];
    float textDim[3];
};

// the three theme tables, defined in palette.cpp
extern const Palette kPalDark;
extern const Palette kPalLight;
extern const Palette kPalOled;

// live slots into the active table, repointed by setPalette()
extern const float* kPalBackground;
extern const float* kPalPanel;
extern const float* kPalSurface;
extern const float* kPalSurfaceHigh;
extern const float* kPalAccent;
extern const float* kPalDanger;
extern const float* kPalWarn;
extern const float* kPalGood;
extern const float* kPalText;
extern const float* kPalTextDim;

// name -> table: "light" and "oled" match, everything else is dark
const Palette& paletteForName(const char* name);

// repoint the kPal* slots at a table
void setPalette(const Palette& p);
void setPaletteForName(const char* name);

// per-frame glue: reads persist.hibiscus.theme and applies it. No-op off
// Android, so host tests steer the slots with setPaletteForName instead.
void syncPalette();

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
