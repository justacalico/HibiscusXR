// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  qvrservice 6DoF head tracking client for the Pico Neo 2 driver.
 * @ingroup drv_pn2
 */

#pragma once

#include "xrt/xrt_defines.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pn2_qvr;

struct pn2_qvr_pose
{
	struct xrt_quat orientation;
	struct xrt_vec3 position;
	struct xrt_vec3 angular_velocity;
	struct xrt_vec3 linear_velocity;
	uint64_t timestamp_ns; //!< converted to os_monotonic domain
	uint32_t tracking_state;
	uint32_t warning_flags;
	float pose_quality;
	float sensor_quality;
	float camera_quality;
};

/*!
 * Opens the qvrservice client and starts VR mode with positional tracking.
 * Returns NULL when the library or service is unavailable; callers fall
 * back to IMU-only tracking.
 */
struct pn2_qvr *
pn2_qvr_create(void);

/*!
 * Copies the newest head pose into @p out. Returns false while tracking
 * has produced no pose yet or the read failed.
 */
bool
pn2_qvr_get_pose(struct pn2_qvr *q, struct pn2_qvr_pose *out);

/*!
 * Stops VR mode and releases the client.
 */
void
pn2_qvr_destroy(struct pn2_qvr *q);

#ifdef __cplusplus
}
#endif
