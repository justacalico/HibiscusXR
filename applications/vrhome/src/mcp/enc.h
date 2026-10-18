#pragma once

#include <string>
#include <vector>

// base64 for MCP image content
std::string b64encode(const unsigned char* data, size_t n);

// RGBA pixels -> PNG bytes, one filter-0 scanline per row
std::vector<unsigned char> pngEncodeRgba(const unsigned char* rgba,
                                         int w, int h);
