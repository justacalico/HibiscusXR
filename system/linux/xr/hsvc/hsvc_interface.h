// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc prober interface.
 * @ingroup drv_hsvc
 */
#pragma once

#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_auto_prober *
hsvc_create_auto_prober(void);

#ifdef __cplusplus
}
#endif
