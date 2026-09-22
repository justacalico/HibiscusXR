// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 CV controller: decodes the CVService sharemem channel
 *         into a pair of tracked input devices.
 *
 * The wire decoder lives in ctrl_state.c - the same file the vrhome dash
 * compiles, copied into this tree by build.sh. This file only adapts the
 * decoded state to xrt_device: freshness-based connect, button/axis input
 * slots, pose rebase into the OpenXR view frame, battery.
 * @ingroup drv_pn2
 */

#include "pn2_interface.h"
#include "pn2_ctrl.h"

#include "ctrl_state.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "math/m_api.h"

#include "os/os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


DEBUG_GET_ONCE_LOG_OPTION(pn2_ctrl_log, "PN2_CTRL_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_NUM_OPTION(pn2_ctrl_axismap, "PN2_CTRL_AXISMAP", -1)
DEBUG_GET_ONCE_FLOAT_OPTION(pn2_ctrl_scale, "PN2_CTRL_SCALE", 0.001f)

// a block that stopped changing for this long is a dead link - the service
// rewrites it per packet while a controller streams
#define PN2_CTRL_LIVE_NS 800000000ull

// sharemem open retry while the service hasn't created the file yet
#define PN2_CTRL_RETRY_NS 2000000000ull

/*!
 * @implements xrt_device
 */
struct pn2_ctrl
{
	struct xrt_device base;
	int which;

	struct ctrl_share share;
	bool share_open;
	uint64_t share_retry_ns;

	// freshness tracking: a streaming block's hash never sits still
	uint64_t hash;
	uint64_t change_ns;
	bool seen;
	bool connected;
	bool logged_state;

	struct ctrl_state cur;
	enum u_logging_level log_level;
};

static inline struct pn2_ctrl *
pn2_ctrl(struct xrt_device *xdev)
{
	return (struct pn2_ctrl *)xdev;
}

#define PN2_CTRL_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define PN2_CTRL_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define PN2_CTRL_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

// input slots
enum
{
	PN2C_GRIP_POSE = 0,
	PN2C_AIM_POSE,
	PN2C_AB_CLICK,   // A on the right, X on the left
	PN2C_BB_CLICK,   // B on the right, Y on the left
	PN2C_MENU_CLICK,
	PN2C_SYSTEM_CLICK,
	PN2C_TRIGGER_CLICK,
	PN2C_TRIGGER_VALUE,
	PN2C_STICK,      // the touchpad doubles as the stick axes
	PN2C_STICK_CLICK,
	PN2C_SQUEEZE_CLICK,
	PN2C_SQUEEZE_VALUE,
	PN2C_COUNT
};

// same rebase the head applies to qvrd poses: tracking world has +X up
static const struct xrt_quat PN2_CTRL_WORLD_TO_VIEW = {
    .x = 0.0f, .y = 0.0f, .z = 0.7071068f, .w = 0.7071068f}; // rotZ(+90)

// controller-local frame fixups, picked by PN2_CTRL_AXISMAP; unverified
// like the head maps were - default turns the stock device frame into the
// view frame the same way the head's does
static const struct xrt_quat PN2_CTRL_DEV_MAPS[] = {
    // 0: same rebase as the head (rotZ(-90))
    {.x = 0.0f, .y = 0.0f, .z = -0.7071068f, .w = 0.7071068f},
    // 1: identity
    {.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f},
    // 2: rotX(-90)
    {.x = -0.7071068f, .y = 0.0f, .z = 0.0f, .w = 0.7071068f},
    // 3: rotY(180)
    {.x = 0.0f, .y = 1.0f, .z = 0.0f, .w = 0.0f},
};

static const struct xrt_quat *
pn2_ctrl_dev_map(void)
{
	int map = (int)debug_get_num_option_pn2_ctrl_axismap();
	if (map < 0) {
		char prop[92];
		prop[0] = 0;
		extern int __system_property_get(const char *, char *);
		if (__system_property_get("debug.pn2.ctrlaxismap", prop) > 0) {
			map = atoi(prop);
		}
	}
	if (map < 0 || map >= (int)ARRAY_SIZE(PN2_CTRL_DEV_MAPS)) {
		map = 0;
	}
	return &PN2_CTRL_DEV_MAPS[map];
}

static xrt_result_t
pn2_ctrl_update_inputs(struct xrt_device *xdev)
{
	struct pn2_ctrl *d = pn2_ctrl(xdev);
	uint64_t now = os_monotonic_get_ns();

	if (!d->share_open) {
		if (now < d->share_retry_ns) {
			return XRT_SUCCESS;
		}
		d->share_retry_ns = now + PN2_CTRL_RETRY_NS;
		if (ctrl_share_open(&d->share, NULL) != 0) {
			return XRT_SUCCESS;
		}
		d->share_open = true;
		PN2_CTRL_INFO(d, "sharemem mapped");
	}

	uint8_t buf[CTRL_SHARE_SIZE];
	if (ctrl_share_snapshot(&d->share, buf) != 0) {
		ctrl_share_close(&d->share);
		d->share_open = false;
		return XRT_SUCCESS;
	}

	const uint64_t hash = ctrl_state_hash(buf, d->which);
	if (hash != d->hash) {
		d->hash = hash;
		// first read is just a baseline: the file can sit stale for
		// hours, so a block only counts live once it moved after that
		d->change_ns = d->seen ? now : now - PN2_CTRL_LIVE_NS;
		d->seen = true;
	}
	const bool live = d->seen && now - d->change_ns < PN2_CTRL_LIVE_NS;
	if (live != d->connected) {
		d->connected = live;
		PN2_CTRL_INFO(d, "ctrl %d %s", d->which, live ? "connected" : "lost");
	}

	if (live) {
		ctrl_state_decode(buf, d->which, &d->cur);
	}

	const struct ctrl_keys *k = &d->cur.keys;
	const bool on = d->connected && d->cur.keys_ok;
	struct xrt_input *in = d->base.inputs;

	in[PN2C_AB_CLICK].value.boolean = on && k->a;
	in[PN2C_BB_CLICK].value.boolean = on && k->b;
	in[PN2C_MENU_CLICK].value.boolean = on && k->app;
	in[PN2C_SYSTEM_CLICK].value.boolean = on && k->home;
	in[PN2C_TRIGGER_CLICK].value.boolean = on && k->trigger;
	in[PN2C_TRIGGER_VALUE].value.vec1.x = on && k->trigger ? 1.0f : 0.0f;
	in[PN2C_STICK].value.vec2.x = on ? (k->touch_x - 128) / 128.0f : 0.0f;
	in[PN2C_STICK].value.vec2.y = on ? (k->touch_y - 128) / 128.0f : 0.0f;
	in[PN2C_STICK_CLICK].value.boolean = on && k->rocker;
	in[PN2C_SQUEEZE_CLICK].value.boolean = on && (k->grip_l || k->grip_r);
	in[PN2C_SQUEEZE_VALUE].value.vec1.x =
	    on && (k->grip_l || k->grip_r) ? 1.0f : 0.0f;

	return XRT_SUCCESS;
}

static xrt_result_t
pn2_ctrl_get_tracked_pose(struct xrt_device *xdev,
                          enum xrt_input_name name,
                          int64_t at_timestamp_ns,
                          struct xrt_space_relation *out_relation)
{
	struct pn2_ctrl *d = pn2_ctrl(xdev);
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;

	if ((name == XRT_INPUT_PICO_NEO3_GRIP_POSE || name == XRT_INPUT_PICO_NEO3_AIM_POSE) &&
	    d->connected && d->cur.pose_ok) {
		const struct ctrl_pose *p = &d->cur.fuse;
		const float scale = debug_get_float_option_pn2_ctrl_scale();
		struct xrt_vec3 pos = {p->x * scale, p->y * scale, p->z * scale};
		struct xrt_quat q = {p->qx, p->qy, p->qz, p->q0};

		struct xrt_quat tmp;
		math_quat_rotate(&PN2_CTRL_WORLD_TO_VIEW, &q, &tmp);
		math_quat_rotate(&tmp, pn2_ctrl_dev_map(), &rel.pose.orientation);
		math_quat_rotate_vec3(&PN2_CTRL_WORLD_TO_VIEW, &pos, &rel.pose.position);
		rel.relation_flags = (enum xrt_space_relation_flags)(
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		    XRT_SPACE_RELATION_POSITION_VALID_BIT);
		// the fused block's status word is the MCU's tracking state -
		// sentinel poses report 0, so a live-but-untracked controller
		// still gets a valid relation without the tracked bits
		if (p->status != 0) {
			rel.relation_flags = (enum xrt_space_relation_flags)(
			    rel.relation_flags |
			    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
			    XRT_SPACE_RELATION_POSITION_TRACKED_BIT);
		}
	}

	*out_relation = rel;
	return XRT_SUCCESS;
}

static xrt_result_t
pn2_ctrl_get_battery_status(struct xrt_device *xdev,
                            bool *out_present,
                            bool *out_charging,
                            float *out_charge)
{
	struct pn2_ctrl *d = pn2_ctrl(xdev);
	*out_present = d->connected && d->cur.keys_ok && d->cur.keys.battery >= 0;
	*out_charging = false;
	*out_charge = *out_present ? d->cur.keys.battery / 100.0f : 0.0f;
	if (*out_charge > 1.0f) {
		*out_charge = 1.0f;
	}
	return XRT_SUCCESS;
}

static void
pn2_ctrl_destroy(struct xrt_device *xdev)
{
	struct pn2_ctrl *d = pn2_ctrl(xdev);
	if (d->share_open) {
		ctrl_share_close(&d->share);
	}
	u_var_remove_root(d);
	free(d);
}

// simple_controller fallback: generic apps bind through this profile even
// though the device name advertises the richer pico_neo3 one
static struct xrt_binding_input_pair pn2_ctrl_simple_inputs[4] = {
    {XRT_INPUT_SIMPLE_SELECT_CLICK, XRT_INPUT_PICO_NEO3_TRIGGER_CLICK},
    {XRT_INPUT_SIMPLE_MENU_CLICK, XRT_INPUT_PICO_NEO3_MENU_CLICK},
    {XRT_INPUT_SIMPLE_GRIP_POSE, XRT_INPUT_PICO_NEO3_GRIP_POSE},
    {XRT_INPUT_SIMPLE_AIM_POSE, XRT_INPUT_PICO_NEO3_AIM_POSE},
};

static struct xrt_binding_profile pn2_ctrl_binding_profiles[1] = {
    {
        .name = XRT_DEVICE_SIMPLE_CONTROLLER,
        .inputs = pn2_ctrl_simple_inputs,
        .input_count = ARRAY_SIZE(pn2_ctrl_simple_inputs),
        .outputs = NULL,
        .output_count = 0,
    },
};

struct xrt_device *
pn2_ctrl_create(int which)
{
	struct pn2_ctrl *d = U_DEVICE_ALLOCATE(
	    struct pn2_ctrl, U_DEVICE_ALLOC_TRACKING_NONE, PN2C_COUNT, 0);
	if (d == NULL) {
		return NULL;
	}
	d->which = which;

	d->base.name = XRT_DEVICE_PICO_NEO3_CONTROLLER;
	d->base.device_type = which == CTRL_LEFT ? XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER
	                                         : XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER;
	d->base.supported.orientation_tracking = true;
	d->base.supported.position_tracking = true;

	d->base.update_inputs = pn2_ctrl_update_inputs;
	d->base.get_tracked_pose = pn2_ctrl_get_tracked_pose;
	d->base.get_battery_status = pn2_ctrl_get_battery_status;
	d->base.destroy = pn2_ctrl_destroy;

	d->base.inputs[PN2C_GRIP_POSE].name = XRT_INPUT_PICO_NEO3_GRIP_POSE;
	d->base.inputs[PN2C_AIM_POSE].name = XRT_INPUT_PICO_NEO3_AIM_POSE;
	d->base.inputs[PN2C_AB_CLICK].name =
	    which == CTRL_LEFT ? XRT_INPUT_PICO_NEO3_X_CLICK : XRT_INPUT_PICO_NEO3_A_CLICK;
	d->base.inputs[PN2C_BB_CLICK].name =
	    which == CTRL_LEFT ? XRT_INPUT_PICO_NEO3_Y_CLICK : XRT_INPUT_PICO_NEO3_B_CLICK;
	d->base.inputs[PN2C_MENU_CLICK].name = XRT_INPUT_PICO_NEO3_MENU_CLICK;
	d->base.inputs[PN2C_SYSTEM_CLICK].name = XRT_INPUT_PICO_NEO3_SYSTEM_CLICK;
	d->base.inputs[PN2C_TRIGGER_CLICK].name = XRT_INPUT_PICO_NEO3_TRIGGER_CLICK;
	d->base.inputs[PN2C_TRIGGER_VALUE].name = XRT_INPUT_PICO_NEO3_TRIGGER_VALUE;
	d->base.inputs[PN2C_STICK].name = XRT_INPUT_PICO_NEO3_THUMBSTICK;
	d->base.inputs[PN2C_STICK_CLICK].name = XRT_INPUT_PICO_NEO3_THUMBSTICK_CLICK;
	d->base.inputs[PN2C_SQUEEZE_CLICK].name = XRT_INPUT_PICO_NEO3_SQUEEZE_CLICK;
	d->base.inputs[PN2C_SQUEEZE_VALUE].name = XRT_INPUT_PICO_NEO3_SQUEEZE_VALUE;

	d->base.binding_profiles = pn2_ctrl_binding_profiles;
	d->base.binding_profile_count = ARRAY_SIZE(pn2_ctrl_binding_profiles);

	snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Pico Neo 2 Controller %s",
	         which == CTRL_LEFT ? "Left" : "Right");
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "PN2C%d", which);

	d->log_level = debug_get_log_option_pn2_ctrl_log();
	// first open attempt happens on the first update_inputs
	d->share_retry_ns = 0;

	u_var_add_root(d, d->base.str, true);
	u_var_add_log_level(d, &d->log_level, "log_level");

	return &d->base;
}
