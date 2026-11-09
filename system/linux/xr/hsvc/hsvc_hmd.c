// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc HMD: streams head poses off the bus and serves them
 *         to monado. Display geometry comes from the service's INFO kv
 *         (see hsvc_geom.h), so the same driver follows whatever hmd
 *         service the device profile started.
 * @ingroup drv_hsvc
 */

#include "hsvc_internal.h"
#include "hsvc_geom.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "math/m_relation_history.h"

#include "os/os_threading.h"
#include "os/os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEBUG_GET_ONCE_LOG_OPTION(hsvc_log, "HSVC_LOG", U_LOGGING_WARN)

/*!
 * @implements xrt_device
 */
struct hsvcx_hmd
{
	struct xrt_device base;
	struct os_thread_helper oth;
	struct os_mutex lock;

	struct hsvcx_conn conn;
	volatile bool stop;

	struct m_relation_history *rh;
	struct hsvcx_panel panel;
	bool have_pose;
	bool connected;

	enum u_logging_level log_level;
};

static inline struct hsvcx_hmd *
hsvcx_hmd(struct xrt_device *xdev)
{
	return (struct hsvcx_hmd *)xdev;
}

#define HSVC_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define HSVC_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define HSVC_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

static uint32_t
pose_flags(uint32_t state)
{
	uint32_t flags = 0;
	if (state & 1u) {
		flags |= XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		         XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
	}
	if (state & 2u) {
		flags |= XRT_SPACE_RELATION_POSITION_VALID_BIT |
		         XRT_SPACE_RELATION_POSITION_TRACKED_BIT;
	}
	return flags;
}

static void
hsvcx_hmd_push(struct hsvcx_hmd *d, const struct hsvc_pose *p, uint64_t rx_ns)
{
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;
	rel.pose.orientation.x = p->qx;
	rel.pose.orientation.y = p->qy;
	rel.pose.orientation.z = p->qz;
	rel.pose.orientation.w = p->qw;
	rel.pose.position.x = p->px;
	rel.pose.position.y = p->py;
	rel.pose.position.z = p->pz;
	rel.relation_flags = (enum xrt_space_relation_flags)pose_flags(p->state);
	m_relation_history_push(d->rh, &rel, (int64_t)rx_ns);
	d->have_pose = true;
}

static void *
hsvcx_hmd_run(void *ptr)
{
	struct hsvcx_hmd *d = ptr;
	struct hsvc_pose p;
	uint32_t stream;
	uint64_t ts;
	while (!d->stop) {
		int n = hsvcx_next(&d->conn, &stream, &ts, &p, sizeof(p));
		if (n < 0)
			break;
		if (n < (int)sizeof(p))
			continue;
		os_mutex_lock(&d->lock);
		hsvcx_hmd_push(d, &p, ts ? ts : (uint64_t)os_monotonic_get_ns());
		d->connected = true;
		os_mutex_unlock(&d->lock);
		HSVC_TRACE(d, "pose q=(%.3f %.3f %.3f %.3f) p=(%.3f %.3f %.3f)",
		           p.qx, p.qy, p.qz, p.qw, p.px, p.py, p.pz);
	}
	os_mutex_lock(&d->lock);
	d->connected = false;
	os_mutex_unlock(&d->lock);
	return NULL;
}

static void
hsvcx_hmd_destroy(struct xrt_device *xdev)
{
	struct hsvcx_hmd *d = hsvcx_hmd(xdev);
	d->stop = true;
	hsvcx_close(&d->conn);
	os_thread_helper_destroy(&d->oth);
	os_mutex_destroy(&d->lock);
	m_relation_history_destroy(&d->rh);
	free(d);
}

static xrt_result_t
hsvcx_hmd_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           int64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
	struct hsvcx_hmd *d = hsvcx_hmd(xdev);
	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}
	os_mutex_lock(&d->lock);
	struct xrt_space_relation rel;
	if (d->have_pose &&
	    m_relation_history_get(d->rh, at_timestamp_ns, &rel) !=
	        M_RELATION_HISTORY_RESULT_INVALID) {
		*out_relation = rel;
	} else if (d->have_pose) {
		int64_t ts = 0;
		m_relation_history_get_latest(d->rh, &ts, out_relation);
	} else {
		/* service up but no sample yet: hold a steady upright pose so
		 * apps get a tracked head instead of erroring on boot */
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		out_relation->pose.orientation.w = 1.0f;
		out_relation->relation_flags = (enum xrt_space_relation_flags)(
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT);
	}
	os_mutex_unlock(&d->lock);
	return XRT_SUCCESS;
}

static xrt_result_t
hsvcx_hmd_compute_distortion(struct xrt_device *xdev, uint32_t view,
                             float u, float v,
                             struct xrt_uv_triplet *result)
{
	struct hsvcx_hmd *d = hsvcx_hmd(xdev);
	float gx, gy, rx, ry, bx, by;
	hsvcx_distort(&d->panel, (int)view, u, v, &gx, &gy, &rx, &ry, &bx, &by);
	result->g = (struct xrt_vec2){gx, gy};
	result->r = (struct xrt_vec2){rx, ry};
	result->b = (struct xrt_vec2){bx, by};
	return XRT_SUCCESS;
}

struct xrt_device *
hsvcx_hmd_create(struct hsvcx_conn *conn)
{
	struct hsvcx_panel panel;
	if (hsvcx_panel_parse(conn->desc, &panel) < 0) {
		/* no panel keys: still usable for tracking-only bring-up, but
		 * refuse so the compositor never sees a 0x0 hmd */
		return NULL;
	}
	if (hsvcx_sub(conn, 0) < 0)
		return NULL;

	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(
	    U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	struct hsvcx_hmd *d = U_DEVICE_ALLOCATE(struct hsvcx_hmd, flags, 1, 0);
	if (d == NULL)
		return NULL;

	d->base.name = XRT_DEVICE_GENERIC_HMD;
	d->base.device_type = XRT_DEVICE_TYPE_HMD;
	d->base.supported.orientation_tracking = true;
	d->base.supported.position_tracking = true;
	snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Hibiscus HMD");
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "hsvc");

	u_device_populate_function_pointers(&d->base,
	                                    hsvcx_hmd_get_tracked_pose,
	                                    hsvcx_hmd_destroy);
	d->base.get_view_poses = u_device_get_view_poses;
	d->base.get_visibility_mask = u_device_get_visibility_mask;
	d->base.compute_distortion = hsvcx_hmd_compute_distortion;
	d->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	d->base.inputs[0].active = true;

	d->log_level = debug_get_log_option_hsvc_log();
	d->panel = panel;
	d->conn = *conn;
	conn->fd = -1;
	conn->desc = NULL;

	if (os_mutex_init(&d->lock) != 0) {
		HSVC_ERROR(d, "mutex init failed");
		hsvcx_hmd_destroy(&d->base);
		return NULL;
	}
	m_relation_history_create(&d->rh);

	struct u_device_simple_info info;
	info.display.w_pixels = panel.w_px;
	info.display.h_pixels = panel.h_px;
	info.display.w_meters = panel.w_m;
	info.display.h_meters = panel.h_m;
	info.lens_horizontal_separation_meters = panel.lens_sep_m;
	info.lens_vertical_position_meters = panel.h_m / 2.0f;
	info.fov[0] = panel.fov[0];
	info.fov[1] = panel.fov[1];
	if (!u_device_setup_split_side_by_side(&d->base, &info)) {
		HSVC_ERROR(d, "u_device_setup_split_side_by_side failed");
		hsvcx_hmd_destroy(&d->base);
		return NULL;
	}
	if (panel.frame_ns)
		d->base.hmd->screens[0].nominal_frame_interval_ns =
		    panel.frame_ns;

	u_distortion_mesh_fill_in_compute(&d->base);

	if (os_thread_helper_init(&d->oth) != 0 ||
	    os_thread_helper_start(&d->oth, hsvcx_hmd_run, d) != 0) {
		HSVC_ERROR(d, "reader thread start failed");
		hsvcx_hmd_destroy(&d->base);
		return NULL;
	}

	u_var_add_root(d, "HSVC HMD", true);
	u_var_add_log_level(d, &d->log_level, "log_level");

	HSVC_INFO(d, "hsvc hmd up: %ux%u", panel.w_px, panel.h_px);
	return &d->base;
}
