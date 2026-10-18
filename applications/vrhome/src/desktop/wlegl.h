#pragma once

#include <cstdint>
#include <utility>
#include <vector>

struct DmaBuf;

// resolve the EGL/GLES2 procs once; false when EGL or the dmabuf import
// extension is missing - callers then just skip the dmabuf global
bool wlEglReady();

// dmabuf (format, modifier) pairs the current EGL accepts; empty when
// the query extension is absent (advertise LINEAR-only then)
void wlEglFormats(std::vector<std::pair<uint32_t, uint64_t>>* out);

// import a dmabuf set as an EGLImage; nullptr on failure
void* wlEglImport(const DmaBuf* d);

void wlEglFree(void* img);

// retarget an existing GL texture at an EGLImage
bool wlEglBindTex(unsigned tex, void* img);
