// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file unit tests for driver-side pure logic: gaze, imu, ctrl map,
 *  xr panel geometry */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "gaze.h"
#include "imuscl.h"
#include "ctrlmap.h"
#include "hsvc_geom.h"

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
test_panel_parse(void)
{
	struct hsvcx_panel p;
	/* only pixels: every other key falls back to the generic default */
	assert(hsvcx_panel_parse("w=3840\nh=2160\n", &p) == 0);
	assert(p.w_px == 3840 && p.h_px == 2160);
	assert(fabsf(p.lens_sep_m - 0.0635f) < 1e-6f);
	assert(p.frame_ns == (uint64_t)(1e9 / 72.0f));
	assert(p.k0 == 1.f && p.k2 == 0.f && p.chroma_r == 1.f);
	/* lens centres are symmetric about the half-panel edge */
	assert(fabsf(p.lens_cx[0] + p.lens_cx[1] - 1.f) < 1e-6f);
	assert(p.lens_cx[0] > 0.f && p.lens_cx[1] < 1.f);
	/* fov lands in the sane band for a face-mounted panel */
	assert(p.fov[0] > 1.0f && p.fov[0] < 2.5f && p.fov[0] == p.fov[1]);

	/* explicit descriptor wins over defaults */
	assert(hsvcx_panel_parse(
	           "w=2160\nh=2160\nw_mm=119.1\nh_mm=67\nipd_mm=60\n"
	           "hz=90\nk0=0.5\ncr=0.9\ncb=1.1\n",
	           &p) == 0);
	assert(fabsf(p.w_m - 0.1191f) < 1e-5f);
	assert(fabsf(p.lens_sep_m - 0.060f) < 1e-5f);
	assert(p.frame_ns == (uint64_t)(1e9 / 90.0f));
	assert(fabsf(p.k0 - 0.5f) < 1e-6f);

	/* no panel pixels: -1, defaults still filled so callers can go on */
	assert(hsvcx_panel_parse("drv=sim\n", &p) < 0);
	assert(p.w_px == 0 && p.w_m > 0.f);
}

static void
test_panel_distort(void)
{
	struct hsvcx_panel p;
	hsvcx_panel_parse("w=2160\nh=2160\n", &p);
	float gx, gy, rx, ry, bx, by;
	/* identity polynomial passes uv through, chroma at 1 keeps rgb aligned */
	hsvcx_distort(&p, 0, 0.3f, 0.7f, &gx, &gy, &rx, &ry, &bx, &by);
	assert(fabsf(gx - 0.3f) < 1e-6f && fabsf(gy - 0.7f) < 1e-6f);
	assert(gx == rx && gy == ry && gx == bx && gy == by);
	/* a positive k2 scales samples up, pushing lookups off the lens */
	p.k2 = 0.2f;
	hsvcx_distort(&p, 0, 0.9f, 0.5f, &gx, &gy, &rx, &ry, &bx, &by);
	assert(gx > 0.9f);
	/* chroma shifts only the r/b channels */
	p.chroma_r = 1.1f;
	hsvcx_distort(&p, 0, 0.9f, 0.5f, &gx, &gy, &rx, &ry, &bx, &by);
	assert(rx != gx && bx == gx);
}

int
main(void)
{
	test_pupil_centroid();
	test_gaze_map();
	test_imu_scale();
	test_ctrl_map();
	test_panel_parse();
	test_panel_distort();
	puts("test_pure OK");
	return 0;
}
