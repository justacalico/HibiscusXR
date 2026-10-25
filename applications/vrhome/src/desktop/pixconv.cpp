#include "pixconv.h"

#include <cstring>

bool pxSupported(uint32_t fmt) {
    return fmt == PX_ARGB8888 || fmt == PX_XRGB8888 ||
           fmt == PX_ABGR8888 || fmt == PX_XBGR8888;
}

int pxStrideAlign(uint32_t fmt) {
    (void)fmt;
    return 4;
}

static inline void clipRect(int* sx, int* sy, int* dx, int* dy,
                            int* w, int* h, int dstW, int dstH) {
    if (*dx < 0) { *sx -= *dx; *w += *dx; *dx = 0; }
    if (*dy < 0) { *sy -= *dy; *h += *dy; *dy = 0; }
    if (*sx < 0) { *w += *sx; *sx = 0; }
    if (*sy < 0) { *h += *sy; *sy = 0; }
    if (*dx + *w > dstW) *w = dstW - *dx;
    if (*dy + *h > dstH) *h = dstH - *dy;
}

void pxBlit(const unsigned char* src, int srcStride, uint32_t fmt,
            int sx, int sy,
            unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h) {
    if (!pxSupported(fmt)) return;
    clipRect(&sx, &sy, &dx, &dy, &w, &h, dstW, dstH);
    if (w <= 0 || h <= 0) return;
    const bool swapRB = fmt == PX_ABGR8888 || fmt == PX_XBGR8888;
    const bool hasAlpha = fmt == PX_ARGB8888 || fmt == PX_ABGR8888;
    if (!swapRB && hasAlpha) {
        for (int y = 0; y < h; ++y)
            memcpy(dst + (size_t)(dy + y) * dstStride + (size_t)dx * 4,
                   src + (size_t)(sy + y) * srcStride + (size_t)sx * 4,
                   (size_t)w * 4);
        return;
    }
    for (int y = 0; y < h; ++y) {
        const unsigned char* s = src + (size_t)(sy + y) * srcStride +
                                 (size_t)sx * 4;
        unsigned char* d = dst + (size_t)(dy + y) * dstStride +
                           (size_t)dx * 4;
        for (int x = 0; x < w; ++x, s += 4, d += 4) {
            d[0] = s[swapRB ? 0 : 2];
            d[1] = s[1];
            d[2] = s[swapRB ? 2 : 0];
            d[3] = hasAlpha ? s[3] : 255;
        }
    }
}

// v*(1/255) with the divide approximated: exact for v <= 65535
static inline unsigned div255(unsigned v) {
    return (v + (v >> 8) + 1) >> 8;
}

void pxBlend(const unsigned char* src, int srcStride, uint32_t fmt,
             int sx, int sy,
             unsigned char* dst, int dstStride, int dstW, int dstH,
             int dx, int dy, int w, int h) {
    if (!pxSupported(fmt)) return;
    clipRect(&sx, &sy, &dx, &dy, &w, &h, dstW, dstH);
    if (w <= 0 || h <= 0) return;
    const bool swapRB = fmt == PX_ABGR8888 || fmt == PX_XBGR8888;
    const bool hasAlpha = fmt == PX_ARGB8888 || fmt == PX_ABGR8888;
    for (int y = 0; y < h; ++y) {
        const unsigned char* s = src + (size_t)(sy + y) * srcStride +
                                 (size_t)sx * 4;
        unsigned char* d = dst + (size_t)(dy + y) * dstStride +
                           (size_t)dx * 4;
        if (!hasAlpha) {
            for (int x = 0; x < w; ++x, s += 4, d += 4) {
                d[0] = s[swapRB ? 0 : 2];
                d[1] = s[1];
                d[2] = s[swapRB ? 2 : 0];
                d[3] = 255;
            }
            continue;
        }
        for (int x = 0; x < w; ++x, s += 4, d += 4) {
            const unsigned sa = s[3];
            if (!sa) continue;
            const unsigned sp0 = s[swapRB ? 0 : 2];
            const unsigned sp1 = s[1];
            const unsigned sp2 = s[swapRB ? 2 : 0];
            if (sa == 255) {
                d[0] = (unsigned char)sp0;
                d[1] = (unsigned char)sp1;
                d[2] = (unsigned char)sp2;
                d[3] = 255;
                continue;
            }
            // premult "over": out = sp + dp*(1 - sa), no division
            const unsigned inv = 255 - sa;
            d[0] = (unsigned char)(sp0 + div255(d[0] * inv));
            d[1] = (unsigned char)(sp1 + div255(d[1] * inv));
            d[2] = (unsigned char)(sp2 + div255(d[2] * inv));
            d[3] = (unsigned char)(sa + div255(d[3] * inv));
        }
    }
}

const unsigned char* cursorDefaultPx(int* w, int* h) {
    // black-edged white arrow: X outline, o fill, . transparent
    static const char* const kRows[] = {
        "X...........",
        "XX..........",
        "XoX.........",
        "XooX........",
        "XoooX.......",
        "XooooX......",
        "XoooooX.....",
        "XooooooX....",
        "XoooooooX...",
        "XooooooooX..",
        "XoooooooooX.",
        "XoooooXXXXX.",
        "XooXoooX....",
        "XoX.XoooX...",
        "XX..XoooX...",
        "X...XoooX...",
        "....XoooX...",
        ".....XXXX...",
    };
    static const int kW = 12, kH = 18;
    static unsigned char px[kW * kH * 4];
    static bool built = false;
    if (!built) {
        built = true;
        for (int y = 0; y < kH; ++y)
            for (int x = 0; x < kW; ++x) {
                const char c = kRows[y][x];
                unsigned char* d = px + (size_t)(y * kW + x) * 4;
                if (c == 'X') { d[3] = 255; }
                else if (c == 'o') { d[0] = d[1] = d[2] = d[3] = 255; }
            }
    }
    *w = kW; *h = kH;
    return px;
}

void pxFill(unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h,
            unsigned char r, unsigned char g, unsigned char b,
            unsigned char a) {
    if (dx < 0) { w += dx; dx = 0; }
    if (dy < 0) { h += dy; dy = 0; }
    if (dx + w > dstW) w = dstW - dx;
    if (dy + h > dstH) h = dstH - dy;
    if (w <= 0 || h <= 0) return;
    for (int y = 0; y < h; ++y) {
        unsigned char* d = dst + (size_t)(dy + y) * dstStride +
                           (size_t)dx * 4;
        for (int x = 0; x < w; ++x, d += 4) {
            d[0] = r; d[1] = g; d[2] = b; d[3] = a;
        }
    }
}
