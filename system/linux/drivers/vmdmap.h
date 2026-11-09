// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  evdev mouse -> virtual 3-dof headset state. Relative motion
 *         integrates to yaw/pitch, the wheel to roll, buttons to the
 *         generic ctrl fields. Same conventions as the old vmd
 *         desktop sim: mouse right turns right, mouse up looks up.
 *         Pure mapping, no device code.
 */
#pragma once

#include <math.h>
#include <stdint.h>

#include "hsvc/proto.h"

/* evdev codes we consume (linux/input-event-codes.h values, inlined so
 * the pure module never needs kernel headers to build or test). */
#define HSVC_EV_KEY 0x01
#define HSVC_EV_REL 0x02
#define HSVC_REL_X 0x00
#define HSVC_REL_Y 0x01
#define HSVC_REL_WHEEL 0x08
#define HSVC_BTN_MOUSE_L 0x110
#define HSVC_BTN_MOUSE_R 0x111
#define HSVC_BTN_MOUSE_M 0x112
#define HSVC_BTN_MOUSE_SIDE 0x113

#define HSVC_VMD_SENS 0.004f      /* rad per count, same as vmd sim */
#define HSVC_VMD_PITCH_MAX 1.45f  /* same clamp as vmd sim */
#define HSVC_VMD_ROLL_STEP 0.05f  /* rad per wheel detent */

struct hsvc_vmd_map
{
	float yaw, pitch, roll;
	float trigger;
	uint32_t buttons;
};

static inline float
hsvc_clampf(float v, float lo, float hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

/* Map one evdev event into the virtual headset. A BTN_SIDE press
 * recenters orientation, like the recenter key on a real 3-dof. */
static inline void
hsvc_vmd_apply(struct hsvc_vmd_map *m, uint16_t type, uint16_t code,
               int32_t value)
{
	if (type == HSVC_EV_REL) {
		switch (code) {
		case HSVC_REL_X:
			m->yaw -= (float)value * HSVC_VMD_SENS;
			break;
		case HSVC_REL_Y:
			m->pitch = hsvc_clampf(
				m->pitch - (float)value * HSVC_VMD_SENS,
				-HSVC_VMD_PITCH_MAX, HSVC_VMD_PITCH_MAX);
			break;
		case HSVC_REL_WHEEL:
			m->roll -= (float)value * HSVC_VMD_ROLL_STEP;
			break;
		default:
			break;
		}
		return;
	}
	if (type == HSVC_EV_KEY) {
		uint32_t bit = 0;
		switch (code) {
		case HSVC_BTN_MOUSE_L: bit = HSVC_BTN_TRIG; break;
		case HSVC_BTN_MOUSE_R: bit = HSVC_BTN_A; break;
		case HSVC_BTN_MOUSE_M: bit = HSVC_BTN_MENU; break;
		case HSVC_BTN_MOUSE_SIDE:
			if (value) {
				m->yaw = 0.f;
				m->pitch = 0.f;
				m->roll = 0.f;
			}
			return;
		default:
			break;
		}
		if (value)
			m->buttons |= bit;
		else
			m->buttons &= ~bit;
		m->trigger = (m->buttons & HSVC_BTN_TRIG) ? 1.f : 0.f;
	}
}

/* Orientation as a quaternion, YXZ order: yaw around +Y, pitch around
 * +X, roll around +Z - identical to vmd's quat_from_angles. */
static inline void
hsvc_vmd_quat(const struct hsvc_vmd_map *m, float *qx, float *qy,
              float *qz, float *qw)
{
	float cy = cosf(m->yaw * 0.5f), sy = sinf(m->yaw * 0.5f);
	float cp = cosf(m->pitch * 0.5f), sp = sinf(m->pitch * 0.5f);
	float cr = cosf(m->roll * 0.5f), sr = sinf(m->roll * 0.5f);
	*qw = cr * cp * cy + sr * sp * sy;
	*qx = cr * sp * cy + sr * cp * sy;
	*qy = cr * cp * sy - sr * sp * cy;
	*qz = sr * cp * cy - cr * sp * sy;
}
