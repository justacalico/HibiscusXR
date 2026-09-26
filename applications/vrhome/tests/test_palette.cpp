#include "test.h"

#include "common/palette.h"

// pins the mirror: each constant must equal the hex the Flutter themes
// declare in applications/{library,settings,quick-panel}/lib/src/ui/
// theme.dart - if one side drifts, a channel check here goes red
static void hexEq(const float* c, int r, int g, int b) {
    CHECK_F(c[0] * 255.0f, (float)r, 0.01f);
    CHECK_F(c[1] * 255.0f, (float)g, 0.01f);
    CHECK_F(c[2] * 255.0f, (float)b, 0.01f);
}

void testPalette() {
    hexEq(kPalBackground,  0x14, 0x1A, 0x21);
    hexEq(kPalPanel,       0x1B, 0x23, 0x2D);
    hexEq(kPalSurface,     0x23, 0x2D, 0x38);
    hexEq(kPalSurfaceHigh, 0x2E, 0x3A, 0x47);
    hexEq(kPalAccent,      0x4E, 0x9C, 0xFF);
    hexEq(kPalDanger,      0xFF, 0x5E, 0x5E);
    hexEq(kPalWarn,        0xF5, 0xC5, 0x42);
    hexEq(kPalGood,        0x3D, 0xD6, 0x8C);
    hexEq(kPalText,        0xF2, 0xF5, 0xF8);
    hexEq(kPalTextDim,     0x9A, 0xA7, 0xB4);

    // the ramp only climbs: every role sits darker than the one above it
    // so chrome never inverts against the cards floating over it
    for (int ch = 0; ch < 3; ++ch) {
        CHECK(kPalPanel[ch] > kPalBackground[ch]);
        CHECK(kPalSurface[ch] > kPalPanel[ch]);
        CHECK(kPalSurfaceHigh[ch] > kPalSurface[ch]);
    }

    // batteryTint mirrors batteryTintFor in quick-panel's battery.dart:
    // >=80 good, >=60 text, >=20 warn, under that danger
    CHECK(batteryTint(100, false) == kPalGood);
    CHECK(batteryTint(80, false) == kPalGood);
    CHECK(batteryTint(79, false) == kPalText);
    CHECK(batteryTint(60, false) == kPalText);
    CHECK(batteryTint(59, false) == kPalWarn);
    CHECK(batteryTint(20, false) == kPalWarn);
    CHECK(batteryTint(19, false) == kPalDanger);
    CHECK(batteryTint(0, false) == kPalDanger);
    // charging pins the warn hue at any level
    CHECK(batteryTint(90, true) == kPalWarn);
    CHECK(batteryTint(5, true) == kPalWarn);
}

void testPaletteThemes() {
    // name -> table: the prop values pn2-themed writes, dark for
    // anything unrecognized
    CHECK(&paletteForName("dark") == &kPalDark);
    CHECK(&paletteForName("light") == &kPalLight);
    CHECK(&paletteForName("oled") == &kPalOled);
    CHECK(&paletteForName("") == &kPalDark);
    CHECK(&paletteForName("sepia") == &kPalDark);
    CHECK(&paletteForName(nullptr) == &kPalDark);

    // setPaletteForName repoints every live slot
    setPaletteForName("oled");
    hexEq(kPalBackground, 0x00, 0x00, 0x00);
    CHECK(kPalPanel == kPalOled.panel);
    CHECK(kPalAccent == kPalOled.accent);
    CHECK(kPalText == kPalOled.text);

    setPaletteForName("light");
    CHECK(kPalPanel == kPalLight.panel);
    hexEq(kPalBackground, 0xDD, 0xE3, 0xEA);
    hexEq(kPalSurface,    0xFF, 0xFF, 0xFF);
    hexEq(kPalAccent,     0x1C, 0x6D, 0xD9);
    hexEq(kPalText,       0x14, 0x1A, 0x21);
    hexEq(kPalTextDim,    0x4E, 0x5A, 0x66);

    // light inverts the ramp: cards sit lighter than the void behind them
    for (int ch = 0; ch < 3; ++ch) {
        CHECK(kPalSurface[ch] > kPalBackground[ch]);
        CHECK(kPalTextDim[ch] > kPalText[ch]);
    }

    // back to the default so later suites see the dark table
    setPaletteForName("dark");
    CHECK(kPalBackground == kPalDark.background);
    CHECK(kPalPanel == kPalDark.panel);
    CHECK(batteryTint(100, false) == kPalDark.good);
}
