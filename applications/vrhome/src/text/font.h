#pragma once

#include "glyphs.h"

#include <GLES2/gl2.h>
#include "../../third_party/stb_truetype.h"
#include <vector>
#include <cstdint>

struct Engine;

// Runtime font: stb_truetype face + lazily baked alpha-atlas texture.
// Glyph metrics live in `set`; this struct adds the stb handle and GL state.
struct Font {
    stbtt_fontinfo info;
    std::vector<uint8_t> data;
    bool ok = false;
    float scale = 0, ascent = 0;
    static const int PX = 36;
    static const int TEX = 1024;
    GLuint tex = 0;
    int packX = 0, packY = 0, packRowH = 0;
    GlyphSet set;
};

bool loadFont(Engine* e);

// find the glyph for cp, rasterising + uploading it on first use
const Glyph* fontGlyph(Engine* e, int cp);

// bake every codepoint a string needs so the pure layout helpers can read
// the set without touching GL
void ensureGlyphs(Engine* e, const char* utf8);
