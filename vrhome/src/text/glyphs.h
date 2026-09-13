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

// sum of advances in metres at mPerPx scale
float textWidth(const GlyphSet& set, const char* utf8, float mPerPx);

// glyph verts in text-local coords: baseline y=0, +x right, +y up. Appends
// (x, y, u, v) quads, returns the pen advance in metres
float emitText(const GlyphSet& set, const char* utf8, float mPerPx,
               std::vector<float>& out);

// lift emitted quads to world space, n = quads in lv (4 floats each), output
// is (x, y, z, u, v) tuples. Flat version sits on z plane; panel version
// follows a yawed plane: o is the baseline start, r the plane's right vector
void liftText(const float* lv, int n, float x, float y, float z,
              std::vector<float>& out);
void liftTextPanel(const float* lv, int n, const float o[3], const float r[3],
                   std::vector<float>& out);
