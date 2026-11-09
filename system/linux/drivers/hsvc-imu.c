// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc-imu - generic IMU service over Linux IIO.
 *         --iio N reads /sys/bus/iio/devices/iio:deviceN channels;
 *         --file replays "gx gy gz ax ay az" raw-int lines; --sim.
 *         Publishes kind=imu samples in SI units.
 */
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "hsvc/hsvc.h"
#include "imuscl.h"

struct imu
{
	char iio[256]; /* iio device dir, empty when not used */
	struct hsvc_imu_scale scale;
	FILE *replay;
	int sim;
	uint64_t seq;
};

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int
sysfs_f(const char *dir, const char *file, float *out)
{
	char p[512];
	snprintf(p, sizeof(p), "%s/%s", dir, file);
	FILE *f = fopen(p, "r");
	if (!f)
		return -1;
	int rc = fscanf(f, "%f", out) == 1 ? 0 : -1;
	fclose(f);
	return rc;
}

static int
sysfs_i(const char *dir, const char *file, int32_t *out)
{
	float v;
	if (sysfs_f(dir, file, &v) < 0)
		return -1;
	*out = (int32_t)v;
	return 0;
}

static int
iio_load_scales(struct imu *m)
{
	float g, a;
	if (sysfs_f(m->iio, "in_anglvel_scale", &g) < 0)
		g = 1.0f;
	if (sysfs_f(m->iio, "in_accel_scale", &a) < 0)
		a = 1.0f;
	m->scale.gyro = hsvc_iio_gyro_scale(g);
	m->scale.accel = hsvc_iio_accel_scale(a);
	return 0;
}

static int
iio_read(struct imu *m, struct hsvc_imu *out)
{
	int32_t gx, gy, gz, ax, ay, az;
	if (sysfs_i(m->iio, "in_anglvel_x_raw", &gx) < 0 ||
	    sysfs_i(m->iio, "in_anglvel_y_raw", &gy) < 0 ||
	    sysfs_i(m->iio, "in_anglvel_z_raw", &gz) < 0 ||
	    sysfs_i(m->iio, "in_accel_x_raw", &ax) < 0 ||
	    sysfs_i(m->iio, "in_accel_y_raw", &ay) < 0 ||
	    sysfs_i(m->iio, "in_accel_z_raw", &az) < 0)
		return -1;
	hsvc_imu_convert(gx, gy, gz, ax, ay, az, &m->scale, &out->gx, &out->gy,
	                 &out->gz, &out->ax, &out->ay, &out->az);
	return 0;
}

static void
emit(struct hsvc_service *s, void *ud)
{
	struct imu *m = ud;
	struct hsvc_imu v = {0};
	if (m->sim) {
		double t = (double)m->seq++ / 200.0;
		v.gz = (float)sin(t) * 0.5f;
		v.az = (float)HSVC_G_MS2;
	} else if (m->replay) {
		int32_t r[6];
		if (fscanf(m->replay, "%d %d %d %d %d %d", &r[0], &r[1], &r[2],
		           &r[3], &r[4], &r[5]) != 6) {
			rewind(m->replay);
			if (fscanf(m->replay, "%d %d %d %d %d %d", &r[0], &r[1],
			           &r[2], &r[3], &r[4], &r[5]) != 6)
				return;
		}
		hsvc_imu_convert(r[0], r[1], r[2], r[3], r[4], r[5], &m->scale,
		                 &v.gx, &v.gy, &v.gz, &v.ax, &v.ay, &v.az);
	} else {
		if (iio_read(m, &v) < 0)
			return;
	}
	hsvc_stream(s, 0, now_ns(), &v, sizeof(v));
}

int
main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	struct imu m = {0};
	const char *file = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--iio") && i + 1 < argc)
			snprintf(m.iio, sizeof(m.iio),
			         "/sys/bus/iio/devices/iio:device%s", argv[++i]);
		else if (!strcmp(argv[i], "--iio-dir") && i + 1 < argc)
			snprintf(m.iio, sizeof(m.iio), "%s", argv[++i]);
		else if (!strcmp(argv[i], "--file") && i + 1 < argc)
			file = argv[++i];
		else if (!strcmp(argv[i], "--sim"))
			m.sim = 1;
	}
	m.scale.gyro = hsvc_iio_gyro_scale(1.0f);
	m.scale.accel = hsvc_iio_accel_scale(1.0f);
	if (m.iio[0])
		iio_load_scales(&m);
	if (file) {
		m.replay = fopen(file, "r");
		if (!m.replay)
			return 1;
	}
	if (!m.sim && !m.replay && !m.iio[0]) {
		fprintf(stderr,
		        "hsvc-imu: need --iio N, --iio-dir P, --file F or --sim\n");
		return 2;
	}
	struct hsvc_service *s =
		hsvc_publish("imu", HSVC_KIND_IMU, "kind=4\ndrv=iio\n");
	if (!s)
		return 1;
	return hsvc_serve(s, 5, emit, &m);
}
