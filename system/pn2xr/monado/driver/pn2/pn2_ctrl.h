// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 CV controller device.
 * @ingroup drv_pn2
 */
#pragma once

#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif

// which: 0 left, 1 right. Returns NULL when the sharemem channel cannot
// be opened - callers treat NULL as "no controller on this side".
struct xrt_device *
pn2_ctrl_create(int which);

#ifdef __cplusplus
}
#endif
