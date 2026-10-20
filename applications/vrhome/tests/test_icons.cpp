#include "test.h"

#include "../src/render/iconatlas.h"

// the baked Material Symbols atlas: every GlyphId must land on a cell
// with real ink inside the atlas bounds
void testIcons() {
    CHECK(iconAtlasW() > 0);
    CHECK(iconAtlasH() > 0);
    CHECK(iconAtlas() != nullptr);
    float uv[4];
    CHECK(!iconUv(-1, uv));
    CHECK(!iconUv(GI_COUNT, uv));
    const int aw = iconAtlasW(), ah = iconAtlasH();
    for (int id = 0; id < GI_COUNT; ++id) {
        CHECK(iconUv(id, uv));
        CHECK(uv[0] >= 0.0f && uv[2] <= 1.0f && uv[0] < uv[2]);
        CHECK(uv[1] >= 0.0f && uv[3] <= 1.0f && uv[1] < uv[3]);
        const int x0 = (int)(uv[0] * aw), x1 = (int)(uv[2] * aw);
        const int y0 = (int)(uv[1] * ah), y1 = (int)(uv[3] * ah);
        long ink = 0;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                ink += iconAtlas()[y * aw + x];
        CHECK(ink > 0);
    }
}
