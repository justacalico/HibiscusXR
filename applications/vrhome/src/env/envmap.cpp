#include "envmap.h"

#include "../common/config.h"
#include "../render/mesh.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

// fixed key light for the baked lambert term: up, a touch forward-left
static const float kLight[3] = {0.32f, 0.83f, 0.45f};
static const float kAmb = 0.38f, kDif = 0.62f;
// vertices without a colour get this base - mid grey keeps the lambert
// ramp visible instead of clamping to black or blowing out white
static const float kBase[3] = {0.62f, 0.64f, 0.68f};

EnvMode envModeOf(const char* sel) {
    if (!sel || !*sel || strcmp(sel, "passthrough") == 0)
        return kEnvPassthrough;
    if (strcmp(sel, "builtin") == 0) return kEnvBuiltin;
    return kEnvCustom;
}

bool envIdOk(const char* id) {
    if (!id) return false;
    size_t n = strlen(id);
    if (!n || n > 64) return false;
    for (size_t i = 0; i < n; ++i) {
        const char c = id[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '_' ||
                        c == '-';
        if (!ok) return false;
    }
    return strcmp(id, ".") && strcmp(id, "..") && strstr(id, "..") == nullptr;
}

bool envZipPath(const char* id, char* out, size_t outSize) {
    if (!out || !outSize) return false;
    out[0] = 0;
    if (!envIdOk(id)) return false;
    const int n = snprintf(out, outSize, "%s/%s.zip", kEnvDir, id);
    if (n <= 0 || (size_t)n >= outSize) { out[0] = 0; return false; }
    return true;
}

namespace {

struct Parser {
    std::vector<float> pos;    // xyz per vertex
    std::vector<float> col;    // rgb per vertex, only while hasCol is set
    std::vector<char> hasCol;
    std::string part;          // current o/g name
    // bboxes: spawn[0..5] for SpawnUser faces, all[] for everything
    float spawn[6] = {0,0,0,0,0,0};
    float all[6] = {0,0,0,0,0,0};
    bool spawnHit = false;
    bool allHit = false;
};

void growBox(float* b, const float* p) {
    if (p[0] < b[0]) b[0] = p[0];
    if (p[1] < b[1]) b[1] = p[1];
    if (p[2] < b[2]) b[2] = p[2];
    if (p[0] > b[3]) b[3] = p[0];
    if (p[1] > b[4]) b[4] = p[1];
    if (p[2] > b[5]) b[5] = p[2];
}

// first whitespace-delimited token after an o/g line start
void partName(const char* p, const char* le, Parser& ps) {
    while (p < le && (*p == ' ' || *p == '\t')) ++p;
    const char* q = p;
    while (q < le && *q != ' ' && *q != '\t' && *q != '\r') ++q;
    ps.part.assign(p, q - p);
}

void vertex(const char* p, const char* le, Parser& ps) {
    float f[6];
    int n = 0;
    const char* q = p;
    while (q < le && n < 6) {
        while (q < le && (*q == ' ' || *q == '\t')) ++q;
        if (q >= le || !(*q == '-' || *q == '+' || *q == '.' ||
                         (*q >= '0' && *q <= '9'))) break;
        char* stop;
        f[n] = strtof(q, &stop);
        if (stop == q) break;
        ++n;
        q = stop;
    }
    if (n < 3) return;
    for (int a = 0; a < 3; ++a) ps.pos.push_back(f[a]);
    if (n == 6) {
        for (int a = 3; a < 6; ++a) ps.col.push_back(f[a]);
        ps.hasCol.push_back(1);
    } else {
        ps.col.push_back(0); ps.col.push_back(0); ps.col.push_back(0);
        ps.hasCol.push_back(0);
    }
}

void emit(Parser& ps, const int* idx, EnvMap* out) {
    const float* a = &ps.pos[(idx[0] - 1) * 3];
    const float* b = &ps.pos[(idx[1] - 1) * 3];
    const float* c = &ps.pos[(idx[2] - 1) * 3];
    // face normal for the lambert bake; degenerate faces keep ambient
    const float u[3] = {b[0]-a[0], b[1]-a[1], b[2]-a[2]};
    const float w[3] = {c[0]-a[0], c[1]-a[1], c[2]-a[2]};
    float n[3] = {u[1]*w[2]-u[2]*w[1], u[2]*w[0]-u[0]*w[2],
                  u[0]*w[1]-u[1]*w[0]};
    const float nl = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    float sh = kAmb;
    if (nl > 1e-12f) {
        const float d = (n[0]*kLight[0] + n[1]*kLight[1] +
                         n[2]*kLight[2]) / nl;
        sh += kDif * (d > 0.0f ? d : 0.0f);
    }
    for (int t = 0; t < 3; ++t) {
        const int vi = idx[t] - 1;
        const float* p = &ps.pos[vi * 3];
        out->v.push_back(p[0]); out->v.push_back(p[1]); out->v.push_back(p[2]);
        if (ps.hasCol[vi]) {
            for (int k = 0; k < 3; ++k)
                out->v.push_back(ps.col[vi * 3 + k] * sh);
        } else {
            for (int k = 0; k < 3; ++k) out->v.push_back(kBase[k] * sh);
        }
    }
}

void face(const char* p, const char* le, Parser& ps, EnvMap* out) {
    int idx[32];
    int n = 0;
    const char* q = p;
    const int vcount = (int)(ps.pos.size() / 3);
    while (q < le && n < 32) {
        while (q < le && (*q == ' ' || *q == '\t')) ++q;
        if (q >= le || !(*q == '-' || (*q >= '0' && *q <= '9'))) break;
        idx[n] = objFaceIndex(q, le, vcount, &q);
        if (!idx[n]) break;
        ++n;
    }
    const bool spawnPart = ps.part == "SpawnUser";
    for (int i = 2; i < n; ++i) {
        const int tri[3] = {idx[0], idx[i - 1], idx[i]};
        for (int t = 0; t < 3; ++t) {
            const float* v = &ps.pos[(tri[t] - 1) * 3];
            if (!ps.allHit) {
                memcpy(ps.all, v, 12);
                memcpy(ps.all + 3, v, 12);
                ps.allHit = true;
            } else {
                growBox(ps.all, v);
            }
            if (spawnPart) {
                if (!ps.spawnHit) {
                    memcpy(ps.spawn, v, 12);
                    memcpy(ps.spawn + 3, v, 12);
                    ps.spawnHit = true;
                } else {
                    growBox(ps.spawn, v);
                }
            }
        }
        if (!spawnPart) emit(ps, tri, out);
    }
}

} // namespace

bool envFromObj(const char* text, size_t len, EnvMap* out) {
    Parser ps;
    out->v.clear();
    const char* p = text;
    const char* end = text + len;
    while (p < end) {
        const char* nl = (const char*)memchr(p, '\n', end - p);
        const char* le = nl ? nl : end;
        if (p + 1 < le && p[0] == 'v' && p[1] == ' ') {
            vertex(p + 2, le, ps);
        } else if (p + 1 < le && p[0] == 'f' && p[1] == ' ') {
            face(p + 2, le, ps, out);
        } else if (p + 1 < le &&
                   (p[0] == 'o' || p[0] == 'g') && p[1] == ' ') {
            partName(p + 2, le, ps);
        }
        p = nl ? nl + 1 : end;
    }
    if (out->v.empty()) return false;

    // shift the world so the spawn point is the user's floor: SpawnUser's
    // box wins, the whole map's bounds stand in when no part is marked
    float off[3] = {0, 0, 0};
    if (ps.spawnHit || ps.allHit) {
        const float* b = ps.spawnHit ? ps.spawn : ps.all;
        off[0] = -((b[0] + b[3]) * 0.5f);
        off[1] = kEnvFloorY - b[1];
        off[2] = -((b[2] + b[5]) * 0.5f);
    }
    for (size_t i = 0; i + 2 < out->v.size(); i += 6)
        for (int a = 0; a < 3; ++a) out->v[i + a] += off[a];
    return true;
}
