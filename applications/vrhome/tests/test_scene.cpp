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

    // lines dim toward the far edge: the brightest vertex sits under the
    // centre line, the dimmest at the ring's edge, and none of them is
    // bright enough to fight the chrome
    float hi = 0.0f, lo = 1.0f;
    for (size_t i = 3; i + 2 < v.size(); i += 6) {
        if (v[i] > hi) hi = v[i];
        if (v[i] < lo) lo = v[i];
    }
    CHECK(hi < 0.10f);
    CHECK(lo < hi * 0.6f);

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
    CHECK(isConfirm(kHwConfirm));
    CHECK(!isConfirm(4));    // AKEYCODE_BACK
    CHECK(!isConfirm(3));    // AKEYCODE_HOME
    CHECK(!isConfirm(0));
}
