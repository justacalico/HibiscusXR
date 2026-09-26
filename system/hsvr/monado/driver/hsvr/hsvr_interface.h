// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvr prober interface.
 * @ingroup drv_hsvr
 */

#pragma once

#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_auto_prober *
hsvr_create_auto_prober(void);

#ifdef __cplusplus
}
#endif
