// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Wire format between the vmd host tool and the in-OS driver.
 *
 * One fixed packet, little-endian, streamed over TCP. The guest reaches
 * the host through qemu's user-net gateway (10.0.2.2), so no guest-side
 * config or kernel support is needed - the same image works anywhere the
 * route exists.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VMD_PROTO_MAGIC 0x50444d56u /* "VMDP" */
#define VMD_PROTO_VERSION 1
#define VMD_PROTO_PORT 7781
#define VMD_PROTO_HOST "10.0.2.2"

#define VMD_POSE_POSITION_VALID 0x1
#define VMD_POSE_TRACKED 0x2

struct vmd_pose_packet
{
	uint32_t magic;
	uint16_t version;
	uint16_t flags;
	uint32_t seq;
	uint64_t host_ts_ns;
	float quat[4];   // x y z w
	float pos[3];    // metres
	float linvel[3]; // m/s
	float angvel[3]; // rad/s
};

#ifdef __cplusplus
}
#endif
