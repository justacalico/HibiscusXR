#include "test.h"

#include "render/scene_geo.h"
#include "input/keys.h"
#include "common/config.h"

void testSceneGeo() {
    std::vector<float> v;

    // grid: (2*half+1) lines x 4 verts, pos3+col3 each
    int n = buildGrid(v);
    CHECK(n == 21 * 4);
    CHECK(v.size() == (size_t)n * 6);

    // all verts on the floor plane
    for (size_t i = 1; i < v.size(); i += 6)
        CHECK_F(v[i], -1.2f, 1e-6f);

    // axis lines are brighter than regular lines
    bool sawBright = false, sawDim = false;
    for (size_t i = 0; i + 5 < v.size(); i += 6) {
        if (v[i+3] > 0.4f) sawBright = true;
        if (v[i+3] < 0.2f) sawDim = true;
    }
    CHECK(sawBright && sawDim);

    // sky: 64 segments x 3 ring quads x 6 verts
    n = buildSky(v);
    CHECK(n == 64 * 3 * 6);
    CHECK(v.size() == (size_t)n * 6);

    // dome radius ~30, gradient goes from dark to bright horizon and back
    for (size_t i = 0; i + 2 < v.size(); i += 6) {
        const float r = sqrtf(v[i]*v[i] + v[i+2]*v[i+2]);
        CHECK_F(r, 30.0f, 1e-4f);
    }
}

void testKeys() {
    CHECK(isConfirm(kKeyEnter));
    CHECK(isConfirm(kKeyDpadCenter));
    CHECK(isConfirm(kKeyButtonA));
    CHECK(isConfirm(kPicoConfirm));
    CHECK(!isConfirm(4));    // AKEYCODE_BACK
    CHECK(!isConfirm(3));    // AKEYCODE_HOME
    CHECK(!isConfirm(0));
}
