#include "test.h"

#include "text/utf8.h"
#include "text/glyphs.h"

#include <cstring>

static GlyphSet asciiSet() {
    GlyphSet s;
    for (int c = 'a'; c <= 'z'; ++c) {
        Glyph* g = s.add(c);
        g->w = 10; g->h = 10; g->advance = 12;
        g->u0 = 0; g->v0 = 0; g->u1 = 0.1f; g->v1 = 0.1f;
    }
    // space: advance only, no bitmap
    Glyph* sp = s.add(' ');
    sp->w = 0; sp->h = 0; sp->advance = 6;
    return s;
}

void testText() {
    // utf8 decode: ascii, 2-byte, 3-byte, malformed
    const char* p = "A";
    CHECK(nextCp(p) == 'A' && *p == 0);

    const char* e = "\xC3\xA9";           // U+00E9
    CHECK(nextCp(e) == 0xE9 && *e == 0);

    const char* zh = "\xE4\xB8\xAD";      // U+4E2D
    CHECK(nextCp(zh) == 0x4E2D && *zh == 0);

    const char* bad = "\xFF" "x";         // lone invalid byte
    CHECK(nextCp(bad) == 0xFF);
    CHECK(*bad == 'x');

    // glyph set find/add
    GlyphSet s;
    CHECK(s.find('a') == nullptr);
    Glyph* ga = s.add('a');
    CHECK(ga != nullptr);
    ga->advance = 10;
    const Glyph* f = s.find('a');
    CHECK(f != nullptr && f->advance == 10);
    CHECK(s.find('b') == nullptr);

    // capacity cap
    GlyphSet full;
    for (int i = 0; i < GlyphSet::CAP; ++i)
        CHECK(full.add(1000 + i) != nullptr);
    CHECK(full.add(9999) == nullptr);
    CHECK(full.find(1000) != nullptr);

    // width = sum of advances * scale, space counts but draws nothing
    GlyphSet as = asciiSet();
    CHECK_F(textWidth(as, "ab", 0.01f), 0.24f, 1e-6f);
    CHECK_F(textWidth(as, "a b", 0.01f), 0.30f, 1e-6f);
    CHECK_F(textWidth(as, "ab", 0.02f), 0.48f, 1e-6f);
    CHECK_F(textWidth(as, "", 0.01f), 0.0f, 1e-6f);
    // unbaked glyphs are skipped, not a crash
    CHECK_F(textWidth(as, "\xE4\xB8\xAD", 0.01f), 0.0f, 1e-6f);

    // emit: 2 visible glyphs -> 2 quads of 6 verts x 4 floats
    std::vector<float> lv;
    float pen = emitText(as, "a b", 0.01f, lv);
    CHECK_F(pen, 0.30f, 1e-6f);
    CHECK(lv.size() == 2 * 6 * 4);
    // first quad sits at pen 0, second after 'a'+space advance
    CHECK_F(lv[0], 0.0f, 1e-6f);
    CHECK_F(lv[24], 0.18f, 1e-5f);

    // lift flat: verts become x/y/z + uv
    std::vector<float> w;
    liftText(lv.data(), 12, 1.0f, 2.0f, -3.0f, w);
    CHECK(w.size() == 12 * 5);
    CHECK_F(w[0], 1.0f + lv[0], 1e-6f);
    CHECK_F(w[1], 2.0f + lv[1], 1e-6f);
    CHECK_F(w[2], -3.0f, 1e-6f);
    CHECK_F(w[3], lv[2], 1e-6f);

    // lift onto a yawed plane: x follows the right vector
    std::vector<float> wp;
    const float o[3] = {0, 0, -2.2f};
    const float rr[3] = {0, 0, 1};   // panel turned 90 deg
    liftTextPanel(lv.data(), 12, o, rr, wp);
    CHECK_F(wp[0], 0.0f + rr[0] * lv[0], 1e-6f);
    CHECK_F(wp[2], -2.2f + rr[2] * lv[0], 1e-6f);

    // textBounds: a glyph with yoff -20 and h 25 spans -5 below to +20 above
    // the baseline at scale 1; a space-only string has no bounds
    GlyphSet bs;
    Glyph* bg = bs.add('x');
    bg->w = 10; bg->h = 25; bg->yoff = -20; bg->advance = 12;
    float top, bot;
    CHECK(textBounds(bs, "x", 1.0f, &top, &bot));
    CHECK_F(top, 20.0f, 1e-6f);
    CHECK_F(bot, -5.0f, 1e-6f);
    CHECK(!textBounds(bs, " ", 1.0f, &top, &bot));
    CHECK(!textBounds(bs, "", 1.0f, &top, &bot));
    CHECK(!textBounds(bs, "z", 1.0f, &top, &bot));   // unbaked

    // bold emit: dx>0 doubles the quads, second copy offset in x
    lv.clear();
    emitText(as, "ab", 0.01f, lv, 0.002f);
    CHECK(lv.size() == 2 * 2 * 6 * 4);
    CHECK_F(lv[24], 0.002f, 1e-6f);   // second copy of 'a' shifted by dx
    CHECK_F(lv[48], 0.12f, 1e-5f);    // first copy of 'b' at pen advance
    // same advance as the regular emit
    CHECK_F(emitText(as, "ab", 0.01f, lv, 0.002f), 0.24f, 1e-6f);
}
