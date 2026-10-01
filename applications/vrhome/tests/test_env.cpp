#include "test.h"

#include "env/envmap.h"
#include "env/zipfile.h"
#include "common/config.h"

#include <zlib.h>
#include <cstring>
#include <vector>

// smallest legal zip writer for fixtures: local header + data per entry,
// then the central directory and the EOCD. method 0 = stored, 8 = deflate
// (pre-compressed bytes passed in).
namespace {

void w16(std::vector<uint8_t>& b, uint16_t v) {
    b.push_back(v & 0xff); b.push_back((v >> 8) & 0xff);
}
void w32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xff);
}
void wbytes(std::vector<uint8_t>& b, const void* p, size_t n) {
    b.insert(b.end(), (const uint8_t*)p, (const uint8_t*)p + n);
}

std::vector<uint8_t> deflateBytes(const uint8_t* src, size_t n) {
    // zip entries carry raw deflate - no zlib wrapper - so compress with a
    // negative windowBits, same as a real packer writes
    std::vector<uint8_t> out(compressBound((uLong)n));
    z_stream s;
    memset(&s, 0, sizeof(s));
    deflateInit2(&s, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    s.next_in = (Bytef*)src;
    s.avail_in = (uInt)n;
    s.next_out = out.data();
    s.avail_out = (uInt)out.size();
    deflate(&s, Z_FINISH);
    out.resize(s.total_out);
    deflateEnd(&s);
    return out;
}

struct Ent {
    const char* name;
    std::vector<uint8_t> data;
    uint16_t method;
};

std::vector<uint8_t> makeZip(const std::vector<Ent>& ents) {
    std::vector<uint8_t> b;
    std::vector<uint32_t> localOffs;
    for (const Ent& e : ents) {
        std::vector<uint8_t> payload = e.data;
        if (e.method == 8)
            payload = deflateBytes(e.data.data(), e.data.size());
        localOffs.push_back((uint32_t)b.size());
        w32(b, 0x04034b50);
        w16(b, 20); w16(b, 0x0800); w16(b, e.method);
        w16(b, 0); w16(b, 0); w32(b, 0);
        w32(b, (uint32_t)payload.size());
        w32(b, (uint32_t)e.data.size());
        w16(b, (uint16_t)strlen(e.name)); w16(b, 0);
        wbytes(b, e.name, strlen(e.name));
        wbytes(b, payload.data(), payload.size());
    }
    const uint32_t cdOff = (uint32_t)b.size();
    for (size_t i = 0; i < ents.size(); ++i) {
        const Ent& e = ents[i];
        const uint32_t csz = e.method == 8
            ? (uint32_t)deflateBytes(e.data.data(), e.data.size()).size()
            : (uint32_t)e.data.size();
        w32(b, 0x02014b50);
        w16(b, 20); w16(b, 20); w16(b, 0x0800); w16(b, e.method);
        w16(b, 0); w16(b, 0); w32(b, 0); w32(b, csz);
        w32(b, (uint32_t)e.data.size());
        w16(b, (uint16_t)strlen(e.name));
        w16(b, 0); w16(b, 0); w16(b, 0); w16(b, 0); w32(b, 0);
        w32(b, localOffs[i]);
        wbytes(b, e.name, strlen(e.name));
    }
    const uint32_t cdSize = (uint32_t)b.size() - cdOff;
    w32(b, 0x06054b50);
    w16(b, 0); w16(b, 0);
    w16(b, (uint16_t)ents.size()); w16(b, (uint16_t)ents.size());
    w32(b, cdSize); w32(b, cdOff); w16(b, 0);
    return b;
}

} // namespace

void testZip() {
    const char* hello = "hello zip world";
    const std::string big(4096, 'x');   // compresses well under deflate
    std::vector<Ent> ents = {
        {"map.json", std::vector<uint8_t>(hello, hello + strlen(hello)), 0},
        {"map.obj", std::vector<uint8_t>(big.begin(), big.end()), 8},
    };
    const std::vector<uint8_t> zip = makeZip(ents);

    std::vector<ZipEntry> list;
    CHECK(zipList(zip.data(), zip.size(), &list));
    CHECK(list.size() == 2);
    CHECK(list[0].name == "map.json");
    CHECK(list[0].method == 0);
    CHECK(list[1].name == "map.obj");
    CHECK(list[1].method == 8);

    std::vector<uint8_t> out;
    CHECK(zipRead(zip.data(), zip.size(), "map.json", &out));
    CHECK(out.size() == strlen(hello));
    CHECK(memcmp(out.data(), hello, out.size()) == 0);

    CHECK(zipRead(zip.data(), zip.size(), "map.obj", &out));
    CHECK(out.size() == big.size());
    CHECK(memcmp(out.data(), big.data(), out.size()) == 0);

    // missing entry, garbage buffer, truncated buffer
    CHECK(!zipRead(zip.data(), zip.size(), "nope", &out));
    const char junk[] = "not a zip at all";
    CHECK(!zipList((const uint8_t*)junk, sizeof(junk), &list));
    CHECK(!zipRead(zip.data(), zip.size() / 2, "map.json", &out));
}

void testEnvSel() {
    CHECK(envModeOf("") == kEnvPassthrough);
    CHECK(envModeOf(nullptr) == kEnvPassthrough);
    CHECK(envModeOf("passthrough") == kEnvPassthrough);
    CHECK(envModeOf("builtin") == kEnvBuiltin);
    CHECK(envModeOf("skyloft") == kEnvCustom);

    CHECK(envIdOk("skyloft"));
    CHECK(envIdOk("my-room_v2.1"));
    CHECK(!envIdOk(""));
    CHECK(!envIdOk("has space"));
    CHECK(!envIdOk("../evil"));
    CHECK(!envIdOk("a/b"));
    CHECK(!envIdOk(".."));
    CHECK(!envIdOk("x..y"));

    char path[128];
    CHECK(envZipPath("skyloft", path, sizeof(path)));
    CHECK(strcmp(path, "/data/local/tmp/hibiscus/envs/skyloft.zip") == 0);
    CHECK(!envZipPath("../evil", path, sizeof(path)));
    char tiny[8];
    CHECK(!envZipPath("skyloft", tiny, sizeof(tiny)));
}

void testEnvMap() {
    // a quad floor at y=0 centred on (10, 0, 20), plus a SpawnUser
    // marker quad off to the side at (30, 0, 40)
    const char* obj =
        "o Floor\n"
        "v 8 0 18\n" "v 8 0 22\n" "v 12 0 22\n" "v 12 0 18\n"
        "f 1 2 3 4\n"
        "o SpawnUser\n"
        "v 29 0 39\n" "v 31 0 39\n" "v 31 0 41\n" "v 29 0 41\n"
        "f 5 6 7 8\n";
    EnvMap m;
    CHECK(envFromObj(obj, strlen(obj), &m));
    // spawn faces never reach the soup: only the floor quad's 2 tris
    CHECK(m.v.size() == 2 * 3 * 6);
    // the marker's bottom-centre (30,0,40) must land at (0,kEnvFloorY,0),
    // shifting the floor to (-20, f, -20) .. (-18, f, -18)
    float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
    for (size_t i = 0; i + 5 < m.v.size(); i += 6) {
        for (int a = 0; a < 3; ++a) {
            if (m.v[i + a] < lo[a]) lo[a] = m.v[i + a];
            if (m.v[i + a] > hi[a]) hi[a] = m.v[i + a];
        }
    }
    CHECK_F(lo[0], -22.0f, 1e-4f);
    CHECK_F(hi[0], -18.0f, 1e-4f);
    CHECK_F(lo[1], kEnvFloorY, 1e-4f);
    CHECK_F(hi[1], kEnvFloorY, 1e-4f);
    CHECK_F(lo[2], -22.0f, 1e-4f);
    CHECK_F(hi[2], -18.0f, 1e-4f);

    // a flat floor faces straight up: full diffuse on top of ambient
    for (size_t i = 0; i + 5 < m.v.size(); i += 6)
        CHECK(m.v[i + 4] > 0.5f);   // g channel well above ambient-only

    // vertex colours: a red floor comes out red
    const char* red =
        "o Floor\n"
        "v 0 0 0 1 0 0\n" "v 1 0 0 1 0 0\n" "v 0 0 1 1 0 0\n"
        "f 1 2 3\n";
    EnvMap rm;
    CHECK(envFromObj(red, strlen(red), &rm));
    CHECK(rm.v[3] > rm.v[4]);
    CHECK(rm.v[3] > rm.v[5]);

    // g lines work the same as o
    const char* g =
        "g SpawnUser\n"
        "v 0 0 0\n" "v 1 0 0\n" "v 0 0 1\n" "f 1 2 3\n"
        "g Box\n"
        "v 0 0 0\n" "v 1 0 0\n" "v 0 1 0\n" "f 4 5 6\n";
    EnvMap gm;
    CHECK(envFromObj(g, strlen(g), &gm));
    CHECK(gm.v.size() == 3 * 6);   // only the Box tri survives

    // no SpawnUser: the map's own bounds pick the standing spot
    const char* plain =
        "v 0 0 0\n" "v 4 0 0\n" "v 4 0 4\n" "v 0 0 4\n"
        "f 1 2 3 4\n";
    EnvMap pm;
    CHECK(envFromObj(plain, strlen(plain), &pm));
    for (size_t i = 0; i + 5 < pm.v.size(); i += 6) {
        CHECK(pm.v[i] >= -2.001f && pm.v[i] <= 2.001f);
        CHECK_F(pm.v[i + 1], kEnvFloorY, 1e-4f);
        CHECK(pm.v[i + 2] >= -2.001f && pm.v[i + 2] <= 2.001f);
    }

    // empty and vertex-only inputs fail cleanly
    EnvMap em;
    CHECK(!envFromObj("", 0, &em));
    CHECK(!envFromObj("v 0 0 0\n", 8, &em));
}
