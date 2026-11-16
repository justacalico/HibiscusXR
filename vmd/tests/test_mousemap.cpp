// mousemap.hpp pure-test: yaw/pitch/roll mapping, button bits,
// recenter, wand offset - the host half of what the in-guest
// hsvc-vmd driver's vmdmap.h used to cover.

#include "mousemap.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>


static int
closef(float a, float b)
{
	return fabsf(a - b) < 1e-5f;
}

int
main(void)
{
	struct vmd_mm m = {};

	// look: right mouse moves yaw right (negative yaw), up looks up
	vmd_mm_move(&m, 100, -100);
	assert(m.yaw == -100 * VMD_MM_SENS);
	assert(m.pitch == 100 * VMD_MM_SENS);
	vmd_mm_move(&m, 0, -100000);
	assert(m.pitch == VMD_MM_PITCH_MAX);
	vmd_mm_move(&m, 0, 100000);
	assert(m.pitch == -VMD_MM_PITCH_MAX);

	// wheel rolls
	float r0 = m.roll;
	vmd_mm_scroll(&m, 1);
	assert(m.roll == r0 - VMD_MM_ROLL_STEP);

	// buttons: left is the trigger bit plus the analog value
	vmd_mm_button(&m, 0, 1);
	assert((m.buttons & VMD_BTN_TRIG) && m.trigger == 1.f);
	vmd_mm_button(&m, 0, 0);
	assert(!(m.buttons & VMD_BTN_TRIG) && m.trigger == 0.f);
	vmd_mm_button(&m, 1, 1);
	assert(m.buttons & VMD_BTN_A);
	vmd_mm_button(&m, 2, 1);
	assert(m.buttons & VMD_BTN_MENU);
	vmd_mm_button(&m, 9, 1); // unmapped
	assert(m.buttons == (VMD_BTN_A | VMD_BTN_MENU));

	// side button recenters orientation
	m.yaw = 1.f;
	m.pitch = 0.5f;
	m.roll = 0.25f;
	assert(vmd_mm_button(&m, 3, 1) == 1);
	assert(m.yaw == 0.f && m.pitch == 0.f && m.roll == 0.f);
	assert(vmd_mm_button(&m, 3, 0) == 0);

	// identity orientation -> identity quat
	float q[4];
	vmd_mm_quat(&m, q);
	assert(closef(q[3], 1.f) && closef(q[0], 0.f) &&
	       closef(q[1], 0.f) && closef(q[2], 0.f));

	// -90 deg yaw (a right turn) swings the -Z wand offset to +X
	m.yaw = -1.5707963f; /* -pi/2 */
	float head[3] = {1.f, 1.6f, 0.f};
	float wp[3];
	vmd_mm_wand(&m, head, wp);
	assert(closef(wp[0], 1.f + 0.35f));
	assert(closef(wp[1], 1.6f - 0.12f));
	assert(closef(wp[2], 0.f));

	printf("mousemap test passed\n");
	return 0;
}
