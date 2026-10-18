#pragma once

#include <cstdint>

struct wl_resource;
struct WlComp;

// plane/fd record behind a zwp_linux_dmabuf_v1 buffer. fds are owned by
// the wl_buffer resource once create lands - closed on buffer destroy
struct DmaBuf {
    int fd[4] = {-1, -1, -1, -1};
    uint32_t off[4] = {0, 0, 0, 0};
    uint32_t stride[4] = {0, 0, 0, 0};
    uint64_t mod[4] = {0, 0, 0, 0};
    int n = 0;
    int w = 0, h = 0;
    uint32_t fmt = 0;
};

// advertise zwp_linux_dmabuf_v1; formats/modifiers come from what the
// current EGL can import (wlegl), LINEAR fallback when it can't say
void dmabufInit(WlComp* c);

// the DmaBuf behind a wl_buffer we created, or nullptr for shm/foreign
DmaBuf* dmabufOf(wl_resource* buf);
