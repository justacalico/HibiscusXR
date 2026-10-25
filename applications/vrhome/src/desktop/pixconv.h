#pragma once

#include <cstddef>
#include <cstdint>

// wl_shm formats we understand. Values mirror wl_shm_format so the glue
// passes protocol codes straight through.
enum DashPx {
    PX_ARGB8888 = 0,
    PX_XRGB8888 = 1,
    PX_ABGR8888 = 0x34324241,
    PX_XBGR8888 = 0x32324258,
};

bool pxSupported(uint32_t fmt);
int pxStrideAlign(uint32_t fmt);

// Copies the src rect (sx,sy,w,h) into dst at (dx,dy), clipped to dst's
// w/h. Both sides stay premultiplied: wl_shm comes in premult and the
// dash's panel blend expects premult, so this is a swizzle, not a
// conversion.
void pxBlit(const unsigned char* src, int srcStride, uint32_t fmt,
            int sx, int sy,
            unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h);

// same as pxBlit but alpha-composites over dst instead of copying:
// src is premultiplied wl_shm, dst stays premultiplied
void pxBlend(const unsigned char* src, int srcStride, uint32_t fmt,
             int sx, int sy,
             unsigned char* dst, int dstStride, int dstW, int dstH,
             int dx, int dy, int w, int h);

// fills a rect with a solid RGBA colour, clipped like pxBlit
void pxFill(unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h,
            unsigned char r, unsigned char g, unsigned char b,
            unsigned char a);

// built-in pointer image for clients that never call set_cursor:
// premultiplied PX_ARGB8888 bytes, hotspot at 0,0
const unsigned char* cursorDefaultPx(int* w, int* h);
