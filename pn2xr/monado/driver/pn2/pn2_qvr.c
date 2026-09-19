// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  qvrservice 6DoF head tracking client for the Pico Neo 2 driver.
 *
 * Loads the stock libqvrservice_client.so at runtime and talks to qvrd
 * through its C++ API. With positional tracking mode + VR mode active,
 * qvrd runs the SLAM camera and the tracker writes fused head poses into
 * a shared-memory ring buffer; GetHeadTrackingData hands us the latest.
 *
 * ABI notes (recovered from qvrservicetest64 on the device):
 *  - QVRServiceClient is an 8-byte handle whose first field points at the
 *    heap-allocated QVRServiceClientImpl (4072 bytes).
 *  - qvrservice_head_tracking_data_t layout below was verified against
 *    live poses; quality/state fields sit at fixed offsets.
 *  - Pose timestamps come from the QVR clock domain, which runs a few
 *    seconds ahead of CLOCK_MONOTONIC. We learn the offset once and
 *    convert.
 * @ingroup drv_pn2
 */

#include "pn2_qvr.h"

#include "util/u_logging.h"

#include "os/os_time.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>

// QVRSERVICE_TRACKING_MODE_POSITIONAL: verified on device, modes 1/2/4
// are accepted and 2 switches the head pose to a fused camera+IMU pose.
#define PN2_QVR_TRACKING_MODE_POSITIONAL 2

#define PN2_QVR_TRACKING_STATE_TRACKING 3

/*!
 * qvrservice_head_tracking_data_t as the service writes it into the
 * ring buffer. Offsets recovered from qvrservicetest64 and confirmed by
 * dumping live data: tracking_state reads 3 and pose_quality 1.0 at the
 * expected offsets while the pose is tracking.
 */
struct pn2_qvr_head_data
{
	float quat[4];            //!< x, y, z, w
	float position[3];
	uint32_t pad0;
	uint64_t timestamp_ns;    //!< QVR clock domain
	uint32_t pad1[2];
	float angular_velocity[3];
	uint32_t pad2;
	float linear_velocity[3];
	uint32_t pad3;
	float reserved0[3];
	uint32_t pad4;
	float reserved1[3];
	uint32_t tracking_state;
	uint32_t warning_flags;
	float pose_quality;
	float sensor_quality;
	float camera_quality;
};
static_assert(sizeof(struct pn2_qvr_head_data) == 128, "head data layout");

typedef void (*pn2_qvr_ctor_t)(void *self);
typedef void (*pn2_qvr_dtor_t)(void *self);
typedef int (*pn2_qvr_m0_t)(void *self);
typedef int (*pn2_qvr_mode_t)(void *self, int mode);
typedef int (*pn2_qvr_head_t)(void *self, struct pn2_qvr_head_data **out);

struct pn2_qvr
{
	void *lib;
	char client[16]; //!< QVRServiceClient object storage
	void *impl;
	pn2_qvr_dtor_t dtor;
	pn2_qvr_m0_t stop_vr_mode;
	pn2_qvr_head_t get_head_data;
	int64_t clock_offset; //!< qvr_ts - monotonic_ts, learned from first pose
	bool clock_offset_set;
	uint64_t last_raw_ts; //!< raw QVR timestamp of the newest read
	int stall;            //!< consecutive reads with last_raw_ts repeated
};

static void *
pn2_qvr_sym(struct pn2_qvr *q, const char *name)
{
	void *fn = dlsym(q->lib, name);
	if (fn == NULL) {
		U_LOG_E("pn2_qvr: missing symbol %s", name);
	}
	return fn;
}

bool
pn2_qvr_service_up(void)
{
	char v[PROP_VALUE_MAX] = {0};
	__system_property_get("init.svc.pn2_qvrd", v);
	// unset on setups that name the service differently: don't gate those
	return v[0] == '\0' || strcmp(v, "running") == 0;
}

struct pn2_qvr *
pn2_qvr_create(void)
{
	if (!pn2_qvr_service_up()) {
		U_LOG_W("pn2_qvr: qvrd not running, not connecting");
		return NULL;
	}
	struct pn2_qvr *q = calloc(1, sizeof(*q));

	// Soname first (app namespaces resolve it through public.libraries.txt),
	// then the absolute paths for shell/testing contexts, then the runtime
	// staging dir the image sets up under /data.
	q->lib = dlopen("libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	if (q->lib == NULL) {
		q->lib = dlopen("/system/lib64/libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	}
	if (q->lib == NULL) {
		q->lib = dlopen("/vendor/lib64/libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	}
	if (q->lib == NULL) {
		// libdrm.so is a non-public dependency; preload it from staging so
		// the client's NEEDED entry resolves against the loaded list.
		dlopen("/data/local/tmp/xr/lib/arm64/libdrm.so", RTLD_NOW | RTLD_LOCAL);
		q->lib =
		    dlopen("/data/local/tmp/xr/lib/arm64/libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	}
	if (q->lib == NULL) {
		U_LOG_W("pn2_qvr: libqvrservice_client.so unavailable: %s", dlerror());
		free(q);
		return NULL;
	}

	pn2_qvr_ctor_t ctor = (pn2_qvr_ctor_t)pn2_qvr_sym(q, "_ZN16QVRServiceClientC1Ev");
	q->dtor = (pn2_qvr_dtor_t)pn2_qvr_sym(q, "_ZN16QVRServiceClientD1Ev");
	pn2_qvr_m0_t start_vr_mode = (pn2_qvr_m0_t)pn2_qvr_sym(q, "_ZN16QVRServiceClient11StartVRModeEv");
	q->stop_vr_mode = (pn2_qvr_m0_t)pn2_qvr_sym(q, "_ZN16QVRServiceClient10StopVRModeEv");
	pn2_qvr_mode_t set_mode =
	    (pn2_qvr_mode_t)pn2_qvr_sym(q, "_ZN16QVRServiceClient15SetTrackingModeE24QVRSERVICE_TRACKING_MODE");
	q->get_head_data = (pn2_qvr_head_t)pn2_qvr_sym(
	    q, "_ZN20QVRServiceClientImpl19GetHeadTrackingDataEPP31qvrservice_head_tracking_data_t");

	if (ctor == NULL || q->dtor == NULL || start_vr_mode == NULL || q->stop_vr_mode == NULL ||
	    set_mode == NULL || q->get_head_data == NULL) {
		U_LOG_E("pn2_qvr: symbol resolution failed");
		dlclose(q->lib);
		free(q);
		return NULL;
	}

	ctor(q->client);
	q->impl = *(void **)q->client;
	if (q->impl == NULL) {
		U_LOG_W("pn2_qvr: client impl is null");
		q->dtor(q->client);
		dlclose(q->lib);
		free(q);
		return NULL;
	}

	if (set_mode(q->client, PN2_QVR_TRACKING_MODE_POSITIONAL) != 0) {
		U_LOG_W("pn2_qvr: positional tracking mode rejected");
	}
	// VR mode is service-global: a second client gets an error here but the
	// pose ring buffer is already running, so treat failure as non-fatal and
	// let get_pose prove whether data flows.
	if (start_vr_mode(q->client) != 0) {
		U_LOG_W("pn2_qvr: StartVRMode rejected, reading poses anyway");
	}

	U_LOG_I("pn2_qvr: client connected");
	return q;
}

bool
pn2_qvr_get_pose(struct pn2_qvr *q, struct pn2_qvr_pose *out)
{
	// GetHeadTrackingData dereferences the dead service's binder state and
	// segfaults: never call it while qvrd is down
	if (!pn2_qvr_service_up()) {
		return false;
	}
	struct pn2_qvr_head_data *d = NULL;
	if (q->get_head_data(q->impl, &d) < 0 || d == NULL || d->tracking_state == 0) {
		// state 0 means this client never got VR mode (or the tracker is
		// dead): refuse the pose so the caller falls back to live IMU data
		// instead of a frozen identity quaternion
		return false;
	}

	// a frozen buffer repeats its timestamp: a dead service keeps the last
	// pose readable (state and all), so repeats are the only liveness check.
	// resync only on a fresh sample - syncing to a frozen one would pin the
	// converted timestamp at "now" forever and hide the stall
	// ts==0 is warm-up (tracker alive, no pose written yet), not a stall
	q->stall = (d->timestamp_ns != 0 && d->timestamp_ns == q->last_raw_ts)
	               ? q->stall + 1 : 0;
	q->last_raw_ts = d->timestamp_ns;

	uint64_t now = os_monotonic_get_ns();
	int64_t sample_off = (int64_t)d->timestamp_ns - (int64_t)now;
	if (!q->clock_offset_set) {
		q->clock_offset = sample_off;
		q->clock_offset_set = true;
	} else if (q->stall == 0 &&
	           (sample_off - q->clock_offset > 50000000 ||
	            sample_off - q->clock_offset < -50000000)) {
		// suspend cycles shift the service clock against monotonic; a stale
		// offset lands every converted timestamp in the past/future and
		// breaks prediction, so resync when drift exceeds 50ms
		q->clock_offset = sample_off;
	}

	out->orientation.x = d->quat[0];
	out->orientation.y = d->quat[1];
	out->orientation.z = d->quat[2];
	out->orientation.w = d->quat[3];
	out->position.x = d->position[0];
	out->position.y = d->position[1];
	out->position.z = d->position[2];
	out->angular_velocity.x = d->angular_velocity[0];
	out->angular_velocity.y = d->angular_velocity[1];
	out->angular_velocity.z = d->angular_velocity[2];
	out->linear_velocity.x = d->linear_velocity[0];
	out->linear_velocity.y = d->linear_velocity[1];
	out->linear_velocity.z = d->linear_velocity[2];
	out->timestamp_ns = (uint64_t)((int64_t)d->timestamp_ns - q->clock_offset);
	out->tracking_state = d->tracking_state;
	out->warning_flags = d->warning_flags;
	out->pose_quality = d->pose_quality;
	out->sensor_quality = d->sensor_quality;
	out->camera_quality = d->camera_quality;
	return true;
}

int
pn2_qvr_stall(struct pn2_qvr *q)
{
	return q->stall;
}

void
pn2_qvr_destroy(struct pn2_qvr *q)
{
	if (q == NULL) {
		return;
	}
	if (q->lib != NULL) {
		if (q->stop_vr_mode != NULL) {
			q->stop_vr_mode(q->client);
		}
		if (q->dtor != NULL) {
			q->dtor(q->client);
		}
		dlclose(q->lib);
	}
	free(q);
}
