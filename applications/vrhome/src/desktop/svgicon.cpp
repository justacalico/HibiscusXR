#include "svgicon.h"

#include "../shell/backend.h"

#include <cstring>

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "../../third_party/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../third_party/nanosvgrast.h"

bool decodeSvgFile(const std::string& path, int size, IconPixels* out) {
    if (size <= 0) return false;
    NSVGimage* img = nsvgParseFromFile(path.c_str(), "px", 96.0f);
    if (!img || img->width <= 0.0f || img->height <= 0.0f) {
        if (img) nsvgDelete(img);
        return false;
    }
    // fit inside the square, then letterbox: the dash always draws icons
    // into a square quad
    const float s = size / (img->width > img->height ? img->width
                                                    : img->height);
    const int sw = (int)(img->width * s + 0.5f);
    const int sh = (int)(img->height * s + 0.5f);
    if (sw <= 0 || sh <= 0) {
        nsvgDelete(img);
        return false;
    }
    std::vector<unsigned char> tmp((size_t)sw * sh * 4);
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(img);
        return false;
    }
    nsvgRasterize(rast, img, 0.0f, 0.0f, s, tmp.data(), sw, sh, sw * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(img);

    out->w = out->h = size;
    out->rgba.assign((size_t)size * size * 4, 0);
    const int ox = (size - sw) / 2, oy = (size - sh) / 2;
    for (int y = 0; y < sh; ++y)
        memcpy(out->rgba.data() + ((size_t)(oy + y) * size + ox) * 4,
               tmp.data() + (size_t)y * sw * 4, (size_t)sw * 4);
    return true;
}
