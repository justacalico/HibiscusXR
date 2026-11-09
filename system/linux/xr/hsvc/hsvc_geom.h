// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc display geometry - the pure half of the monado driver.
 *
 * HMD services describe their panel through INFO kv keys; the driver
 * parses the blob once and everything below is pure arithmetic, which
 * is why it lives in a header the host test suite can compile.
 *
 *   w, h        panel pixels             (required)
 *   w_mm, h_mm  panel size, millimetres  (default 119.1 x 67.0)
 *   ipd_mm      lens separation          (default 63.5)
 *   hz          refresh rate             (default 72)
 *   k0,k2,k4,k6 lens polynomial          (default 1,0,0,0: no distortion)
 *   cr, cb      per-channel chroma gain  (default 1.0)
 */
#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "hsvc/proto.h"

struct hsvcx_panel
{
	uint32_t w_px, h_px;
	float w_m, h_m;
	float lens_sep_m;
	float fov[2];      /* per-view horizontal fov, radians */
	float lens_cx[2];  /* normalized lens centre x per view */
	float lens_cy;
	float tan_x, tan_y;
	float k0, k2, k4, k6;
	float chroma_r, chroma_b;
	uint64_t frame_ns;
};

static inline int
hsvcx_kv_f32(const char *kv, const char *key, float *out)
{
	char tmp[32];
	if (hsvc_kv_get_str(kv, key, tmp, sizeof(tmp)) < 0)
		return -1;
	char *end = NULL;
	float v = strtof(tmp, &end);
	if (end == tmp || *end != '\0')
		return -1;
	*out = v;
	return 0;
}

static inline float
hsvcx_kv_f32_or(const char *kv, const char *key, float def)
{
	float v;
	return hsvcx_kv_f32(kv, key, &v) == 0 ? v : def;
}

/* Fill `p` from a service descriptor blob; absent keys take the
 * generic defaults above. Returns 0 when w/h pixels were present, -1
 * when they were not (a panel-less descriptor still fills defaults so
 * bring-up always has something to render into). */
static inline int
hsvcx_panel_parse(const char *kv, struct hsvcx_panel *p)
{
	uint32_t w = 0, h = 0;
	hsvc_kv_get_u32(kv, "w", &w);
	hsvc_kv_get_u32(kv, "h", &h);
	p->w_px = w;
	p->h_px = h;
	p->w_m = hsvcx_kv_f32_or(kv, "w_mm", 119.1f) / 1000.0f;
	p->h_m = hsvcx_kv_f32_or(kv, "h_mm", 67.0f) / 1000.0f;
	p->lens_sep_m = hsvcx_kv_f32_or(kv, "ipd_mm", 63.5f) / 1000.0f;
	float hz = hsvcx_kv_f32_or(kv, "hz", 72.0f);
	p->frame_ns = hz > 0 ? (uint64_t)(1e9 / hz) : 0;
	p->k0 = hsvcx_kv_f32_or(kv, "k0", 1.0f);
	p->k2 = hsvcx_kv_f32_or(kv, "k2", 0.0f);
	p->k4 = hsvcx_kv_f32_or(kv, "k4", 0.0f);
	p->k6 = hsvcx_kv_f32_or(kv, "k6", 0.0f);
	p->chroma_r = hsvcx_kv_f32_or(kv, "cr", 1.0f);
	p->chroma_b = hsvcx_kv_f32_or(kv, "cb", 1.0f);

	const float half_w = p->w_m / 2.0f;
	const float relief = p->h_m / 2.0f;
	const float half_sep = p->lens_sep_m / 2.0f;
	p->fov[0] = p->fov[1] =
	    atanf(half_sep / relief) + atanf((half_w - half_sep) / relief);
	p->lens_cx[0] = (half_w - half_sep) / half_w;
	p->lens_cx[1] = half_sep / half_w;
	p->lens_cy = 0.5f;
	p->tan_x = half_w / relief;
	p->tan_y = p->h_m / relief;
	return (w && h) ? 0 : -1;
}

/* Lens distortion for one sample point, same polynomial the Android
 * drivers shipped: g is the undistorted lookup, r/b get their chroma
 * gains on top. Writes green into g, red into r, blue into b. */
static inline void
hsvcx_distort(const struct hsvcx_panel *p, int view, float u, float v,
              float *gx, float *gy, float *rx, float *ry, float *bx,
              float *by)
{
	const float cx = p->lens_cx[view];
	const float cy = p->lens_cy;
	const float px = (u - cx) * p->tan_x;
	const float py = (v - cy) * p->tan_y;
	const float r2 = px * px + py * py;
	const float scale = p->k0 + p->k2 * r2 + p->k4 * r2 * r2 +
	                    p->k6 * r2 * r2 * r2;
	const float x = cx + (px / p->tan_x) * scale;
	const float y = cy + (py / p->tan_y) * scale;
	*gx = x;
	*gy = y;
	*rx = cx + (x - cx) * p->chroma_r;
	*ry = cy + (y - cy) * p->chroma_r;
	*bx = cx + (x - cx) * p->chroma_b;
	*by = cy + (y - cy) * p->chroma_b;
}
