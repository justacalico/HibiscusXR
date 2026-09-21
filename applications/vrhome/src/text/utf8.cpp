#include "utf8.h"

int nextCp(const char*& p) {
    const unsigned char c = (unsigned char)*p++;
    if (c < 0x80) return c;
    int r;
    if      ((c & 0xF8) == 0xF0) r = c & 0x07;
    else if ((c & 0xF0) == 0xE0) r = c & 0x0F;
    else if ((c & 0xE0) == 0xC0) r = c & 0x1F;
    else return c;
    while (*p && (*p & 0xC0) == 0x80) r = (r << 6) | (*p++ & 0x3F);
    return r;
}
