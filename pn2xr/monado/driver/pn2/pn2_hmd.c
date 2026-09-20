// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 HMD driver: raw IMU (ASensorManager) -> m_imu_3dof fusion,
 *         qvrservice client -> fused 6DoF head pose.
 *
 * The device's virtual rotation-vector sensors emit identity quaternions on
 * this build, so we fuse the raw BMG160 gyroscope + BMA2x2 accelerometer
 * ourselves. When libqvrservice_client.so is reachable the driver also
 * connects to qvrd, which runs the tracking camera and produces a real
 * position-tracked pose; the IMU fusion stays as fallback.
 * @ingroup drv_pn2
 */

#include "pn2_interface.h"
#include "pn2_hmd.h"
#include "pn2_qvr.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "math/m_api.h"
#include "math/m_imu_3dof.h"
#include "math/m_relation_history.h"

#include "os/os_threading.h"

#include "xrt/xrt_device.h"

#include "android/android_globals.h"

#include <android/sensor.h>
#include <android/looper.h>

#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Workaround to avoid the inclusion of "android_native_app_glue.h".
#ifndef LOOPER_ID_USER
#define LOOPER_ID_USER 3
#endif


DEBUG_GET_ONCE_LOG_OPTION(pn2_log, "PN2_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_NUM_OPTION(pn2_axismap, "PN2_AXISMAP", 0)
DEBUG_GET_ONCE_FLOAT_OPTION(pn2_ipd, "PN2_IPD", 0.0635f)
DEBUG_GET_ONCE_BOOL_OPTION(pn2_no_qvr, "PN2_NO_QVR", false)

// Stock Pico lens field, same constants verified on-device in vrhome:
// the texture offset scales by K0 + K2 r^2 + K4 r^4 + K6 r^6 where r is the
// tan-angle radius of the panel pixel from the lens centre.
#define PN2_LENS_K0 0.740740741f
#define PN2_LENS_K2 0.192360375f
#define PN2_LENS_K4 -0.020400088f
#define PN2_LENS_K6 0.216338258f

/*!
 * @implements xrt_device
 */
struct pn2_device
{
	struct xrt_device base;
	struct os_thread_helper oth;

	struct
	{
		struct os_mutex lock;
		struct m_imu_3dof fusion;
		struct m_relation_history *rh;
		struct xrt_vec3 accel;
		bool has_accel;
	};

	enum u_logging_level log_level;
	struct xrt_quat sens_to_head; //!< maps raw sensor axes into head frame
	float lens_cx[2];             //!< lens centre uv in each half-panel
	float lens_cy;
	float tan_x;                  //!< panel-uv offset -> tan(angle) scales
	float tan_y;
	float chroma_r;
	float chroma_b;

	struct pn2_qvr *qvr;       //!< qvrservice 6DoF client, NULL when absent
	uint64_t qvr_last_ts;      //!< last pose timestamp pushed to history
	uint64_t qvr_dead_ns;      //!< when the pose stream went dead (0 = alive)
	uint64_t qvr_retry_at;     //!< next allowed client reconnect attempt

	// coast state while the tracker is degraded: the last real pose is the
	// anchor, the IMU fusion delta keeps the head turning until tracking
	// returns instead of freezing or snapping to origin
	bool coasting;
	struct xrt_quat coast_imu_base;
	struct xrt_quat coast_orient;
	struct xrt_vec3 coast_pos;
	bool coast_has_pos;
};

static inline struct pn2_device *
pn2_device(struct xrt_device *xdev)
{
	return (struct pn2_device *)xdev;
}

#define PN2_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define PN2_DEBUG(d, ...) U_LOG_XDEV_IFL_D(&d->base, d->log_level, __VA_ARGS__)
#define PN2_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define PN2_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

// Frame rebase from the qvrd tracking frame to the OpenXR view frame.
//
// Dumped live poses show qvrd reports the device pose in a world whose +X
// axis is up: swiveling the headset on a desk lands every delta on world X.
// The device local frame has the head top at +X, head left at +Y, and the
// face normal at -Z: nodding up/down lands on local Y, swiveling on X.
// OpenXR wants a +Y-up world and a -Z-forward / +Y-up view local frame, so
// poses are rebased on both sides: world by C (rotZ(+90)) and local by M.
static const struct xrt_quat PN2_QVR_WORLD_TO_VIEW = {
    .x = 0.0f, .y = 0.0f, .z = 0.7071068f, .w = 0.7071068f}; // rotZ(+90)
static const struct xrt_quat PN2_DEV_TO_VIEW = {
    .x = 0.0f, .y = 0.0f, .z = -0.7071068f, .w = 0.7071068f}; // rotZ(-90)

/*
 * Sensor axis maps.
 *
 * Raw dump on the device shows gravity dominated by +X when the unit lies
 * face-up, so sensor +X points out of the face. The other two sensor axes
 * were never dumped, so the maps are unverified: index selects among the
 * proper rotations for poking at with PN2_AXISMAP /
 * `setprop debug.pn2.axismap`.
 */
static const struct xrt_quat PN2_AXIS_MAPS[] = {
    // 0: cyclic  x_h=y_s, y_h=z_s, z_h=x_s  (120deg about (1,1,1))
    {.x = 0.5f, .y = 0.5f, .z = 0.5f, .w = 0.5f},
    // 1: x_h=z_s, y_h=-y_s, z_h=x_s
    {.x = 0.7071068f, .y = 0.0f, .z = 0.7071068f, .w = 0.0f},
    // 2: x_h=-z_s, y_h=y_s, z_h=x_s
    {.x = 0.0f, .y = 0.7071068f, .z = 0.7071068f, .w = 0.0f},
    // 3: x_h=-y_s, y_h=-z_s, z_h=x_s
    {.x = -0.5f, .y = -0.5f, .z = 0.5f, .w = 0.5f},
    // 4: identity (sensor frame already == head frame)
    {.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f},
    // 5: upstream android driver remap: x'=y, y'=-x, z'=z (rot -90 about z)
    {.x = 0.0f, .y = 0.0f, .z = -0.7071068f, .w = 0.7071068f},
};

static int
pn2_axis_map_index(void)
{
	const char *env = getenv("PN2_AXISMAP");
	if (env != NULL && env[0] != 0) {
		return atoi(env);
	}
	{
		char prop[92];
		prop[0] = 0;
		extern int __system_property_get(const char *, char *);
		if (__system_property_get("debug.pn2.axismap", prop) > 0) {
			return atoi(prop);
		}
	}
	return (int)debug_get_num_option_pn2_axismap();
}

static void
pn2_remap(const struct pn2_device *d, const struct xrt_vec3 *in, struct xrt_vec3 *out)
{
	math_quat_rotate_vec3(&d->sens_to_head, in, out);
}

static void
pn2_push_fused(struct pn2_device *d, uint64_t ts)
{
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;
	// the fusion already runs in a +Y-up world; only the local frame needs
	// the device -> view rebase
	math_quat_rotate(&d->fusion.rot, &PN2_DEV_TO_VIEW, &rel.pose.orientation);
	rel.relation_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT);
	// angular_velocity is world-frame per Monado convention; the gyro reads
	// body-frame, so rotate by the current pose
	math_quat_rotate_vec3(&d->fusion.rot, &d->fusion.last.gyro, &rel.angular_velocity);
	rel.relation_flags =
	    (enum xrt_space_relation_flags)(rel.relation_flags | XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT);
	m_relation_history_push_with_motion_estimation(d->rh, &rel, (int64_t)ts);
}

// returns false when the pose stream is dead - read failure, state 0, or a
// frozen buffer (the service keeps the last pose readable after it dies,
// so staleness is the only reliable liveness signal)
static bool
pn2_push_qvr(struct pn2_device *d)
{
	struct pn2_qvr_pose pose;
	if (!pn2_qvr_get_pose(d->qvr, &pose)) {
		return false;
	}
	// a dead service keeps the last pose readable at st=3 forever: repeated
	// raw timestamps (~2s worth) or an aged-out sample both mean it's gone
	if (pn2_qvr_stall(d->qvr) > 900 ||
	    (int64_t)(os_monotonic_get_ns() - pose.timestamp_ns) > 3000000000ll) {
		return false;
	}
	// Only fully-tracked poses enter the history: a state dip would push
	// orientation-without-position and snap the world to the head origin
	// for those frames. Skipping coasts the last tracked pose instead.
	if (pose.timestamp_ns <= d->qvr_last_ts || pose.tracking_state != 3) {
		return true;
	}
	d->qvr_last_ts = pose.timestamp_ns;

	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;
	struct xrt_quat tmp;
	math_quat_rotate(&PN2_QVR_WORLD_TO_VIEW, &pose.orientation, &tmp);
	math_quat_rotate(&tmp, &PN2_DEV_TO_VIEW, &rel.pose.orientation);
	math_quat_rotate_vec3(&PN2_QVR_WORLD_TO_VIEW, &pose.position, &rel.pose.position);
	// The service's velocity fields are noise on this build (dumped lv
	// swings ±2 m/s with the head bolted to a desk), so every display-time
	// prediction got a random positional kick. Leave them invalid and let
	// the history estimate motion by finite-differencing the poses.
	rel.relation_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	    XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT);
	m_relation_history_push_with_motion_estimation(d->rh, &rel, (int64_t)pose.timestamp_ns);
	PN2_TRACE(d, "qvr pos %.4f %.4f %.4f rot %.3f %.3f %.3f %.3f st %u", pose.position.x,
	          pose.position.y, pose.position.z, pose.orientation.x, pose.orientation.y,
	          pose.orientation.z, pose.orientation.w, pose.tracking_state);
	return true;
}

// polls qvr while alive; a dead stream drops the client after 3s, an
// up-but-empty one (tracker warm-up, state 0) after 15s - the fresh client
// re-requests positional mode, which is also how a wedged tracker gets
// kicked back to life
static void
pn2_pump_qvr(struct pn2_device *d)
{
	uint64_t now = os_monotonic_get_ns();
	if (d->qvr != NULL) {
		if (pn2_push_qvr(d)) {
			d->qvr_dead_ns = 0;
			return;
		}
		if (d->qvr_dead_ns == 0) {
			d->qvr_dead_ns = now;
			return;
		}
		uint64_t fuse = pn2_qvr_service_up() ? 15000000000ull : 3000000000ull;
		if (now - d->qvr_dead_ns < fuse) {
			return;
		}
		PN2_INFO(d, "qvr pose stream %s, reconnecting",
		         pn2_qvr_service_up() ? "idle" : "dead");
		pn2_qvr_destroy(d->qvr);
		d->qvr = NULL;
		d->qvr_last_ts = 0;
		d->qvr_dead_ns = 0;
		d->qvr_retry_at = now;
	}
	if (d->qvr_retry_at != 0 && now >= d->qvr_retry_at) {
		d->qvr = pn2_qvr_create();
		if (d->qvr != NULL) {
			PN2_INFO(d, "qvr client reconnected");
			d->qvr_retry_at = 0;
		} else {
			d->qvr_retry_at = now + 5000000000ull;
		}
	}
}

static void
pn2_handle_event(struct pn2_device *d, const ASensorEvent *event)
{
	struct xrt_vec3 v;
	os_mutex_lock(&d->lock);
	switch (event->type) {
	case ASENSOR_TYPE_ACCELEROMETER:
		v.x = event->acceleration.x;
		v.y = event->acceleration.y;
		v.z = event->acceleration.z;
		pn2_remap(d, &v, &d->accel);
		d->has_accel = true;
		break;
	case ASENSOR_TYPE_GYROSCOPE: {
		v.x = event->data[0];
		v.y = event->data[1];
		v.z = event->data[2];
		struct xrt_vec3 gyro;
		pn2_remap(d, &v, &gyro);
		struct xrt_vec3 accel = XRT_VEC3_ZERO;
		if (d->has_accel) {
			accel = d->accel;
		}
		m_imu_3dof_update(&d->fusion, (uint64_t)event->timestamp, &accel, &gyro);
		// QVR already fuses IMU + camera; mixing flag-less IMU relations into
		// the history would flicker position validity. The fusion keeps
		// running for the no-history fallback in get_tracked_pose.
		if (d->qvr == NULL) {
			pn2_push_fused(d, (uint64_t)event->timestamp);
		}
		PN2_TRACE(d, "gyro %.4f %.4f %.4f rot %.3f %.3f %.3f %.3f", gyro.x, gyro.y, gyro.z, d->fusion.rot.x,
		          d->fusion.rot.y, d->fusion.rot.z, d->fusion.rot.w);
		break;
	}
	default: break;
	}
	os_mutex_unlock(&d->lock);
}

static void *
pn2_run_thread(void *ptr)
{
	struct pn2_device *d = ptr;

	ASensorManager *sm = ASensorManager_getInstance();
	if (sm == NULL) {
		PN2_ERROR(d, "ASensorManager_getInstance failed");
		return NULL;
	}
	ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	ASensorEventQueue *queue = ASensorManager_createEventQueue(sm, looper, LOOPER_ID_USER, NULL, NULL);
	if (queue == NULL) {
		PN2_ERROR(d, "ASensorManager_createEventQueue failed");
		return NULL;
	}

	const ASensor *gyro = ASensorManager_getDefaultSensor(sm, ASENSOR_TYPE_GYROSCOPE);
	const ASensor *accel = ASensorManager_getDefaultSensor(sm, ASENSOR_TYPE_ACCELEROMETER);
	if (gyro == NULL || accel == NULL) {
		PN2_ERROR(d, "missing gyro(%p) or accel(%p)", (void *)gyro, (void *)accel);
	}

	if (accel != NULL) {
		ASensorEventQueue_enableSensor(queue, accel);
		int32_t min_delay = ASensor_getMinDelay(accel);
		ASensorEventQueue_setEventRate(queue, accel, min_delay > 0 ? min_delay : 10000);
	}
	if (gyro != NULL) {
		ASensorEventQueue_enableSensor(queue, gyro);
		int32_t min_delay = ASensor_getMinDelay(gyro);
		ASensorEventQueue_setEventRate(queue, gyro, min_delay > 0 ? min_delay : 2000);
	}

	// Watch the activity that created the instance: an app that gets closed
	// without calling xrDestroyInstance leaves this thread running in the
	// cached process, so the sensor subscription, the qvrd client and the
	// compositor stay alive and a relaunched app wedges behind the stale
	// state. Poll isFinishing here - the callback is main-thread state and
	// still fires while the app hangs inside its own destroy.
	JNIEnv *env = NULL;
	jobject activity = NULL;
	jmethodID is_finishing = NULL;
	jmethodID is_destroyed = NULL;
	struct _JavaVM *vm = android_globals_get_vm();
	jobject act = vm != NULL ? (jobject)android_globals_get_activity() : NULL;
	if (act != NULL &&
	    vm->functions->AttachCurrentThread(&vm->functions, &env, NULL) == JNI_OK &&
	    env != NULL) {
		activity = (*env)->NewGlobalRef(env, act);
		if (activity != NULL) {
			jclass cls = (*env)->GetObjectClass(env, activity);
			if (cls != NULL) {
				is_finishing = (*env)->GetMethodID(env, cls, "isFinishing", "()Z");
				is_destroyed = (*env)->GetMethodID(env, cls, "isDestroyed", "()Z");
				// context can be a service: neither method exists there
				if ((*env)->ExceptionCheck(env)) {
					(*env)->ExceptionClear(env);
				}
				(*env)->DeleteLocalRef(env, cls);
			}
		}
	}

	bool orphan = false;
	ASensorEvent events[32];
	uint64_t next_check = 0;
	while (os_thread_helper_is_running(&d->oth)) {
		ALooper_pollOnce(50, NULL, NULL, NULL);
		ssize_t n = ASensorEventQueue_getEvents(queue, events, 32);
		for (ssize_t i = 0; i < n; i++) {
			pn2_handle_event(d, &events[i]);
		}
		os_mutex_lock(&d->lock);
		pn2_pump_qvr(d);
		os_mutex_unlock(&d->lock);

		if (activity == NULL || is_finishing == NULL) {
			continue;
		}
		uint64_t now = os_monotonic_get_ns();
		if (now < next_check) {
			continue;
		}
		next_check = now + 100000000ull;
		jboolean gone = (*env)->CallBooleanMethod(env, activity, is_finishing);
		if (!gone && is_destroyed != NULL) {
			gone = (*env)->CallBooleanMethod(env, activity, is_destroyed);
		}
		if ((*env)->ExceptionCheck(env)) {
			(*env)->ExceptionClear(env);
			gone = false;
		}
		if (gone) {
			PN2_INFO(d, "activity finished with live instance, tearing down");
			orphan = true;
			break;
		}
	}

	if (accel != NULL) {
		ASensorEventQueue_disableSensor(queue, accel);
	}
	if (gyro != NULL) {
		ASensorEventQueue_disableSensor(queue, gyro);
	}
	ASensorManager_destroyEventQueue(sm, queue);

	if (env != NULL) {
		if (activity != NULL) {
			(*env)->DeleteGlobalRef(env, activity);
		}
		vm->functions->DetachCurrentThread(&vm->functions);
	}

	if (orphan) {
		// Nothing else will tear the runtime down now: kill the cached
		// process so the compositor, sessions, windows and service
		// connections die with the activity instead of leaking into the
		// next XR app.
		_exit(0);
	}
	return NULL;
}

static void
pn2_destroy(struct xrt_device *xdev)
{
	struct pn2_device *d = pn2_device(xdev);
	os_thread_helper_destroy(&d->oth);
	pn2_qvr_destroy(d->qvr);
	os_mutex_destroy(&d->lock);
	m_imu_3dof_close(&d->fusion);
	m_relation_history_destroy(&d->rh);
	u_var_remove_root(d);
	free(d);
}

static xrt_result_t
pn2_get_tracked_pose(struct xrt_device *xdev,
                     enum xrt_input_name name,
                     int64_t at_timestamp_ns,
                     struct xrt_space_relation *out_relation)
{
	struct pn2_device *d = pn2_device(xdev);
	struct xrt_space_relation rel = XRT_SPACE_RELATION_ZERO;

	os_mutex_lock(&d->lock);
	bool got = false;
	if (at_timestamp_ns > 0) {
		got = m_relation_history_get(d->rh, at_timestamp_ns, &rel) != M_RELATION_HISTORY_RESULT_INVALID;
	}
	if (!got) {
		// at_timestamp_ns == 0 means "latest": serve the newest history
		// entry, not the IMU fallback - that fusion frame disagrees with
		// the QVR world frame and snaps the view when hit
		int64_t latest_ts;
		got = m_relation_history_get_latest(d->rh, &latest_ts, &rel);
	}
	if (got) {
		// a stale newest entry means tracking is degraded or the stream
		// died: keep the last real pose as anchor and coast on the IMU
		// delta so the head still turns instead of freezing or snapping
		struct xrt_quat fusion_view;
		math_quat_rotate(&d->fusion.rot, &PN2_DEV_TO_VIEW, &fusion_view);
		int64_t latest_ts = 0;
		struct xrt_space_relation latest;
		if (m_relation_history_get_latest(d->rh, &latest_ts, &latest) &&
		    (int64_t)os_monotonic_get_ns() - latest_ts > 500000000ll) {
			if (!d->coasting) {
				d->coasting = true;
				d->coast_imu_base = fusion_view;
				d->coast_orient = latest.pose.orientation;
				d->coast_pos = latest.pose.position;
				d->coast_has_pos = (latest.relation_flags &
				                    XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0;
			}
			struct xrt_quat inv_base, delta;
			math_quat_invert(&d->coast_imu_base, &inv_base);
			math_quat_rotate(&inv_base, &fusion_view, &delta);
			math_quat_rotate(&d->coast_orient, &delta, &rel.pose.orientation);
			rel.pose.position = d->coast_pos;
			math_quat_rotate_vec3(&d->fusion.rot, &d->fusion.last.gyro, &rel.angular_velocity);
			rel.relation_flags = (enum xrt_space_relation_flags)(
			    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
			    XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT);
			if (d->coast_has_pos) {
				rel.relation_flags = (enum xrt_space_relation_flags)(
				    rel.relation_flags | XRT_SPACE_RELATION_POSITION_VALID_BIT);
			}
		} else {
			d->coasting = false;
		}
	}
	if (!got || !(rel.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT)) {
		struct xrt_space_relation zero_rel = XRT_SPACE_RELATION_ZERO;
		rel = zero_rel;
		math_quat_rotate(&d->fusion.rot, &PN2_DEV_TO_VIEW, &rel.pose.orientation);
		math_quat_rotate_vec3(&d->fusion.rot, &d->fusion.last.gyro, &rel.angular_velocity);
		rel.relation_flags = (enum xrt_space_relation_flags)(
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
		    XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT);
	}
	os_mutex_unlock(&d->lock);

	*out_relation = rel;
	return XRT_SUCCESS;
}

static xrt_result_t
pn2_compute_distortion(struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *result)
{
	struct pn2_device *d = pn2_device(xdev);

	// panel-pixel offset from this eye's lens centre, in half-panel uv
	const float cx = d->lens_cx[view];
	const float cy = d->lens_cy;
	float px = u - cx;
	float py = v - cy;
	float qx = px * d->tan_x;
	float qy = py * d->tan_y;
	float r2 = qx * qx + qy * qy;
	float scale = PN2_LENS_K0 + r2 * (PN2_LENS_K2 + r2 * (PN2_LENS_K4 + r2 * PN2_LENS_K6));

	float gx = cx + px * scale;
	float gy = cy + py * scale;
	result->g = (struct xrt_vec2){gx, gy};
	result->r = (struct xrt_vec2){cx + (gx - cx) * d->chroma_r, cy + (gy - cy) * d->chroma_r};
	result->b = (struct xrt_vec2){cx + (gx - cx) * d->chroma_b, cy + (gy - cy) * d->chroma_b};
	return XRT_SUCCESS;
}

static xrt_result_t
pn2_ref_space_usage(struct xrt_device *xdev,
                    enum xrt_reference_space_type type,
                    enum xrt_input_name name,
                    bool used)
{
	return XRT_SUCCESS;
}

struct xrt_device *
pn2_hmd_create(void)
{
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	struct pn2_device *d = U_DEVICE_ALLOCATE(struct pn2_device, flags, 1, 0);

	d->base.name = XRT_DEVICE_GENERIC_HMD;
	d->base.device_type = XRT_DEVICE_TYPE_HMD;
	d->base.supported.ref_space_usage = true;
	d->base.supported.orientation_tracking = true;

	if (debug_get_bool_option_pn2_no_qvr()) {
		d->qvr = NULL;
	} else {
		d->qvr = pn2_qvr_create();
	}
	// a failed connect leaves qvr_retry_at seeded so the pump keeps trying:
	// qvrd often isn't accepting clients yet at process start
	if (d->qvr == NULL && !debug_get_bool_option_pn2_no_qvr()) {
		d->qvr_retry_at = os_monotonic_get_ns() + 2000000000ull;
	}
	d->base.supported.position_tracking = !debug_get_bool_option_pn2_no_qvr();
	PN2_INFO(d, "qvrservice 6DoF: %s", d->qvr != NULL ? "connected" : "unavailable");
	u_device_populate_function_pointers(&d->base, pn2_get_tracked_pose, pn2_destroy);
	d->base.get_view_poses = u_device_get_view_poses;
	d->base.get_visibility_mask = u_device_get_visibility_mask;
	d->base.compute_distortion = pn2_compute_distortion;
	d->base.ref_space_usage = pn2_ref_space_usage;
	d->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Pico Neo 2");
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "PICOA7B10");

	d->log_level = debug_get_log_option_pn2_log();
	d->chroma_r = 0.992f;
	d->chroma_b = 1.012f;

	int map = pn2_axis_map_index();
	if (map < 0 || map >= (int)(sizeof(PN2_AXIS_MAPS) / sizeof(PN2_AXIS_MAPS[0]))) {
		map = 0;
	}
	d->sens_to_head = PN2_AXIS_MAPS[map];
	PN2_INFO(d, "sensor axis map %d", map);

	m_imu_3dof_init(&d->fusion, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);
	m_relation_history_create(&d->rh);
	os_thread_helper_init(&d->oth);

	if (os_mutex_init(&d->lock) != 0) {
		PN2_ERROR(d, "mutex init failed");
		pn2_destroy(&d->base);
		return NULL;
	}

	struct u_device_simple_info info;
	info.display.w_pixels = 3840;
	info.display.h_pixels = 2160;
	info.display.w_meters = 0.1191f;
	info.display.h_meters = 0.0670f;
	info.lens_horizontal_separation_meters = debug_get_float_option_pn2_ipd();
	info.lens_vertical_position_meters = info.display.h_meters / 2.0f;

	// Geometry matching the lens field above: the vertical panel edge sits
	// at tan = 1 (90 deg total), so the eye relief is half the panel height.
	// The horizontal total falls out of the per-eye half width and the lens
	// offset: 63.5mm IPD vs a 59.55mm half puts each lens centre ~2mm
	// outboard of its half's centre.
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
		PN2_ERROR(d, "u_device_setup_split_side_by_side failed");
		pn2_destroy(&d->base);
		return NULL;
	}
	d->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 72.0f);

	u_distortion_mesh_fill_in_compute(&d->base);

	if (os_thread_helper_start(&d->oth, pn2_run_thread, d) != 0) {
		PN2_ERROR(d, "sensor thread start failed");
		pn2_destroy(&d->base);
		return NULL;
	}

	u_var_add_root(d, "Pico Neo 2", true);
	u_var_add_log_level(d, &d->log_level, "log_level");
	u_var_add_ro_vec3_f32(d, &d->fusion.last.accel, "last.accel");
	u_var_add_ro_vec3_f32(d, &d->fusion.last.gyro, "last.gyro");
	m_imu_3dof_add_vars(&d->fusion, d, "fusion.");

	return &d->base;
}
