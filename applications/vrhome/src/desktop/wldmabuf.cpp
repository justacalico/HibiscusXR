#include "dmabuf.h"

#include "wlegl.h"
#include "wlcomp.h"
#include "../common/log.h"

#include "linux-dmabuf-v1-server-protocol.h"
#include <wayland-server.h>
#include <drm_fourcc.h>

#include <unistd.h>
#include <utility>
#include <vector>

// wl_buffer user data for buffers we built out of dmabuf params
struct DmaBufRes {
    DmaBuf d;
    wl_listener gone;
};

struct DmaParams {
    WlComp* c = nullptr;
    DmaBuf d;
    bool used = false;
    wl_listener gone;
};

static DmaParams* paramsOf(wl_resource* res) {
    return (DmaParams*)wl_resource_get_user_data(res);
}

static void dmaBufDestroyed(wl_listener* lis, void*) {
    DmaBufRes* r = wl_container_of(lis, r, gone);
    for (int i = 0; i < 4; ++i)
        if (r->d.fd[i] >= 0) close(r->d.fd[i]);
    delete r;
}

static void reqDmaBufDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}
static const struct wl_buffer_interface dmaBufImpl = {
    reqDmaBufDestroy
};

DmaBuf* dmabufOf(wl_resource* buf) {
    if (!buf || !wl_resource_instance_of(buf, &wl_buffer_interface,
                                       &dmaBufImpl))
        return nullptr;
    return &((DmaBufRes*)wl_resource_get_user_data(buf))->d;
}

// build the wl_buffer; the params' fds move into it on success
static wl_resource* dmabufCreate(wl_client* cl, wl_resource* paramsRes,
                                 DmaParams* p, uint32_t bufId,
                                 int w, int h, uint32_t fmt,
                                 uint32_t flags) {
    if (w <= 0 || h <= 0) {
        wl_resource_post_error(paramsRes,
            ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_DIMENSIONS,
            "bad dmabuf dimensions");
        return nullptr;
    }
    if (p->d.n <= 0) {
        wl_resource_post_error(paramsRes,
            ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
            "no planes added");
        return nullptr;
    }
    DmaBufRes* r = new DmaBufRes;
    r->d = p->d;
    r->d.w = w; r->d.h = h; r->d.fmt = fmt;
    wl_resource* buf = wl_resource_create(cl, &wl_buffer_interface, 1,
                                          bufId);
    if (!buf) { delete r; return nullptr; }
    wl_resource_set_implementation(buf, &dmaBufImpl, r, nullptr);
    r->gone.notify = dmaBufDestroyed;
    wl_resource_add_destroy_listener(buf, &r->gone);
    // plane fds now belong to the buffer resource
    for (int i = 0; i < 4; ++i) { p->d.fd[i] = -1; }
    p->d.n = 0;
    (void)flags;
    return buf;
}

static void reqParamsDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}

static void reqParamsAdd(wl_client*, wl_resource* res, int32_t fd,
                         uint32_t idx, uint32_t off, uint32_t stride,
                         uint32_t modHi, uint32_t modLo) {
    DmaParams* p = paramsOf(res);
    if (idx >= 4 || p->d.fd[idx] >= 0 || p->used) {
        wl_resource_post_error(res,
            ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_SET,
            "bad dmabuf plane add");
        close(fd);
        return;
    }
    p->d.fd[idx] = fd;
    p->d.off[idx] = off;
    p->d.stride[idx] = stride;
    p->d.mod[idx] = ((uint64_t)modHi << 32) | modLo;
    if ((int)idx + 1 > p->d.n) p->d.n = idx + 1;
}

static void reqParamsCreate(wl_client* cl, wl_resource* res,
                            int32_t w, int32_t h, uint32_t fmt,
                            uint32_t flags) {
    DmaParams* p = paramsOf(res);
    if (p->used) {
        wl_resource_post_error(res,
            ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
            "params already used");
        return;
    }
    p->used = true;
    wl_resource* buf = dmabufCreate(cl, res, p, 0, w, h, fmt, flags);
    if (buf) zwp_linux_buffer_params_v1_send_created(res, buf);
    else zwp_linux_buffer_params_v1_send_failed(res);
}

static void reqParamsCreateImmed(wl_client* cl, wl_resource* res,
                                 uint32_t bufId, int32_t w, int32_t h,
                                 uint32_t fmt, uint32_t flags) {
    DmaParams* p = paramsOf(res);
    if (p->used) {
        wl_resource_post_error(res,
            ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
            "params already used");
        return;
    }
    p->used = true;
    dmabufCreate(cl, res, p, bufId, w, h, fmt, flags);
}

static const struct zwp_linux_buffer_params_v1_interface paramsImpl = {
    reqParamsDestroy,
    reqParamsAdd,
    reqParamsCreate,
    reqParamsCreateImmed,
};

static void paramsDestroyed(wl_listener* lis, void*) {
    DmaParams* p = wl_container_of(lis, p, gone);
    for (int i = 0; i < 4; ++i)
        if (p->d.fd[i] >= 0) close(p->d.fd[i]);
    delete p;
}

static void reqDmaDestroy(wl_client*, wl_resource* res) {
    wl_resource_destroy(res);
}

static void reqDmaParams(wl_client* cl, wl_resource* res,
                         uint32_t id) {
    WlComp* c = (WlComp*)wl_resource_get_user_data(res);
    DmaParams* p = new DmaParams;
    p->c = c;
    wl_resource* pr = wl_resource_create(cl,
        &zwp_linux_buffer_params_v1_interface,
        wl_resource_get_version(res), id);
    wl_resource_set_implementation(pr, &paramsImpl, p, nullptr);
    p->gone.notify = paramsDestroyed;
    wl_resource_add_destroy_listener(pr, &p->gone);
}

static const struct zwp_linux_dmabuf_v1_interface dmaImpl = {
    reqDmaDestroy,
    reqDmaParams,
};

static void bindDma(wl_client* cl, void* data, uint32_t ver,
                    uint32_t id) {
    wl_resource* r = wl_resource_create(cl,
        &zwp_linux_dmabuf_v1_interface, ver > 3 ? 3 : ver, id);
    wl_resource_set_implementation(r, &dmaImpl, data, nullptr);

    std::vector<std::pair<uint32_t, uint64_t>> fmts;
    wlEglFormats(&fmts);
    if (fmts.empty()) {
        const uint32_t base[] = {DRM_FORMAT_ARGB8888,
                                 DRM_FORMAT_XRGB8888};
        for (uint32_t f : base) fmts.emplace_back(f, DRM_FORMAT_MOD_INVALID);
    }
    std::vector<uint32_t> seen;
    for (const auto& fm : fmts) {
        bool dup = false;
        for (uint32_t s : seen) if (s == fm.first) dup = true;
        if (dup) continue;
        seen.push_back(fm.first);
        zwp_linux_dmabuf_v1_send_format(r, fm.first);
    }
    if (wl_resource_get_version(r) >= 3)
        for (const auto& fm : fmts)
            if (fm.second != DRM_FORMAT_MOD_INVALID)
                zwp_linux_dmabuf_v1_send_modifier(r, fm.first,
                    (uint32_t)(fm.second >> 32),
                    (uint32_t)(fm.second & 0xffffffffu));
}

void dmabufInit(WlComp* c) {
    if (!wlEglReady()) {
        LOGI("no egl dmabuf import, skipping dmabuf global");
        return;
    }
    wl_global_create(c->dpy, &zwp_linux_dmabuf_v1_interface, 3, c,
                     bindDma);
}
