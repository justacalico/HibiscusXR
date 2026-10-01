#include "zipfile.h"

#include <zlib.h>

#include <cstring>

// zip record signatures
static const uint32_t kSigEocd = 0x06054b50;   // end of central directory
static const uint32_t kSigCen  = 0x02014b50;   // central directory entry
static const uint32_t kSigLoc  = 0x04034b50;   // local file header
static const uint16_t kFlagDataDesc = 0x0008;  // sizes trail the data
static const uint16_t kFlagUtf8 = 0x0800;

static uint16_t r16(const uint8_t* p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t r32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// the EOCD sits in the last 64K + 22 bytes; scan backwards for its signature
static const uint8_t* findEocd(const uint8_t* data, size_t len) {
    if (len < 22) return nullptr;
    const size_t back = len < 22 + 0xffff ? len - 22 : 0xffff;
    for (size_t i = back; ; --i) {
        if (r32(data + i) == kSigEocd) return data + i;
        if (i == 0) break;
    }
    return nullptr;
}

bool zipList(const uint8_t* data, size_t len, std::vector<ZipEntry>* out) {
    out->clear();
    const uint8_t* eocd = findEocd(data, len);
    if (!eocd) return false;
    const uint16_t count = r16(eocd + 10);
    const uint32_t cdOff = r32(eocd + 16);
    size_t p = cdOff;
    for (uint16_t i = 0; i < count; ++i) {
        if (p + 46 > len || r32(data + p) != kSigCen) break;
        const uint16_t flags = r16(data + p + 8);
        const uint16_t nlen = r16(data + p + 28);
        const uint16_t elen = r16(data + p + 30);
        const uint16_t clen = r16(data + p + 32);
        if (p + 46 + nlen > len) break;
        // data-descriptor entries can't be read without decompressing to
        // find the data end - the packers we care about never write them
        if (!(flags & kFlagDataDesc)) {
            ZipEntry ze;
            ze.name.assign((const char*)(data + p + 46), nlen);
            ze.method = r16(data + p + 10);
            ze.csize = r32(data + p + 20);
            ze.usize = r32(data + p + 24);
            ze.localOff = r32(data + p + 42);
            out->push_back(ze);
        }
        p += 46 + nlen + elen + clen;
    }
    return !out->empty() || count == 0;
}

// a central-directory entry's bytes sit behind its local header, which
// repeats the name/extra lengths - the local copies are the ones that count
static const uint8_t* entryData(const uint8_t* data, size_t len,
                                const ZipEntry& ze) {
    const size_t l = ze.localOff;
    if (l + 30 > len || r32(data + l) != kSigLoc) return nullptr;
    const uint16_t nlen = r16(data + l + 26);
    const uint16_t elen = r16(data + l + 28);
    const size_t off = l + 30 + nlen + elen;
    if (off + ze.csize > len) return nullptr;
    return data + off;
}

static bool inflateRaw(const uint8_t* src, size_t srcLen,
                       std::vector<uint8_t>* out, size_t usize) {
    out->resize(usize);
    if (usize == 0) return true;
    z_stream s;
    memset(&s, 0, sizeof(s));
    s.next_in = (Bytef*)src;
    s.avail_in = (uInt)srcLen;
    s.next_out = out->data();
    s.avail_out = (uInt)usize;
    // -15: raw deflate, no zlib header - that's what zip entries carry
    if (inflateInit2(&s, -15) != Z_OK) return false;
    const int rc = inflate(&s, Z_FINISH);
    inflateEnd(&s);
    if (rc != Z_STREAM_END || s.total_out != usize) {
        out->clear();
        return false;
    }
    return true;
}

bool zipRead(const uint8_t* data, size_t len, const char* name,
             std::vector<uint8_t>* out) {
    out->clear();
    std::vector<ZipEntry> entries;
    if (!zipList(data, len, &entries)) return false;
    for (const ZipEntry& ze : entries) {
        if (ze.name != name) continue;
        const uint8_t* raw = entryData(data, len, ze);
        if (!raw) return false;
        if (ze.method == 0) {
            out->assign(raw, raw + ze.csize);
            return ze.csize == ze.usize;
        }
        if (ze.method == 8)
            return inflateRaw(raw, ze.csize, out, ze.usize);
        return false;   // unknown compression
    }
    return false;
}
