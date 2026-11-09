// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc internals shared between the prober and the devices.
 * @ingroup drv_hsvc
 */
#pragma once

#include "hsvc_bus.h"

#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create the HMD device on an already-open hsvc connection (takes
 * ownership; desc is the display geometry contract). NULL on failure. */
struct xrt_device *
hsvcx_hmd_create(struct hsvcx_conn *conn);

/* Create one controller device on an already-open connection serving
 * `stream` (`n` in the service INFO is the stream count; single-stream
 * services just use 0). `index` selects the device type: 0 left hand,
 * 1 right, anything else generic. NULL on failure. */
struct xrt_device *
hsvcx_ctrl_create(struct hsvcx_conn *conn, uint32_t stream, int index);

#ifdef __cplusplus
}
#endif
