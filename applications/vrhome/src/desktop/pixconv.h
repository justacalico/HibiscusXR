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
// w/h, converting to straight-alpha RGBA. dst is premultiply-free: ARGB
// sources get unpremultiplied so the dash's stock alpha blend stays right.
void pxBlit(const unsigned char* src, int srcStride, uint32_t fmt,
            int sx, int sy,
            unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h);

// fills a rect with a solid RGBA colour, clipped like pxBlit
void pxFill(unsigned char* dst, int dstStride, int dstW, int dstH,
            int dx, int dy, int w, int h,
            unsigned char r, unsigned char g, unsigned char b,
            unsigned char a);
