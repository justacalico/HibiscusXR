// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 driver interface.
 * @ingroup drv_pn2
 */

#pragma once

#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_auto_prober *
pn2_create_auto_prober(void);

#ifdef __cplusplus
}
#endif
