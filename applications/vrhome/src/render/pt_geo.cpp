#include "pt_geo.h"

#include <cmath>

// A view ray in GL camera space (x right, y up, z out the back) becomes a
// camera-frame ray (x right, y down, z forward) and projects through the
// equidistant fisheye model: theta is the angle off the lens axis, the
// distorted theta the polynomial r(theta), and the pixel lands at the
// principal point plus that radius along the ray's image-plane bearing.
static void project(const CamIntr& c, float vx, float vy, float vz,
                    float rollDeg, float* px, float* py) {
    // view -> sensor frame: y down and forward is +z
    float x = vx, y = -vy, z = -vz;
    // sensor mounting roll around the optical axis
    if (rollDeg != 0.0f) {
        const float a = rollDeg * (float)M_PI / 180.0f;
        const float cs = cosf(a), sn = sinf(a);
        const float xr = x * cs - y * sn, yr = x * sn + y * cs;
        x = xr; y = yr;
    }
    // undo this camera's rig rotation so the ray is in its own frame
    {
        const float* r = c.rig;   // camera->rig, row major
        const float xr = r[0]*x + r[3]*y + r[6]*z;
        const float yr = r[1]*x + r[4]*y + r[7]*z;
        const float zr = r[2]*x + r[5]*y + r[8]*z;
        x = xr; y = yr; z = zr;
    }
    const float rxy = sqrtf(x * x + y * y);
    const float theta = atan2f(rxy, z);
    const float t2 = theta * theta, t4 = t2 * t2,
                t6 = t4 * t2, t8 = t4 * t4;
    const float td = theta * (1.0f + c.k1 * t2 + c.k2 * t4 +
                              c.k3 * t6 + c.k4 * t8);
    const float sc = rxy > 1e-9f ? td / rxy : td / 1e-9f;
    *px = c.cx + c.fx * x * sc;
    *py = c.cy + c.fy * y * sc;
}

// rotate a projected pixel around the camera's principal point in 90 deg
// steps: 1 = quarter turn ccw. The tracking sensors sit portrait in the
// frame, so the image lands rotated in the half and the fix belongs in
// image space - rolling the view ray instead pushes samples past the
// half's edge and they clamp to stripes
static void rotPx(const CamIntr& c, int quarters, float* px, float* py) {
    float x = *px - c.cx, y = *py - c.cy;
    for (int i = 0; i < quarters; ++i) {
        const float xr = -y, yr = x;
        x = xr; y = yr;
    }
    *px = c.cx + x; *py = c.cy + y;
}

int buildPtMesh(float* verts, int cap, int eye, const CamIntr cams[2],
                int cols, int rows, float tanX, float tanY,
                bool swapEyes, bool flipU, bool flipV, float rollDeg,
                int rotQuarters) {
    if (!verts || cols < 2 || rows < 2) return 0;
    const int need = ptMeshFloats(cols, rows);
    if (cap < need) return 0;
    const CamIntr& cam = cams[(eye == 0) != swapEyes ? 0 : 1];
    float* o = verts;
    for (int gy = 0; gy < rows; ++gy) {
        const float ny0 = 1.0f - 2.0f * gy / rows;
        const float ny1 = 1.0f - 2.0f * (gy + 1) / rows;
        for (int gx = 0; gx < cols; ++gx) {
            const float nx0 = -1.0f + 2.0f * gx / cols;
            const float nx1 = -1.0f + 2.0f * (gx + 1) / cols;
            const float xs[4] = {nx0, nx1, nx1, nx0},
                        ys[4] = {ny0, ny0, ny1, ny1};
            const int tris[6] = {0, 1, 2, 0, 2, 3};
            for (int t = 0; t < 6; ++t) {
                const float nx = xs[tris[t]], ny = ys[tris[t]];
                float px, py;
                project(cam, nx * tanX, ny * tanY, -1.0f,
                        rollDeg, &px, &py);
                rotPx(cam, ((rotQuarters % 4) + 4) % 4, &px, &py);
                if (flipU) px = 640.0f - px;
                if (flipV) py = 400.0f - py;
                // the pair packs into the 1280x400 frame as interleaved
                // columns: camera 0 owns the even ones, camera 1 the odd
                const int cam = (eye == 0) != swapEyes ? 0 : 1;
                const float u = (floorf(px) * 2.0f + (float)cam + 0.5f)
                                / 1280.0f;
                const float v = py / 400.0f;
                o[0] = nx; o[1] = ny; o[2] = 0.0f;
                o[3] = u;  o[4] = v;
                o += 5;
            }
        }
    }
    return cols * rows * 6;
}
