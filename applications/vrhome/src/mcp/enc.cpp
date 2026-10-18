#include "enc.h"

#include <cstring>
#include <zlib.h>

std::string b64encode(const unsigned char* data, size_t n) {
    static const char* kT =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((n + 2) / 3 * 4);
    for (size_t i = 0; i < n; i += 3) {
        const unsigned a = data[i];
        const unsigned b = i + 1 < n ? data[i + 1] : 0;
        const unsigned c = i + 2 < n ? data[i + 2] : 0;
        out.push_back(kT[a >> 2]);
        out.push_back(kT[((a & 3) << 4) | (b >> 4)]);
        out.push_back(i + 1 < n ? kT[((b & 15) << 2) | (c >> 6)] : '=');
        out.push_back(i + 2 < n ? kT[c & 63] : '=');
    }
    return out;
}

namespace {

void be32(std::vector<unsigned char>* out, unsigned v) {
    out->push_back((unsigned char)(v >> 24));
    out->push_back((unsigned char)(v >> 16));
    out->push_back((unsigned char)(v >> 8));
    out->push_back((unsigned char)v);
}

void chunk(std::vector<unsigned char>* out, const char type[4],
           const unsigned char* data, size_t n) {
    be32(out, (unsigned)n);
    const size_t at = out->size();
    out->insert(out->end(), type, type + 4);
    if (n) out->insert(out->end(), data, data + n);
    const unsigned crc = (unsigned)crc32(crc32(0L, Z_NULL, 0),
                                       out->data() + at,
                                       (uInt)(out->size() - at));
    be32(out, crc);
}

} // namespace

std::vector<unsigned char> pngEncodeRgba(const unsigned char* rgba,
                                         int w, int h) {
    std::vector<unsigned char> raw((size_t)h * ((size_t)w * 4 + 1));
    for (int y = 0; y < h; ++y) {
        unsigned char* row = raw.data() + (size_t)y * ((size_t)w * 4 + 1);
        row[0] = 0;
        memcpy(row + 1, rgba + (size_t)y * w * 4, (size_t)w * 4);
    }
    uLongf clen = compressBound((uLong)raw.size());
    std::vector<unsigned char> z(clen);
    if (compress2(z.data(), &clen, raw.data(), (uLong)raw.size(),
                  Z_DEFAULT_COMPRESSION) != Z_OK)
        return {};
    z.resize(clen);

    std::vector<unsigned char> png;
    static const unsigned char sig[8] = {0x89, 'P', 'N', 'G',
                                         0x0D, 0x0A, 0x1A, 0x0A};
    png.insert(png.end(), sig, sig + 8);
    unsigned char ihdr[13];
    ihdr[0] = (unsigned char)(w >> 24); ihdr[1] = (unsigned char)(w >> 16);
    ihdr[2] = (unsigned char)(w >> 8);  ihdr[3] = (unsigned char)w;
    ihdr[4] = (unsigned char)(h >> 24); ihdr[5] = (unsigned char)(h >> 16);
    ihdr[6] = (unsigned char)(h >> 8);  ihdr[7] = (unsigned char)h;
    ihdr[8] = 8;   // bit depth
    ihdr[9] = 6;   // RGBA
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(&png, "IHDR", ihdr, sizeof(ihdr));
    chunk(&png, "IDAT", z.data(), z.size());
    chunk(&png, "IEND", nullptr, 0);
    return png;
}
