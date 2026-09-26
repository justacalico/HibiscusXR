#include "palette.h"

#include <cstring>

#ifdef __ANDROID__
#include "props.h"
#endif

// The hex behind each row is the Color(0xAARRGGBB) in the Flutter
// theme.dart files; keep the three copies in lockstep.

const Palette kPalDark = {
    {0x14 / 255.0f, 0x1A / 255.0f, 0x21 / 255.0f},
    {0x1B / 255.0f, 0x23 / 255.0f, 0x2D / 255.0f},
    {0x23 / 255.0f, 0x2D / 255.0f, 0x38 / 255.0f},
    {0x2E / 255.0f, 0x3A / 255.0f, 0x47 / 255.0f},
    {0x4E / 255.0f, 0x9C / 255.0f, 0xFF / 255.0f},
    {0xFF / 255.0f, 0x5E / 255.0f, 0x5E / 255.0f},
    {0xF5 / 255.0f, 0xC5 / 255.0f, 0x42 / 255.0f},
    {0x3D / 255.0f, 0xD6 / 255.0f, 0x8C / 255.0f},
    {0xF2 / 255.0f, 0xF5 / 255.0f, 0xF8 / 255.0f},
    {0x9A / 255.0f, 0xA7 / 255.0f, 0xB4 / 255.0f},
};

const Palette kPalOled = {
    {0x00 / 255.0f, 0x00 / 255.0f, 0x00 / 255.0f},
    {0x0B / 255.0f, 0x0F / 255.0f, 0x13 / 255.0f},
    {0x15 / 255.0f, 0x1A / 255.0f, 0x20 / 255.0f},
    {0x21 / 255.0f, 0x2A / 255.0f, 0x33 / 255.0f},
    {0x4E / 255.0f, 0x9C / 255.0f, 0xFF / 255.0f},
    {0xFF / 255.0f, 0x5E / 255.0f, 0x5E / 255.0f},
    {0xF5 / 255.0f, 0xC5 / 255.0f, 0x42 / 255.0f},
    {0x3D / 255.0f, 0xD6 / 255.0f, 0x8C / 255.0f},
    {0xF2 / 255.0f, 0xF5 / 255.0f, 0xF8 / 255.0f},
    {0x9A / 255.0f, 0xA7 / 255.0f, 0xB4 / 255.0f},
};

const Palette kPalLight = {
    {0xDD / 255.0f, 0xE3 / 255.0f, 0xEA / 255.0f},
    {0xE9 / 255.0f, 0xEE / 255.0f, 0xF4 / 255.0f},
    {0xFF / 255.0f, 0xFF / 255.0f, 0xFF / 255.0f},
    {0xC7 / 255.0f, 0xD0 / 255.0f, 0xDB / 255.0f},
    {0x1C / 255.0f, 0x6D / 255.0f, 0xD9 / 255.0f},
    {0xD9 / 255.0f, 0x36 / 255.0f, 0x36 / 255.0f},
    {0x8F / 255.0f, 0x64 / 255.0f, 0x00 / 255.0f},
    {0x1E / 255.0f, 0x9E / 255.0f, 0x5A / 255.0f},
    {0x14 / 255.0f, 0x1A / 255.0f, 0x21 / 255.0f},
    {0x4E / 255.0f, 0x5A / 255.0f, 0x66 / 255.0f},
};

const float* kPalBackground  = kPalDark.background;
const float* kPalPanel       = kPalDark.panel;
const float* kPalSurface     = kPalDark.surface;
const float* kPalSurfaceHigh = kPalDark.surfaceHigh;
const float* kPalAccent      = kPalDark.accent;
const float* kPalDanger      = kPalDark.danger;
const float* kPalWarn        = kPalDark.warn;
const float* kPalGood        = kPalDark.good;
const float* kPalText        = kPalDark.text;
const float* kPalTextDim     = kPalDark.textDim;

const Palette& paletteForName(const char* name) {
    if (name) {
        if (strcmp(name, "light") == 0) return kPalLight;
        if (strcmp(name, "oled") == 0) return kPalOled;
    }
    return kPalDark;
}

void setPalette(const Palette& p) {
    kPalBackground  = p.background;
    kPalPanel       = p.panel;
    kPalSurface     = p.surface;
    kPalSurfaceHigh = p.surfaceHigh;
    kPalAccent      = p.accent;
    kPalDanger      = p.danger;
    kPalWarn        = p.warn;
    kPalGood        = p.good;
    kPalText        = p.text;
    kPalTextDim     = p.textDim;
}

void setPaletteForName(const char* name) {
    setPalette(paletteForName(name));
}

void syncPalette() {
#ifdef __ANDROID__
    char name[16];
    propS("persist.hibiscus.theme", name, sizeof(name));
    setPaletteForName(name);
#endif
}
