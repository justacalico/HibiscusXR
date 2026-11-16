// Copyright 2025, Hibiscus
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VMD_PROTO_MAGIC 0x50444d56u /* "VMDP" */
#define VMD_PROTO_VERSION 1
#define VMD_PROTO_PORT 7781
#define VMD_PROTO_HOST "10.0.2.2"

/*
 * The hsvc service endpoint: the host also speaks the hsvc frame
 * protocol (system/linux/include/hsvc/proto.h) on this port, so a
 * Linux guest's drv_hsvc consumes the host's virtual motion device
 * exactly like a bus service - no driver daemon needed inside the
 * guest. The client sends HELLO carrying the service name below, the
 * host answers that service's INFO, then SUB/STREAM proceed as usual.
 */
#define VMD_SVC_PORT 7782
#define VMD_SVC_HMD "vmd-hmd"
#define VMD_SVC_CTRL "vmd-ctrl"

#define VMD_POSE_POSITION_VALID 0x1
#define VMD_POSE_TRACKED 0x2

/* wand button bits the host packs into the hsvc_ctrl stream - the
 * values are the HSVC_BTN_* ones so no translation is needed */
#define VMD_BTN_A (1u << 0)
#define VMD_BTN_B (1u << 1)
#define VMD_BTN_MENU (1u << 2)
#define VMD_BTN_SYS (1u << 3)
#define VMD_BTN_STICK (1u << 4)
#define VMD_BTN_TRIG (1u << 5)
#define VMD_BTN_GRIP (1u << 6)

struct vmd_pose_packet
{
	uint32_t magic;
	uint16_t version;
	uint16_t flags;
	uint32_t seq;
	uint32_t reserved;
	uint64_t host_ts_ns;
	float quat[4];   // x y z w
	float pos[3];    // metres
	float linvel[3]; // m/s
	float angvel[3]; // rad/s
};

static_assert(sizeof(struct vmd_pose_packet) == 80, "vmd_pose_packet wire size changed");
static_assert(offsetof(struct vmd_pose_packet, host_ts_ns) == 16,
              "vmd_pose_packet layout changed");

#ifdef __cplusplus
}
#endif
