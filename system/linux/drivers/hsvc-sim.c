// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc-sim - the virtual headset. Serves hmd, ctrl x2 and eye
 *         with synthetic motion so the OS and its tooling run on any
 *         machine with zero hardware. One fork per published service.
 */
#include <math.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "hsvc/hsvc.h"

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

struct gen
{
	uint64_t t0;
	double phase;
};

static void
gen_init(struct gen *g, double phase)
{
	g->t0 = now_ns();
	g->phase = phase;
}

static double
gen_t(const struct gen *g)
{
	return (double)(now_ns() - g->t0) / 1e9 + g->phase;
}

static void
hmd_emit(struct hsvc_service *s, void *ud)
{
	struct gen *g = ud;
	double t = gen_t(g);
	struct hsvc_pose p = {0};
	/* slow yaw figure-eight + a little bob */
	double yaw = sin(t * 0.4) * 0.5;
	p.qw = cos(yaw / 2);
	p.qy = sin(yaw / 2);
	p.py = (float)(sin(t * 0.8) * 0.02);
	p.state = 3;
	hsvc_stream(s, 0, now_ns(), &p, sizeof(p));
}

static void
ctrl_emit(struct hsvc_service *s, void *ud)
{
	struct gen *g = ud;
	double t = gen_t(g);
	uint32_t idx = (g->phase > 1.0) ? 1 : 0;
	struct hsvc_ctrl c = {0};
	c.px = idx ? 0.15f : -0.15f;
	c.py = -0.2f;
	c.pz = -0.3f;
	c.qw = 1.f;
	c.stick_x = (float)sin(t);
	c.stick_y = (float)cos(t);
	c.trigger = (float)(sin(t * 2) + 1.0) * 0.5f;
	c.buttons = ((int)(t * 0.5) % 8 == 0) ? HSVC_BTN_A : 0;
	c.battery = 100 - (idx * 10);
	c.tracking = 3;
	c.connected = 1;
	hsvc_stream(s, idx, now_ns(), &c, sizeof(c));
}

static void
eye_emit(struct hsvc_service *s, void *ud)
{
	struct gen *g = ud;
	double t = gen_t(g);
	struct hsvc_gaze gz;
	gz.lx = gz.rx = (float)sin(t);
	gz.ly = gz.ry = (float)cos(t * 0.7) * 0.6f;
	gz.valid = 3;
	hsvc_stream(s, 0, now_ns(), &gz, sizeof(gz));
}

static int
run(const char *name, enum hsvc_kind kind, const char *desc,
    hsvc_emit_cb emit, double phase)
{
	struct hsvc_service *s = hsvc_publish(name, kind, desc);
	if (!s) {
		fprintf(stderr, "hsvc-sim: cannot publish %s\n", name);
		return 1;
	}
	static struct gen g;
	gen_init(&g, phase);
	return hsvc_serve(s, 8, emit, &g);
}

int
main(void)
{
	signal(SIGPIPE, SIG_IGN);
	for (int i = 0; i < 3; i++) {
		pid_t p = fork();
		if (p == 0) {
			prctl(PR_SET_PDEATHSIG, SIGTERM);
			switch (i) {
			case 0:
				return run("hmd", HSVC_KIND_HMD,
				           "kind=1\ndrv=sim\nw=2160\nh=2160\n"
				           "w_mm=119.1\nh_mm=67.0\nipd_mm=63.5\n"
				           "hz=72\nk0=1\ncr=1\ncb=1\n",
				           hmd_emit, 0);
			case 1:
				return run("ctrl", HSVC_KIND_CTRL,
				           "kind=2\ndrv=sim\nn=2\n",
				           ctrl_emit, 2.0);
			default:
				return run("eye", HSVC_KIND_EYE,
				           "kind=3\ndrv=sim\n",
				           eye_emit, 4.0);
			}
		}
	}
	int st;
	while (wait(&st) > 0)
		;
	return 0;
}
