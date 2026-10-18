#include "pixconv.h"

bool pxSupported(uint32_t fmt) {
    return fmt == PX_ARGB8888 || fmt == PX_XRGB8888 ||
           fmt == PX_ABGR8888 || fmt == PX_XBGR8888;
}

int pxStrideAlign(uint32_t fmt) {
    (void)fmt;
    return 4;
}

static inline unsigned char unpremul(unsigned char c, unsigned char a) {
    if (!a) return 0;
    const unsigned v = (unsigned)c * 255u / a;
    return v > 255 ? 255 : (unsigned char)v;
}

void pxBlit(const unsigned char* src, int srcStride, uint32_t fmt,
            int sx, int sy,
            unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h) {
    if (!pxSupported(fmt)) return;
    if (dx < 0) { sx -= dx; w += dx; dx = 0; }
    if (dy < 0) { sy -= dy; h += dy; dy = 0; }
    if (sx < 0) { w += sx; sx = 0; }
    if (sy < 0) { h += sy; sy = 0; }
    if (dx + w > dstW) w = dstW - dx;
    if (dy + h > dstH) h = dstH - dy;
    if (w <= 0 || h <= 0) return;
    const bool swapRB = fmt == PX_ABGR8888 || fmt == PX_XBGR8888;
    const bool hasAlpha = fmt == PX_ARGB8888 || fmt == PX_ABGR8888;
    for (int y = 0; y < h; ++y) {
        const unsigned char* s = src + (size_t)(sy + y) * srcStride +
                                 (size_t)sx * 4;
        unsigned char* d = dst + (size_t)(dy + y) * dstStride +
                           (size_t)dx * 4;
        for (int x = 0; x < w; ++x, s += 4, d += 4) {
            const unsigned char a = hasAlpha ? s[3] : 255;
            // wl_shm is premultiplied: undo so GL's stock blend works
            d[0] = hasAlpha ? unpremul(s[swapRB ? 0 : 2], a)
                            : s[swapRB ? 0 : 2];
            d[1] = hasAlpha ? unpremul(s[1], a) : s[1];
            d[2] = hasAlpha ? unpremul(s[swapRB ? 2 : 0], a)
                            : s[swapRB ? 2 : 0];
            d[3] = a;
        }
    }
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
