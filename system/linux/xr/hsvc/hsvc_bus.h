// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc bus client - dials services, reads typed samples.
 * @ingroup drv_hsvc
 */
#pragma once

#include <stdint.h>

#include "hsvc/hsvc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One connection to a service socket; `desc` is the INFO blob. */
struct hsvcx_conn
{
	int fd;
	char *desc;
};

/* A kind+path pair out of the daemon's LIST. */
struct hsvcx_svc
{
	char name[64];
	char path[256];
};

/* Connect to the service serving `kind` (first registered wins, same
 * as the OS). Reads the INFO blob into desc. 0 ok, -1 absent. */
int hsvcx_connect_kind(enum hsvc_kind kind, struct hsvcx_conn *out);
int hsvcx_connect_path(const char *path, struct hsvcx_conn *out);

/* Dial the vmd host's hsvc endpoint for the named service (one of the
 * VMD_SVC_* names in vmd_proto.h) and read its INFO, same shape as a
 * bus conn. Address order: VMD_ADDR, then the qemu user-net host, then
 * localhost - a vmd running on the same box answers the same way.
 * 0 ok, -1 when no host channel answers. */
int hsvcx_vmd_dial(const char *name, struct hsvcx_conn *out);

int hsvcx_sub(struct hsvcx_conn *c, uint32_t stream);

/* List every registered service of `kind`, up to `max`. Returns count. */
int hsvcx_list(enum hsvc_kind kind, struct hsvcx_svc *out, int max);

/* Block for the next STREAM frame. Fills stream id, timestamp and up to
 * `cap` bytes of payload (truncated past that). Returns payload bytes,
 * or -1 on disconnect/protocol error. PING is answered, other frame
 * types are skipped. */
int hsvcx_next(struct hsvcx_conn *c, uint32_t *stream, uint64_t *ts_ns,
               void *data, uint32_t cap);

void hsvcx_close(struct hsvcx_conn *c);

#ifdef __cplusplus
}
#endif
