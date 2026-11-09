// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file unit tests for driver-side pure logic: gaze, imu, ctrl map,
 *  vmd mouse map */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "gaze.h"
#include "imuscl.h"
#include "ctrlmap.h"
#include "vmdmap.h"

static void
test_pupil_centroid(void)
{
	uint8_t img[64 * 48] = {0};
	/* bright 4x4 blob at (20,10) */
	for (int y = 10; y < 14; y++)
		for (int x = 20; x < 24; x++)
			img[y * 64 + x] = 255;
	float cx, cy;
	assert(hsvc_pupil_centroid(img, 64, 48, 200, &cx, &cy) == 0);
	assert(fabsf(cx - 21.5f) < 0.01f);
	assert(fabsf(cy - 11.5f) < 0.01f);
	/* all dark -> no pupil */
	memset(img, 0, sizeof(img));
	assert(hsvc_pupil_centroid(img, 64, 48, 200, &cx, &cy) < 0);
}

static void
test_gaze_map(void)
{
	struct hsvc_eye_cal cal = {.cx = 32, .cy = 24, .sx = 32, .sy = 24,
	                           .present = 1};
	float gx, gy;
	assert(hsvc_gaze(&cal, 32, 24, &gx, &gy) == 0);
	assert(gx == 0.f && gy == 0.f);
	assert(hsvc_gaze(&cal, 64, 0, &gx, &gy) == 0);
	assert(gx == 1.f && gy == -1.f);
	/* beyond range clamps */
	assert(hsvc_gaze(&cal, 255, 255, &gx, &gy) == 0);
	assert(gx == 1.f && gy == 1.f);
	/* no calibration -> invalid */
	cal.present = 0;
	assert(hsvc_gaze(&cal, 32, 24, &gx, &gy) < 0);
}

static void
test_imu_scale(void)
{
	struct hsvc_imu_scale s = {.gyro = hsvc_iio_gyro_scale(0.5f),
	                           .accel = hsvc_iio_accel_scale(0.001f)};
	float gx, gy, gz, ax, ay, az;
	hsvc_imu_convert(100, -100, 0, 1000, 0, -1000, &s, &gx, &gy, &gz,
	                 &ax, &ay, &az);
	assert(fabsf(gx - 100.f * 0.5f * HSVC_DEG2RAD) < 1e-5);
	assert(fabsf(gy + 100.f * 0.5f * HSVC_DEG2RAD) < 1e-5);
	assert(gz == 0.f);
	assert(fabsf(ax - 1000.f * 0.001f * HSVC_G_MS2) < 1e-4);
	assert(fabsf(az + 1000.f * 0.001f * HSVC_G_MS2) < 1e-4);
}

static void
test_ctrl_map(void)
{
	struct hsvc_ctrl_map m = {0};
	hsvc_ev_apply(&m, HSVC_EV_KEY, HSVC_BTN_SOUTH, 1);
	assert(m.buttons & HSVC_BTN_A);
	hsvc_ev_apply(&m, HSVC_EV_KEY, HSVC_BTN_START, 1);
	assert(m.buttons & HSVC_BTN_SYS);
	hsvc_ev_apply(&m, HSVC_EV_KEY, HSVC_BTN_SOUTH, 0);
	assert(!(m.buttons & HSVC_BTN_A));
	hsvc_ev_apply(&m, HSVC_EV_ABS, HSVC_ABS_X, 32767);
	assert(fabsf(m.stick_x - 1.f) < 1e-4);
	hsvc_ev_apply(&m, HSVC_EV_ABS, HSVC_ABS_Y, -32767);
	assert(fabsf(m.stick_y + 1.f) < 1e-4);
	hsvc_ev_apply(&m, HSVC_EV_ABS, HSVC_ABS_Z, 32767);
	assert(fabsf(m.trigger - 1.f) < 1e-4);
	hsvc_ev_apply(&m, 99, 1, 1); /* unknown type ignored */
	assert(m.buttons == HSVC_BTN_SYS);
}

static void
test_vmd_map(void)
{
	struct hsvc_vmd_map m = {0};
	/* mouse right -> look right (yaw down), mouse up -> look up */
	hsvc_vmd_apply(&m, HSVC_EV_REL, HSVC_REL_X, 100);
	assert(m.yaw < 0.f);
	hsvc_vmd_apply(&m, HSVC_EV_REL, HSVC_REL_Y, -100);
	assert(m.pitch > 0.f);
	/* pitch clamps at the limit */
	hsvc_vmd_apply(&m, HSVC_EV_REL, HSVC_REL_Y, -100000);
	assert(m.pitch == HSVC_VMD_PITCH_MAX);
	hsvc_vmd_apply(&m, HSVC_EV_REL, HSVC_REL_Y, 100000);
	assert(m.pitch == -HSVC_VMD_PITCH_MAX);
	/* wheel rolls */
	hsvc_vmd_apply(&m, HSVC_EV_REL, HSVC_REL_WHEEL, 1);
	assert(m.roll < 0.f);
	/* left click is the trigger */
	hsvc_vmd_apply(&m, HSVC_EV_KEY, HSVC_BTN_MOUSE_L, 1);
	assert(m.buttons & HSVC_BTN_TRIG);
	assert(m.trigger == 1.f);
	hsvc_vmd_apply(&m, HSVC_EV_KEY, HSVC_BTN_MOUSE_L, 0);
	assert(!(m.buttons & HSVC_BTN_TRIG));
	assert(m.trigger == 0.f);
	/* side button recenters */
	hsvc_vmd_apply(&m, HSVC_EV_KEY, HSVC_BTN_MOUSE_SIDE, 1);
	assert(m.yaw == 0.f && m.pitch == 0.f && m.roll == 0.f);
	/* quat: identity at rest, 90 deg yaw puts it all in qy */
	float qx, qy, qz, qw;
	hsvc_vmd_quat(&m, &qx, &qy, &qz, &qw);
	assert(qw == 1.f && qx == 0.f && qy == 0.f && qz == 0.f);
	m.yaw = (float)M_PI / 2.f;
	hsvc_vmd_quat(&m, &qx, &qy, &qz, &qw);
	assert(fabsf(qy - sqrtf(0.5f)) < 1e-6);
	assert(fabsf(qw - sqrtf(0.5f)) < 1e-6);
	assert(fabsf(qx) < 1e-6 && fabsf(qz) < 1e-6);
}

int
main(void)
{
	test_pupil_centroid();
	test_gaze_map();
	test_imu_scale();
	test_ctrl_map();
	test_vmd_map();
	puts("test_pure OK");
	return 0;
}
