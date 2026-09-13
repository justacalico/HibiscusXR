#include "font.h"

#include "utf8.h"
#include "../engine.h"
#include "../common/log.h"

#include <cstdio>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb_truetype.h"

bool loadFont(Engine* e) {
    static const char* paths[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/DroidSans.ttf",
        "/system/fonts/Roboto-Regular.ttf",
    };
    FILE* f = nullptr;
    for (const char* p : paths) { f = fopen(p, "rb"); if (f) break; }
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    e->font.data.resize(sz);
    fread(e->font.data.data(), 1, sz, f);
    fclose(f);
    const int off = stbtt_GetFontOffsetForIndex(e->font.data.data(), 0);
    if (!stbtt_InitFont(&e->font.info, e->font.data.data(), off)) return false;
    e->font.scale = stbtt_ScaleForPixelHeight(&e->font.info, Font::PX);
    int a, d, lg;
    stbtt_GetFontVMetrics(&e->font.info, &a, &d, &lg);
    e->font.ascent = a * e->font.scale;

    glGenTextures(1, &e->font.tex);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    std::vector<uint8_t> zero(Font::TEX * Font::TEX, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Font::TEX, Font::TEX, 0, GL_ALPHA,
                 GL_UNSIGNED_BYTE, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    e->font.ok = true;
    return true;
}

const Glyph* fontGlyph(Engine* e, int cp) {
    Font& f = e->font;
    if (const Glyph* g = f.set.find(cp)) return g;
    Glyph* g = f.set.add(cp);
    if (!g) return nullptr;

    int adv, lsb;
    stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&f.info, cp, f.scale, f.scale, &x0, &y0, &x1, &y1);
    int gw = x1 - x0, gh = y1 - y0;

    g->xoff = (float)x0; g->yoff = (float)y0;
    g->w = (float)gw; g->h = (float)gh;
    g->advance = adv * f.scale;
    g->valid = true;

    if (gw > 0 && gh > 0) {
        if (f.packX + gw + 3 > Font::TEX) {
            f.packX = 0; f.packY += f.packRowH + 2; f.packRowH = 0;
        }
        if (f.packY + gh + 2 <= Font::TEX) {
            std::vector<uint8_t> bmp(gw * gh);
            stbtt_MakeCodepointBitmap(&f.info, bmp.data(), gw, gh, gw,
                                      f.scale, f.scale, cp);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glBindTexture(GL_TEXTURE_2D, f.tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, f.packX, f.packY, gw, gh,
                            GL_ALPHA, GL_UNSIGNED_BYTE, bmp.data());
            g->u0 = f.packX / (float)Font::TEX;
            g->u1 = (f.packX + gw) / (float)Font::TEX;
            g->v0 = f.packY / (float)Font::TEX;
            g->v1 = (f.packY + gh) / (float)Font::TEX;
            f.packX += gw + 2;
            if (gh > f.packRowH) f.packRowH = gh;
        }
    }
    return g;
}

void ensureGlyphs(Engine* e, const char* utf8) {
    const char* p = utf8;
    while (*p) fontGlyph(e, nextCp(p));
}
