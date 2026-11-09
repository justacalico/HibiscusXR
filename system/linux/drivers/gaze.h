// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Eye-camera frame to gaze: pure math, no device code.
 *         A driver locates the pupil centroid in the frame, then this
 *         maps it to normalized gaze with per-eye calibration.
 */
#pragma once

#include <stdint.h>

struct hsvc_eye_cal
{
	float cx, cy;    /* pupil centroid when looking at panel center */
	float sx, sy;    /* pixels of travel for a +-1 gaze sweep */
	uint8_t present; /* calibration loaded */
};

/* Bright-blob centroid of a grayscale frame. Returns 0 when nothing
 * bright enough is found (blink / off-eye), else fills cx/cy. */
static inline int
hsvc_pupil_centroid(const uint8_t *img, uint32_t w, uint32_t h,
                    uint8_t thresh, float *cx, float *cy)
{
	uint64_t sx = 0, sy = 0, n = 0;
	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {
			if (img[y * w + x] >= thresh) {
				sx += x;
				sy += y;
				n++;
			}
		}
	}
	if (n == 0)
		return -1;
	*cx = (float)sx / (float)n;
	*cy = (float)sy / (float)n;
	return 0;
}

/* Centroid -> normalized gaze, clamped to [-1,1]. */
static inline float
hsvc_clamp1(float v)
{
	return v < -1.f ? -1.f : v > 1.f ? 1.f : v;
}

static inline int
hsvc_gaze(const struct hsvc_eye_cal *cal, float px, float py, float *gx,
          float *gy)
{
	if (!cal->present || cal->sx == 0.f || cal->sy == 0.f)
		return -1;
	*gx = hsvc_clamp1((px - cal->cx) / cal->sx);
	*gy = hsvc_clamp1((py - cal->cy) / cal->sy);
	return 0;
}
