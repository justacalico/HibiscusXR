// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Raw IIO integer samples -> SI units. IIO exposes
 *         in_*_scale alongside raw channels; the product is SI.
 *         Pure conversion, no sysfs code.
 */
#pragma once

#include <stdint.h>

struct hsvc_imu_scale
{
	float gyro;  /* rad/s per count   (IIO scale is dps; see below) */
	float accel; /* m/s^2 per count   (IIO scale is g's or m/s^2)   */
};

#define HSVC_DEG2RAD 0.017453292519943295f
#define HSVC_G_MS2 9.80665f

/* IIO angular velocity scales are degrees/second per count. */
static inline float
hsvc_iio_gyro_scale(float iio_scale)
{
	return iio_scale * HSVC_DEG2RAD;
}

/* IIO accel scales are g's per count on most parts. */
static inline float
hsvc_iio_accel_scale(float iio_scale)
{
	return iio_scale * HSVC_G_MS2;
}

static inline void
hsvc_imu_convert(int32_t raw_gx, int32_t raw_gy, int32_t raw_gz,
                 int32_t raw_ax, int32_t raw_ay, int32_t raw_az,
                 const struct hsvc_imu_scale *s, float *gx, float *gy,
                 float *gz, float *ax, float *ay, float *az)
{
	*gx = (float)raw_gx * s->gyro;
	*gy = (float)raw_gy * s->gyro;
	*gz = (float)raw_gz * s->gyro;
	*ax = (float)raw_ax * s->accel;
	*ay = (float)raw_ay * s->accel;
	*az = (float)raw_az * s->accel;
}
