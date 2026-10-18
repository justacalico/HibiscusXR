#include "palette.h"

#include <cstring>

#ifdef __ANDROID__
#include "props.h"
#endif

// The hex behind each row is the Color(0xAARRGGBB) in the Flutter
// theme.dart files; keep the three copies in lockstep.

const Palette kPalDark = {
    {0x13 / 255.0f, 0x13 / 255.0f, 0x18 / 255.0f},
    {0x38 / 255.0f, 0x38 / 255.0f, 0x3F / 255.0f},
    {0x4A / 255.0f, 0x4A / 255.0f, 0x52 / 255.0f},
    {0x5D / 255.0f, 0x5D / 255.0f, 0x66 / 255.0f},
    {0x0A / 255.0f, 0x84 / 255.0f, 0xFF / 255.0f},
    {0xFF / 255.0f, 0x45 / 255.0f, 0x3A / 255.0f},
    {0xFF / 255.0f, 0x9F / 255.0f, 0x0A / 255.0f},
    {0x30 / 255.0f, 0xD1 / 255.0f, 0x58 / 255.0f},
    {0xF5 / 255.0f, 0xF5 / 255.0f, 0xF7 / 255.0f},
    {0x98 / 255.0f, 0x98 / 255.0f, 0xA0 / 255.0f},
};

const Palette kPalOled = {
    {0x00 / 255.0f, 0x00 / 255.0f, 0x00 / 255.0f},
    {0x1E / 255.0f, 0x1E / 255.0f, 0x22 / 255.0f},
    {0x2B / 255.0f, 0x2B / 255.0f, 0x31 / 255.0f},
    {0x3D / 255.0f, 0x3D / 255.0f, 0x44 / 255.0f},
    {0x0A / 255.0f, 0x84 / 255.0f, 0xFF / 255.0f},
    {0xFF / 255.0f, 0x45 / 255.0f, 0x3A / 255.0f},
    {0xFF / 255.0f, 0x9F / 255.0f, 0x0A / 255.0f},
    {0x30 / 255.0f, 0xD1 / 255.0f, 0x58 / 255.0f},
    {0xF5 / 255.0f, 0xF5 / 255.0f, 0xF7 / 255.0f},
    {0x98 / 255.0f, 0x98 / 255.0f, 0xA0 / 255.0f},
};

const Palette kPalLight = {
    {0xD8 / 255.0f, 0xD8 / 255.0f, 0xDE / 255.0f},
    {0xF4 / 255.0f, 0xF4 / 255.0f, 0xF6 / 255.0f},
    {0xFF / 255.0f, 0xFF / 255.0f, 0xFF / 255.0f},
    {0xC9 / 255.0f, 0xC9 / 255.0f, 0xCF / 255.0f},
    {0x00 / 255.0f, 0x7A / 255.0f, 0xFF / 255.0f},
    {0xFF / 255.0f, 0x3B / 255.0f, 0x30 / 255.0f},
    {0xFF / 255.0f, 0x95 / 255.0f, 0x00 / 255.0f},
    {0x34 / 255.0f, 0xC7 / 255.0f, 0x59 / 255.0f},
    {0x1C / 255.0f, 0x1C / 255.0f, 0x1E / 255.0f},
    {0x6E / 255.0f, 0x6E / 255.0f, 0x73 / 255.0f},
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
