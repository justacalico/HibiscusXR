#include "mesh.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// face element -> vertex index (1-based, negative counts from the end).
// Texture/normal parts after slashes are skipped. 0 = parse failure
static int faceIndex(const char* p, const char* end, int vcount,
                     const char** next) {
    char* stop;
    long idx = strtol(p, &stop, 10);
    if (stop == p) return 0;
    // swallow "/vt", "//vn" or "/vt/vn" - only the vertex index matters
    while (stop < end && *stop == '/') {
        ++stop;
        while (stop < end && *stop >= '0' && *stop <= '9') ++stop;
    }
    *next = stop;
    if (idx > 0) return (int)idx <= vcount ? (int)idx : 0;
    if (idx < 0) {
        const int rel = vcount + (int)idx + 1;
        return rel > 0 ? rel : 0;
    }
    return 0;
}

bool meshFromObj(const char* text, size_t len, Mesh* out) {
    std::vector<float> verts;
    out->v.clear();
    const char* p = text;
    const char* end = text + len;
    while (p < end) {
        const char* nl = (const char*)memchr(p, '\n', end - p);
        const char* le = nl ? nl : end;
        if (p + 1 < le && p[0] == 'v' && p[1] == ' ') {
            float x, y, z;
            if (sscanf(p + 2, "%f %f %f", &x, &y, &z) == 3) {
                verts.push_back(x);
                verts.push_back(y);
                verts.push_back(z);
            }
        } else if (p + 1 < le && p[0] == 'f' && p[1] == ' ') {
            int idx[32];
            int n = 0;
            const char* q = p + 2;
            while (q < le && n < 32) {
                while (q < le && (*q == ' ' || *q == '\t')) ++q;
                if (q >= le || !(*q == '-' || (*q >= '0' && *q <= '9'))) break;
                idx[n] = faceIndex(q, le, (int)(verts.size() / 3), &q);
                if (!idx[n]) break;
                ++n;
            }
            for (int i = 2; i < n; ++i) {
                const int tri[3] = {idx[0], idx[i - 1], idx[i]};
                for (int t = 0; t < 3; ++t) {
                    const float* v = &verts[(tri[t] - 1) * 3];
                    out->v.push_back(v[0]);
                    out->v.push_back(v[1]);
                    out->v.push_back(v[2]);
                }
            }
        }
        p = nl ? nl + 1 : end;
    }
    return !out->v.empty();
}

void meshFit(Mesh* m, float size) {
    if (m->v.empty()) return;
    float lo[3] = {m->v[0], m->v[1], m->v[2]};
    float hi[3] = {lo[0], lo[1], lo[2]};
    for (size_t i = 3; i < m->v.size(); i += 3)
        for (int a = 0; a < 3; ++a) {
            if (m->v[i + a] < lo[a]) lo[a] = m->v[i + a];
            if (m->v[i + a] > hi[a]) hi[a] = m->v[i + a];
        }
    const float c[3] = {(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2,
                        (lo[2] + hi[2]) / 2};
    float d = 0.0f;
    for (int a = 0; a < 3; ++a) {
        const float e = hi[a] - lo[a];
        if (e > d) d = e;
    }
    if (d <= 0.0f) return;
    const float s = size / d;
    for (size_t i = 0; i < m->v.size(); i += 3)
        for (int a = 0; a < 3; ++a) m->v[i + a] = (m->v[i + a] - c[a]) * s;
}
