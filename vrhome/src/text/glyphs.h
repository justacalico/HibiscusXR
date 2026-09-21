#pragma once

#include <vector>

// Baked glyph metrics + atlas UVs. Pure data + layout math, unit tested on
// the host with a hand-filled GlyphSet; font.cpp fills it from stb_truetype
// on device.
struct Glyph {
    float u0, v0, u1, v1;     // atlas rect
    float xoff, yoff, w, h;   // px offsets + bitmap size
    float advance;            // px pen advance
    bool valid = false;
};

struct GlyphSet {
    static const int CAP = 512;
    int cp[CAP];
    Glyph g[CAP];
    int n = 0;

    const Glyph* find(int c) const;
    // append a slot for c and return it, or nullptr when the set is full
    Glyph* add(int c);
};

#include <string>

// sum of advances in metres at mPerPx scale
float textWidth(const GlyphSet& set, const char* utf8, float mPerPx);

// longest prefix of utf8 that stays under maxW metres at mPerPx; when text
// is dropped an ellipsis is appended if it still fits
std::string clipText(const GlyphSet& set, const char* utf8, float mPerPx,
                     float maxW);

// tightest vertical extent of the shaped string in text-local units:
// top > 0 above the baseline, bot < 0 below it. false when the string has
// no visible glyphs
bool textBounds(const GlyphSet& set, const char* utf8, float mPerPx,
                float* top, float* bot);

// glyph verts in text-local coords: baseline y=0, +x right, +y up. Appends
// (x, y, u, v) quads, returns the pen advance in metres. dx > 0 emits each
// glyph a second time offset that far in +x: cheap fake bold
float emitText(const GlyphSet& set, const char* utf8, float mPerPx,
               std::vector<float>& out, float dx = 0.0f);

// lift emitted quads to world space, n = quads in lv (4 floats each), output
// is (x, y, z, u, v) tuples. Flat version sits on z plane; panel version
// follows the plane spanned by r and up: o is the baseline start
void liftText(const float* lv, int n, float x, float y, float z,
              std::vector<float>& out);
void liftTextPanel(const float* lv, int n, const float o[3], const float r[3],
                   const float up[3], std::vector<float>& out);
