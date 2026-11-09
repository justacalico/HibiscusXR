// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc controller: pose, buttons, stick, trigger and grip
 *         from one CTRL service stream. Multi-controller services hand
 *         each client its own stream id, single-controller services
 *         just get one connection each.
 * @ingroup drv_hsvc
 */

#include "hsvc_internal.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "math/m_relation_history.h"

#include "os/os_threading.h"
#include "os/os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEBUG_GET_ONCE_LOG_OPTION(hsvc_ctrl_log, "HSVC_LOG", U_LOGGING_WARN)

enum hsvcx_ctrl_in
{
	IN_AIM = 0,
	IN_GRIP,
	IN_SELECT,
	IN_MENU,
	IN_PRIMARY,
	IN_SECONDARY,
	IN_TRIG,
	IN_SQUEEZE,
	IN_STICK,
	IN_STICK_CLICK,
	IN_COUNT
};

static const enum xrt_input_name hsvcx_ctrl_names[IN_COUNT] = {
    [IN_AIM] = XRT_INPUT_GENERIC_AIM_POSE,
    [IN_GRIP] = XRT_INPUT_GENERIC_GRIP_POSE,
    [IN_SELECT] = XRT_INPUT_SIMPLE_SELECT_CLICK,
    [IN_MENU] = XRT_INPUT_SIMPLE_MENU_CLICK,
    [IN_PRIMARY] = XRT_INPUT_GENERIC_PRIMARY_CLICK,
    [IN_SECONDARY] = XRT_INPUT_GENERIC_SECONDARY_CLICK,
    [IN_TRIG] = XRT_INPUT_GENERIC_TRIGGER_VALUE,
    [IN_SQUEEZE] = XRT_INPUT_GENERIC_SQUEEZE_VALUE,
    [IN_STICK] = XRT_INPUT_GENERIC_THUMBSTICK,
    [IN_STICK_CLICK] = XRT_INPUT_GENERIC_THUMBSTICK_CLICK,
};

/*!
 * @implements xrt_device
 */
struct hsvcx_ctrl
{
	struct xrt_device base;
	struct os_thread_helper oth;
	struct os_mutex lock;

	struct hsvcx_conn conn;
	uint32_t stream;
	volatile bool stop;

	struct m_relation_history *rh;
	struct hsvc_ctrl last;
	bool have;
	uint64_t last_ts;

	enum u_logging_level log_level;
};

static inline struct hsvcx_ctrl *
hsvcx_ctrl(struct xrt_device *xdev)
{
	return (struct hsvcx_ctrl *)xdev;
}

#define CTR_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define CTR_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define CTR_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

static uint32_t
ctrl_pose_flags(uint32_t tracking)
{
	uint32_t flags = 0;
	if (tracking & 1u) {
		flags |= XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		         XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
	}
	if (tracking & 2u) {
		flags |= XRT_SPACE_RELATION_POSITION_VALID_BIT |
		         XRT_SPACE_RELATION_POSITION_TRACKED_BIT;
	}
	return flags;
}

static void
hsvcx_ctrl_push(struct hsvcx_ctrl *d, const struct hsvc_ctrl *c,
                uint64_t ts)
{
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;
	rel.pose.orientation.x = c->qx;
	rel.pose.orientation.y = c->qy;
	rel.pose.orientation.z = c->qz;
	rel.pose.orientation.w = c->qw;
	rel.pose.position.x = c->px;
	rel.pose.position.y = c->py;
	rel.pose.position.z = c->pz;
	rel.relation_flags =
	    (enum xrt_space_relation_flags)ctrl_pose_flags(c->tracking);
	m_relation_history_push(d->rh, &rel, (int64_t)ts);
	d->last = *c;
	d->last_ts = ts;
	d->have = true;
}

static void *
hsvcx_ctrl_run(void *ptr)
{
	struct hsvcx_ctrl *d = ptr;
	struct hsvc_ctrl c;
	uint32_t stream;
	uint64_t ts;
	while (!d->stop) {
		int n = hsvcx_next(&d->conn, &stream, &ts, &c, sizeof(c));
		if (n < 0)
			break;
		if (n < (int)sizeof(c) || stream != d->stream)
			continue;
		os_mutex_lock(&d->lock);
		hsvcx_ctrl_push(d, &c,
		                ts ? ts : (uint64_t)os_monotonic_get_ns());
		os_mutex_unlock(&d->lock);
		CTR_TRACE(d, "ctrl %u p=(%.3f %.3f %.3f) btn=%x", stream, c.px,
		          c.py, c.pz, c.buttons);
	}
	return NULL;
}

static xrt_result_t
hsvcx_ctrl_update_inputs(struct xrt_device *xdev)
{
	struct hsvcx_ctrl *d = hsvcx_ctrl(xdev);
	os_mutex_lock(&d->lock);
	struct hsvc_ctrl c = d->last;
	uint64_t ts = (int64_t)d->last_ts;
	os_mutex_unlock(&d->lock);
	if (!d->have)
		return XRT_SUCCESS;

	struct xrt_input *in = d->base.inputs;
	in[IN_TRIG].timestamp = ts;
	in[IN_TRIG].value.vec1.x = c.trigger;
	in[IN_SQUEEZE].timestamp = ts;
	in[IN_SQUEEZE].value.vec1.x = c.grip;
	in[IN_STICK].timestamp = ts;
	in[IN_STICK].value.vec2.x = c.stick_x;
	in[IN_STICK].value.vec2.y = c.stick_y;
	bool a = (c.buttons & HSVC_BTN_A) != 0;
	in[IN_SELECT].timestamp = ts;
	in[IN_SELECT].value.boolean = a;
	in[IN_PRIMARY].timestamp = ts;
	in[IN_PRIMARY].value.boolean = a;
	in[IN_SECONDARY].timestamp = ts;
	in[IN_SECONDARY].value.boolean = (c.buttons & HSVC_BTN_B) != 0;
	in[IN_MENU].timestamp = ts;
	in[IN_MENU].value.boolean = (c.buttons & HSVC_BTN_MENU) != 0;
	in[IN_STICK_CLICK].timestamp = ts;
	in[IN_STICK_CLICK].value.boolean =
	    (c.buttons & HSVC_BTN_STICK) != 0;
	return XRT_SUCCESS;
}

static xrt_result_t
hsvcx_ctrl_get_tracked_pose(struct xrt_device *xdev,
                            enum xrt_input_name name,
                            int64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	struct hsvcx_ctrl *d = hsvcx_ctrl(xdev);
	if (name != XRT_INPUT_GENERIC_AIM_POSE &&
	    name != XRT_INPUT_GENERIC_GRIP_POSE) {
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}
	os_mutex_lock(&d->lock);
	struct xrt_space_relation rel;
	if (d->have &&
	    m_relation_history_get(d->rh, at_timestamp_ns, &rel) !=
	        M_RELATION_HISTORY_RESULT_INVALID) {
		*out_relation = rel;
	} else if (d->have) {
		int64_t ts = 0;
		m_relation_history_get_latest(d->rh, &ts, out_relation);
	} else {
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		out_relation->pose.orientation.w = 1.0f;
		out_relation->relation_flags =
		    (enum xrt_space_relation_flags)0;
	}
	os_mutex_unlock(&d->lock);
	return XRT_SUCCESS;
}

static void
hsvcx_ctrl_destroy(struct xrt_device *xdev)
{
	struct hsvcx_ctrl *d = hsvcx_ctrl(xdev);
	d->stop = true;
	hsvcx_close(&d->conn);
	os_thread_helper_destroy(&d->oth);
	os_mutex_destroy(&d->lock);
	m_relation_history_destroy(&d->rh);
	free(d);
}

struct xrt_device *
hsvcx_ctrl_create(struct hsvcx_conn *conn, uint32_t stream, int index)
{
	if (hsvcx_sub(conn, stream) < 0)
		return NULL;

	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)U_DEVICE_ALLOC_TRACKING_NONE;
	struct hsvcx_ctrl *d =
	    U_DEVICE_ALLOCATE(struct hsvcx_ctrl, flags, IN_COUNT, 0);
	if (d == NULL)
		return NULL;

	d->base.name = XRT_DEVICE_SIMPLE_CONTROLLER;
	d->base.device_type = index == 0   ? XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER
	                      : index == 1 ? XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER
	                                   : XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER;
	d->base.supported.orientation_tracking = true;
	d->base.supported.position_tracking = true;
	snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Hibiscus controller");
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "hsvc-ctrl-%d",
	         index);

	u_device_populate_function_pointers(&d->base,
	                                    hsvcx_ctrl_get_tracked_pose,
	                                    hsvcx_ctrl_destroy);
	d->base.update_inputs = hsvcx_ctrl_update_inputs;
	for (int i = 0; i < IN_COUNT; i++) {
		d->base.inputs[i].name = hsvcx_ctrl_names[i];
		d->base.inputs[i].active = true;
	}

	d->log_level = debug_get_log_option_hsvc_ctrl_log();
	d->conn = *conn;
	d->stream = stream;
	conn->fd = -1;
	conn->desc = NULL;

	if (os_mutex_init(&d->lock) != 0) {
		CTR_ERROR(d, "mutex init failed");
		hsvcx_ctrl_destroy(&d->base);
		return NULL;
	}
	m_relation_history_create(&d->rh);

	if (os_thread_helper_init(&d->oth) != 0 ||
	    os_thread_helper_start(&d->oth, hsvcx_ctrl_run, d) != 0) {
		CTR_ERROR(d, "reader thread start failed");
		hsvcx_ctrl_destroy(&d->base);
		return NULL;
	}

	u_var_add_root(d, "HSVC ctrl", true);
	u_var_add_log_level(d, &d->log_level, "log_level");

	CTR_INFO(d, "hsvc ctrl up: stream %u", stream);
	return &d->base;
}
