#include "scene_geo.h"

#include <cmath>

static const int   kGridHalf = 10;
static const float kGridStep = 0.5f;

int buildGrid(std::vector<float>& out) {
    out.clear();
    const float e = kGridHalf * kGridStep;
    for (int i = -kGridHalf; i <= kGridHalf; ++i) {
        const float t = i * kGridStep;
        const bool axis = (i == 0);
        const float r = axis ? 0.45f : 0.10f, g = axis ? 0.50f : 0.13f,
                    b = axis ? 0.55f : 0.18f;
        const float pts[4][3] = {{t, -1.2f, -e}, {t, -1.2f, e},
                                 {-e, -1.2f, t}, {e, -1.2f, t}};
        for (int k = 0; k < 4; ++k) {
            out.push_back(pts[k][0]); out.push_back(pts[k][1]);
            out.push_back(pts[k][2]);
            out.push_back(r); out.push_back(g); out.push_back(b);
        }
    }
    return (int)out.size() / 6;
}

static const int kSkySeg = 64;

int buildSky(std::vector<float>& out) {
    out.clear();
    const float R = 30.0f;
    const float rings[4][4] = {  // y, r, g, b
        {-6.0f, 0.012f, 0.016f, 0.026f},
        { 0.0f, 0.085f, 0.105f, 0.150f},
        { 8.0f, 0.040f, 0.050f, 0.075f},
        {26.0f, 0.012f, 0.016f, 0.030f},
    };
    for (int s = 0; s < kSkySeg; ++s) {
        const float a0 = s * 2.0f * (float)M_PI / kSkySeg;
        const float a1 = (s + 1) * 2.0f * (float)M_PI / kSkySeg;
        const float x0 = sinf(a0) * R, z0 = cosf(a0) * R;
        const float x1 = sinf(a1) * R, z1 = cosf(a1) * R;
        for (int r = 0; r < 3; ++r) {
            const float* lo = rings[r];
            const float* hi = rings[r + 1];
            const float quad[4][3] = {{x0, lo[0], z0}, {x1, lo[0], z1},
                                      {x1, hi[0], z1}, {x0, hi[0], z0}};
            const float* cols[4] = {lo + 1, lo + 1, hi + 1, hi + 1};
            const int tris[6] = {0,1,2, 0,2,3};
            for (int t = 0; t < 6; ++t) {
                const int v = tris[t];
                out.push_back(quad[v][0]); out.push_back(quad[v][1]);
                out.push_back(quad[v][2]);
                out.push_back(cols[v][0]); out.push_back(cols[v][1]);
                out.push_back(cols[v][2]);
            }
        }
    }
    return (int)out.size() / 6;
}
