#include "test.h"

#include "render/pt_geo.h"

#include <vector>
#include <cmath>

// the device calibration values, same set scene.cpp ships
static CamIntr mkCam(bool right) {
    CamIntr c{};
    if (right) {
        c.fx = c.fy = 283.59317f;
        c.cx = 319.61478f; c.cy = 208.24164f;
        c.k1 = -0.0043927087f; c.k2 = 0.0051424201f;
        c.k3 = 0.0041712555f;  c.k4 = -0.0028565519f;
        const float r[9] = {0.99986655f, 0.016029568f, -0.003151021f,
                            -0.01605548f, 0.99983603f, -0.0083779357f,
                            0.0030162097f, 0.0084274104f, 0.99995995f};
        for (int i = 0; i < 9; ++i) c.rig[i] = r[i];
    } else {
        c.fx = c.fy = 285.48572f;
        c.cx = 319.78693f; c.cy = 207.4184f;
        c.k1 = -0.0084880358f; c.k2 = 0.0071489909f;
        c.k3 = 0.0024346174f;  c.k4 = -0.0022024566f;
        const float r[9] = {1,0,0, 0,1,0, 0,0,1};
        for (int i = 0; i < 9; ++i) c.rig[i] = r[i];
    }
    return c;
}

void testPt() {
    const CamIntr cams[2] = {mkCam(false), mkCam(true)};
    const int cols = 40, rows = 30;
    std::vector<float> verts(ptMeshFloats(cols, rows));
    const float tanX = 1.2f, tanY = 0.7f;

    // the mesh is a full NDC square of cols*rows quads
    const int n = buildPtMesh(verts.data(), (int)verts.size(), 0, cams,
                              cols, rows, tanX, tanY,
                              false, false, true, 0.0f);
    CHECK(n == cols * rows * 6);

    // vertex count sanity: bad inputs refuse to build
    CHECK(buildPtMesh(nullptr, 10, 0, cams, cols, rows,
                      tanX, tanY, false, false, true, 0.0f) == 0);
    CHECK(buildPtMesh(verts.data(), 4, 0, cams, cols, rows,
                      tanX, tanY, false, false, true, 0.0f) == 0);

    // every NDC coordinate sits inside the clip square
    for (int i = 0; i < n; ++i) {
        CHECK(verts[i*5] >= -1.0001f && verts[i*5] <= 1.0001f);
        CHECK(verts[i*5+1] >= -1.0001f && verts[i*5+1] <= 1.0001f);
    }

    // the dead-centre ray lands on the left camera's principal point:
    // u from the image row (turned sideways), v from the column inside
    // the top band
    {
        for (int i = 0; i < n; ++i) {
            if (fabsf(verts[i*5]) < 1e-6f && fabsf(verts[i*5+1]) < 1e-6f) {
                const float u = verts[i*5+3], v = verts[i*5+4];
                CHECK_F(u, 1.0f - 207.4184f / 400.0f, 1e-3f);
                CHECK_F(v, 319.78693f / 1280.0f, 1e-3f);
            }
        }
    }

    // the pair packs as stacked bands: eye 0 stays in the top band
    // (v < 0.5), eye 1 in the bottom one
    {
        for (int i = 0; i < n; ++i) {
            const float v = verts[i*5+4];
            CHECK(v >= 0.0f && v <= 0.5f);
        }
        std::vector<float> v2(ptMeshFloats(cols, rows));
        const int n2 = buildPtMesh(v2.data(), (int)v2.size(), 1, cams,
                                   cols, rows, tanX, tanY,
                                   false, false, true, 0.0f);
        for (int i = 0; i < n2; ++i) {
            const float v = v2[i*5+4];
            CHECK(v >= 0.5f && v <= 1.0f);
        }
    }

    // swapEyes flips the bands: eye 0 then reads the bottom one
    {
        std::vector<float> v2(ptMeshFloats(cols, rows));
        const int n2 = buildPtMesh(v2.data(), (int)v2.size(), 0, cams,
                                   cols, rows, tanX, tanY,
                                   true, false, true, 0.0f);
        CHECK(n2 == n);
        for (int i = 0; i < n2; ++i)
            CHECK(v2[i*5+4] >= 0.5f && v2[i*5+4] <= 1.0f);
    }

    // coverage: the fisheye is wider than the render fov, so no eye
    // vertex should need uv outside [0,1]
    for (int i = 0; i < n; ++i) {
        CHECK(verts[i*5+4] >= -0.001f && verts[i*5+4] <= 1.001f);
    }
}
