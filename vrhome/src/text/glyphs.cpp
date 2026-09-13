#include "glyphs.h"

#include "utf8.h"

const Glyph* GlyphSet::find(int c) const {
    for (int i = 0; i < n; ++i)
        if (cp[i] == c) return &g[i];
    return nullptr;
}

Glyph* GlyphSet::add(int c) {
    if (n >= CAP) return nullptr;
    cp[n] = c;
    g[n] = Glyph{};
    return &g[n++];
}

float textWidth(const GlyphSet& set, const char* utf8, float mPerPx) {
    const char* p = utf8;
    float w = 0;
    while (*p) {
        const Glyph* g = set.find(nextCp(p));
        if (g) w += g->advance * mPerPx;
    }
    return w;
}

float emitText(const GlyphSet& set, const char* utf8, float mPerPx,
               std::vector<float>& out) {
    const char* p = utf8;
    float pen = 0;
    while (*p) {
        const Glyph* g = set.find(nextCp(p));
        if (!g) continue;
        if (g->w > 0 && g->h > 0) {
            const float gx = pen + g->xoff * mPerPx;
            const float gtop = -g->yoff * mPerPx;
            const float gbot = gtop - g->h * mPerPx;
            const float gw = g->w * mPerPx;
            const float quad[6][4] = {
                {gx,    gbot, g->u0, g->v1},
                {gx+gw, gbot, g->u1, g->v1},
                {gx+gw, gtop, g->u1, g->v0},
                {gx,    gbot, g->u0, g->v1},
                {gx+gw, gtop, g->u1, g->v0},
                {gx,    gtop, g->u0, g->v0},
            };
            for (auto& q : quad)
                for (int k = 0; k < 4; ++k) out.push_back(q[k]);
        }
        pen += g->advance * mPerPx;
    }
    return pen;
}

void liftText(const float* lv, int n, float x, float y, float z,
              std::vector<float>& out) {
    for (int i = 0; i < n; ++i) {
        out.push_back(x + lv[i*4]);
        out.push_back(y + lv[i*4+1]);
        out.push_back(z);
        out.push_back(lv[i*4+2]);
        out.push_back(lv[i*4+3]);
    }
}

void liftTextPanel(const float* lv, int n, const float o[3], const float r[3],
                   std::vector<float>& out) {
    for (int i = 0; i < n; ++i) {
        out.push_back(o[0] + r[0] * lv[i*4]);
        out.push_back(o[1] + lv[i*4+1]);
        out.push_back(o[2] + r[2] * lv[i*4]);
        out.push_back(lv[i*4+2]);
        out.push_back(lv[i*4+3]);
    }
}
