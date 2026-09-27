// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  VMD virtual headset driver: a vmd host process on the PC streams
 *         head poses over TCP and this driver serves them to monado.
 *
 * It exists so the identical system image runs under qemu with working
 * tracking - no guest-side tweaks, the driver only activates when the
 * host channel answers.
 * @ingroup drv_vmd
 */

#include "hsvr_kit.h"
#include "vmd_chan.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "math/m_api.h"
#include "math/m_relation_history.h"

#include "os/os_threading.h"
#include "os/os_time.h"

#include "xrt/xrt_device.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


DEBUG_GET_ONCE_LOG_OPTION(vmd_log, "VMD_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_OPTION(vmd_addr, "VMD_ADDR", NULL)

// host the guest reaches through qemu user networking; overridable with
// VMD_ADDR="host:port" for non-qemu setups
static const char *
vmd_addr(void)
{
	const char *env = debug_get_option_vmd_addr();
	if (env != NULL && env[0] != '\0') {
		return env;
	}
	return NULL; /* chan defaults */
}

/*!
 * @implements xrt_device
 */
struct vmd_device
{
	struct xrt_device base;
	struct os_thread_helper oth;
	struct os_mutex lock;

	struct vmd_chan *chan;
	volatile bool stop;

	struct m_relation_history *rh;
	struct vmd_pose_packet last;
	bool have_pose;
	uint32_t last_seq;

	float lens_cx[2];
	float lens_cy;
	float tan_x;
	float tan_y;
	float chroma_r;
	float chroma_b;

	enum u_logging_level log_level;
};

static inline struct vmd_device *
vmd_device(struct xrt_device *xdev)
{
	return (struct vmd_device *)xdev;
}

#define VMD_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define VMD_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define VMD_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

static void
vmd_push(struct vmd_device *d, const struct vmd_pose_packet *pkt, uint64_t rx_ns)
{
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;
	rel.pose.orientation.x = pkt->quat[0];
	rel.pose.orientation.y = pkt->quat[1];
	rel.pose.orientation.z = pkt->quat[2];
	rel.pose.orientation.w = pkt->quat[3];
	rel.pose.position.x = pkt->pos[0];
	rel.pose.position.y = pkt->pos[1];
	rel.pose.position.z = pkt->pos[2];
	rel.linear_velocity.x = pkt->linvel[0];
	rel.linear_velocity.y = pkt->linvel[1];
	rel.linear_velocity.z = pkt->linvel[2];
	rel.angular_velocity.x = pkt->angvel[0];
	rel.angular_velocity.y = pkt->angvel[1];
	rel.angular_velocity.z = pkt->angvel[2];
	rel.relation_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	    XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
	    XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT);
	if (pkt->flags & VMD_POSE_POSITION_VALID) {
		rel.relation_flags = (enum xrt_space_relation_flags)(
		    rel.relation_flags | XRT_SPACE_RELATION_POSITION_VALID_BIT |
		    XRT_SPACE_RELATION_POSITION_TRACKED_BIT);
	}
	m_relation_history_push(d->rh, &rel, (int64_t)rx_ns);
	d->last = *pkt;
	d->have_pose = true;
}

static void *
vmd_run_thread(void *ptr)
{
	struct vmd_device *d = ptr;
	struct vmd_pose_packet pkt;
	while (!d->stop) {
		if (!vmd_chan_read(d->chan, &pkt, &d->stop)) {
			break;
		}
		uint64_t now = os_monotonic_get_ns();
		os_mutex_lock(&d->lock);
		if (pkt.seq != d->last_seq) {
			vmd_push(d, &pkt, now);
			d->last_seq = pkt.seq;
		}
		os_mutex_unlock(&d->lock);
		VMD_TRACE(d, "pose seq=%u q=(%.3f %.3f %.3f %.3f) p=(%.3f %.3f %.3f)", pkt.seq,
		          pkt.quat[0], pkt.quat[1], pkt.quat[2], pkt.quat[3], pkt.pos[0],
		          pkt.pos[1], pkt.pos[2]);
	}
	return NULL;
}

static void
vmd_destroy(struct xrt_device *xdev)
{
	struct vmd_device *d = vmd_device(xdev);
	d->stop = true;
	os_thread_helper_destroy(&d->oth);
	vmd_chan_close(d->chan);
	os_mutex_destroy(&d->lock);
	m_relation_history_destroy(&d->rh);
	free(d);
}

static xrt_result_t
vmd_get_tracked_pose(struct xrt_device *xdev,
                     enum xrt_input_name name,
                     int64_t at_timestamp_ns,
                     struct xrt_space_relation *out_relation)
{
	struct vmd_device *d = vmd_device(xdev);
	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}
	os_mutex_lock(&d->lock);
	struct xrt_space_relation rel;
	if (d->have_pose &&
	    m_relation_history_get(d->rh, at_timestamp_ns, &rel) != M_RELATION_HISTORY_RESULT_INVALID) {
		*out_relation = rel;
	} else if (d->have_pose) {
		// extrapolation past the newest sample came back invalid; serve
		// the latest rather than an error
		int64_t ts = 0;
		m_relation_history_get_latest(d->rh, &ts, out_relation);
	} else {
		// host connected but no packet yet: hold a steady upright pose so
		// apps get a tracked head instead of erroring during VM boot
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		out_relation->pose.orientation.w = 1.0f;
		out_relation->relation_flags = (enum xrt_space_relation_flags)(
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT);
	}
	os_mutex_unlock(&d->lock);
	return XRT_SUCCESS;
}

// Same panel + lens field as drivers/pn2: the VM presents the identical
// display hardware, so the distortion math must match what the dash and
// xrtest were tuned against.
#define VMD_LENS_K0 0.740740741f
#define VMD_LENS_K2 0.192360375f
#define VMD_LENS_K4 -0.020400088f
#define VMD_LENS_K6 0.216338258f

static xrt_result_t
vmd_compute_distortion(struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *result)
{
	struct vmd_device *d = vmd_device(xdev);
	const float cx = d->lens_cx[view];
	const float cy = d->lens_cy;
	const float px = (u - cx) * d->tan_x;
	const float py = (v - cy) * d->tan_y;
	const float r2 = px * px + py * py;
	const float scale = VMD_LENS_K0 + VMD_LENS_K2 * r2 + VMD_LENS_K4 * r2 * r2 + VMD_LENS_K6 * r2 * r2 * r2;
	const float gx = cx + (px / d->tan_x) * scale;
	const float gy = cy + (py / d->tan_y) * scale;
	result->g = (struct xrt_vec2){gx, gy};
	result->r = (struct xrt_vec2){cx + (gx - cx) * d->chroma_r, cy + (gy - cy) * d->chroma_r};
	result->b = (struct xrt_vec2){cx + (gx - cx) * d->chroma_b, cy + (gy - cy) * d->chroma_b};
	return XRT_SUCCESS;
}

static struct xrt_device *
vmd_hmd_create(void)
{
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	struct vmd_device *d = U_DEVICE_ALLOCATE(struct vmd_device, flags, 1, 0);
	if (d == NULL) {
		return NULL;
	}

	d->base.name = XRT_DEVICE_GENERIC_HMD;
	d->base.device_type = XRT_DEVICE_TYPE_HMD;
	d->base.supported.orientation_tracking = true;
	d->base.supported.position_tracking = true;
	snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "VMD");
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "vmd-qemu");

	u_device_populate_function_pointers(&d->base, vmd_get_tracked_pose, vmd_destroy);
	d->base.get_view_poses = u_device_get_view_poses;
	d->base.get_visibility_mask = u_device_get_visibility_mask;
	d->base.compute_distortion = vmd_compute_distortion;
	d->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	d->base.inputs[0].active = true;

	d->log_level = debug_get_log_option_vmd_log();
	d->chroma_r = 0.992f;
	d->chroma_b = 1.012f;

	if (os_mutex_init(&d->lock) != 0) {
		VMD_ERROR(d, "mutex init failed");
		vmd_destroy(&d->base);
		return NULL;
	}
	m_relation_history_create(&d->rh);

	struct u_device_simple_info info;
	info.display.w_pixels = 3840;
	info.display.h_pixels = 2160;
	info.display.w_meters = 0.1191f;
	info.display.h_meters = 0.0670f;
	info.lens_horizontal_separation_meters = 0.0635f;
	info.lens_vertical_position_meters = info.display.h_meters / 2.0f;

	const float half_w_m = info.display.w_meters / 2.0f;
	const float eye_relief = info.display.h_meters / 2.0f;
	const float lens_cx_m = info.lens_horizontal_separation_meters / 2.0f;
	info.fov[0] = info.fov[1] =
	    atanf(lens_cx_m / eye_relief) + atanf((half_w_m - lens_cx_m) / eye_relief);
	d->lens_cx[0] = (half_w_m - lens_cx_m) / half_w_m;
	d->lens_cx[1] = lens_cx_m / half_w_m;
	d->lens_cy = 0.5f;
	d->tan_x = half_w_m / eye_relief;
	d->tan_y = info.display.h_meters / eye_relief;
	if (!u_device_setup_split_side_by_side(&d->base, &info)) {
		VMD_ERROR(d, "u_device_setup_split_side_by_side failed");
		vmd_destroy(&d->base);
		return NULL;
	}
	d->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 72.0f);

	u_distortion_mesh_fill_in_compute(&d->base);

	d->chan = vmd_chan_open(vmd_addr());
	if (d->chan == NULL) {
		VMD_ERROR(d, "channel alloc failed");
		vmd_destroy(&d->base);
		return NULL;
	}
	if (os_thread_helper_start(&d->oth, vmd_run_thread, d) != 0) {
		VMD_ERROR(d, "reader thread start failed");
		vmd_destroy(&d->base);
		return NULL;
	}

	u_var_add_root(d, "VMD", true);
	u_var_add_log_level(d, &d->log_level, "log_level");

	VMD_INFO(d, "vmd hmd up, waiting for host poses");
	return &d->base;
}

static int
vmd_probe(void)
{
	if (getenv("VMD_FORCE") != NULL) {
		return 300;
	}
	// the channel answering means a vmd host is attached: this outscores
	// hardware probes since the identical image also reports real-device
	// sysprops inside the VM
	return vmd_chan_probe(vmd_addr()) ? 200 : 0;
}

const struct hsvr_driver hsvr_drv_vmd = {
    .name = "vmd",
    .probe = vmd_probe,
    .create_hmd = vmd_hmd_create,
    .create_controller = NULL,
};
