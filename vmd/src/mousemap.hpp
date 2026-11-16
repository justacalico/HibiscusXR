// Copyright 2026, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Host mouse -> virtual 3-dof headset state, the pure half of the
// desktop sim so it stays testable without a window. Same conventions
// the retired in-guest vmd driver used: mouse right turns right,
// mouse up looks up, the wheel rolls, the side button recenters, and
// the wand follows the head so the trigger ray tracks gaze.

#pragma once

#include "vmd_proto.h"

#include <math.h>
#include <stdint.h>

#define VMD_MM_SENS 0.004f      /* rad per count */
#define VMD_MM_PITCH_MAX 1.45f
#define VMD_MM_ROLL_STEP 0.05f  /* rad per wheel detent */

/* mouse button indices the sim passes in (GLFW order):
 * 0 left = trigger, 1 right = A, 2 middle = menu, 3 side = recenter */
struct vmd_mm
{
	float yaw, pitch, roll;
	float trigger, grip;
	float stick[2];
	uint32_t buttons;
};

static inline float
vmd_mm_clampf(float v, float lo, float hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

static inline void
vmd_mm_move(struct vmd_mm *m, float dx, float dy)
{
	m->yaw -= dx * VMD_MM_SENS;
	m->pitch = vmd_mm_clampf(m->pitch - dy * VMD_MM_SENS,
	                         -VMD_MM_PITCH_MAX, VMD_MM_PITCH_MAX);
}

static inline void
vmd_mm_scroll(struct vmd_mm *m, float dy)
{
	m->roll -= dy * VMD_MM_ROLL_STEP;
}

/* One mouse button transition; returns 1 when it recentred the
 * orientation so the caller can log or reset derived state. */
static inline int
vmd_mm_button(struct vmd_mm *m, int btn, int down)
{
	uint32_t bit;
	switch (btn) {
	case 0: bit = VMD_BTN_TRIG; break;
	case 1: bit = VMD_BTN_A; break;
	case 2: bit = VMD_BTN_MENU; break;
	case 3:
		if (down) {
			m->yaw = 0.f;
			m->pitch = 0.f;
			m->roll = 0.f;
			return 1;
		}
		return 0;
	default:
		return 0;
	}
	if (down)
		m->buttons |= bit;
	else
		m->buttons &= ~bit;
	m->trigger = (m->buttons & VMD_BTN_TRIG) ? 1.f : 0.f;
	return 0;
}

/* Orientation as a quaternion, YXZ order: yaw around +Y, pitch around
 * +X, roll around +Z. */
static inline void
vmd_mm_quat(const struct vmd_mm *m, float out[4])
{
	float cy = cosf(m->yaw * 0.5f), sy = sinf(m->yaw * 0.5f);
	float cp = cosf(m->pitch * 0.5f), sp = sinf(m->pitch * 0.5f);
	float cr = cosf(m->roll * 0.5f), sr = sinf(m->roll * 0.5f);
	out[3] = cy * cp * cr + sy * sp * sr;
	out[0] = cy * sp * cr + sy * cp * sr;
	out[1] = sy * cp * cr - cy * sp * sr;
	out[2] = cy * cp * sr - sy * sp * cr;
}

static inline void
vmd_mm_rot(const float q[4], float x, float y, float z, float out[3])
{
	float tx = 2.f * (q[1] * z - q[2] * y);
	float ty = 2.f * (q[2] * x - q[0] * z);
	float tz = 2.f * (q[0] * y - q[1] * x);
	out[0] = x + q[3] * tx + q[1] * tz - q[2] * ty;
	out[1] = y + q[3] * ty + q[2] * tx - q[0] * tz;
	out[2] = z + q[3] * tz + q[0] * ty - q[1] * tx;
}

/* The wand sits a hand's width below and ahead of the head, oriented
 * with it - same trick the in-guest driver used for the trigger ray. */
static inline void
vmd_mm_wand(const struct vmd_mm *m, const float head[3], float out[3])
{
	float q[4];
	vmd_mm_quat(m, q);
	float o[3];
	vmd_mm_rot(q, 0.f, -0.12f, -0.35f, o);
	out[0] = head[0] + o[0];
	out[1] = head[1] + o[1];
	out[2] = head[2] + o[2];
}
