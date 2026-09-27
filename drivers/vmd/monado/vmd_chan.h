// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Reconnecting read side of the vmd pose channel.
 * @ingroup drv_vmd
 */

#pragma once

#include "vmd_proto.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


struct vmd_chan;

/*!
 * True when the host channel looks reachable: a quick non-blocking
 * connect to @p addr ("host:port", NULL means the qemu default). Used by
 * the kit probe - cheap and leaves no state behind.
 */
bool
vmd_chan_probe(const char *addr);

/*!
 * Opens the channel reader. @p addr NULL = VMD_PROTO_HOST:VMD_PROTO_PORT.
 */
struct vmd_chan *
vmd_chan_open(const char *addr);

/*!
 * Next packet. Blocks until one arrives; reconnects by itself after a
 * drop. Returns false only when @p stop was set.
 */
bool
vmd_chan_read(struct vmd_chan *c, struct vmd_pose_packet *out, volatile bool *stop);

void
vmd_chan_close(struct vmd_chan *c);

#ifdef __cplusplus
}
#endif
