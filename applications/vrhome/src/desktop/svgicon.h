#pragma once

#include <string>

struct IconPixels;

// rasterizes an svg icon to straight RGBA at `size` px square; the
// letterboxed center keeps non-square art from stretching on the quad
bool decodeSvgFile(const std::string& path, int size, IconPixels* out);
