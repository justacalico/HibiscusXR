// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  evdev codes -> generic controller state. Headset-neutral:
 *         any pad/wand whose kernel driver reports gamepad codes maps
 *         onto the hsvc_ctrl fields. Pure mapping, no device code.
 */
#pragma once

#include <stdint.h>

#include "hsvc/proto.h"

/* evdev codes we consume (linux/input-event-codes.h values, inlined so
 * the pure module never needs kernel headers to build or test). */
#define HSVC_EV_KEY 0x01
#define HSVC_EV_ABS 0x03
#define HSVC_BTN_SOUTH 0x130
#define HSVC_BTN_EAST 0x131
#define HSVC_BTN_SELECT 0x13a
#define HSVC_BTN_START 0x13b
#define HSVC_BTN_THUMBL 0x13d
#define HSVC_BTN_TL 0x136
#define HSVC_BTN_TL2 0x138
#define HSVC_ABS_X 0x00
#define HSVC_ABS_Y 0x01
#define HSVC_ABS_Z 0x02 /* often trigger */
#define HSVC_ABS_RZ 0x05 /* often trigger */

struct hsvc_ctrl_map
{
	uint32_t buttons;
	float stick_x, stick_y, trigger, grip;
};

/* Map one evdev event into controller state. */
static inline void
hsvc_ev_apply(struct hsvc_ctrl_map *m, uint16_t type, uint16_t code,
              int32_t value)
{
	if (type == HSVC_EV_KEY) {
		uint32_t bit = 0;
		switch (code) {
		case HSVC_BTN_SOUTH: bit = HSVC_BTN_A; break;
		case HSVC_BTN_EAST: bit = HSVC_BTN_B; break;
		case HSVC_BTN_SELECT: bit = HSVC_BTN_MENU; break;
		case HSVC_BTN_START: bit = HSVC_BTN_SYS; break;
		case HSVC_BTN_THUMBL: bit = HSVC_BTN_STICK; break;
		case HSVC_BTN_TL: bit = HSVC_BTN_GRIP; break;
		case HSVC_BTN_TL2: bit = HSVC_BTN_TRIG; break;
		default: break;
		}
		if (value)
			m->buttons |= bit;
		else
			m->buttons &= ~bit;
		return;
	}
	if (type == HSVC_EV_ABS) {
		/* 16-bit axis to -1..1 / 0..1 */
		float v = (float)value / 32767.f;
		switch (code) {
		case HSVC_ABS_X: m->stick_x = v; break;
		case HSVC_ABS_Y: m->stick_y = v; break;
		case HSVC_ABS_Z: m->trigger = (v + 1.f) * 0.5f; break;
		case HSVC_ABS_RZ: m->grip = (v + 1.f) * 0.5f; break;
		default: break;
		}
	}
}
