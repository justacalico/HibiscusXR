// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 HMD creation.
 * @ingroup drv_pn2
 */

#pragma once

#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_device *
pn2_hmd_create(void);

#ifdef __cplusplus
}
#endif
