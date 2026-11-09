// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc-ctrl - generic controller service over evdev.
 *         --evdev /dev/input/eventN reads input_event structs;
 *         --file replays "type code value" lines; --sim.
 *         Publishes kind=ctrl with gamepad-style state per index.
 */
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/input.h>

#include "hsvc/hsvc.h"
#include "ctrlmap.h"

struct ctrl
{
	int fd; /* evdev */
	FILE *replay;
	int sim;
	uint32_t idx;
	struct hsvc_ctrl_map map;
	uint64_t seq;
};

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void
emit(struct hsvc_service *s, void *ud)
{
	struct ctrl *c = ud;
	if (c->sim) {
		double t = (double)c->seq++ / 60.0;
		c->map.stick_x = (float)sin(t);
		c->map.stick_y = (float)cos(t);
		c->map.buttons = ((int)t % 4 == 0) ? HSVC_BTN_A : 0;
	} else if (c->replay) {
		unsigned ty, co;
		int v, got = 0;
		while (got < 64 &&
		       fscanf(c->replay, "%u %u %d", &ty, &co, &v) == 3) {
			hsvc_ev_apply(&c->map, (uint16_t)ty, (uint16_t)co, v);
			got++;
		}
		if (got == 0) {
			rewind(c->replay);
			return;
		}
	} else {
		struct input_event batch[32];
		ssize_t n = read(c->fd, batch, sizeof(batch));
		if (n < (ssize_t)sizeof(struct input_event))
			return;
		for (int i = 0; i < n / (ssize_t)sizeof(struct input_event); i++)
			hsvc_ev_apply(&c->map, batch[i].type, batch[i].code,
			              batch[i].value);
	}
	struct hsvc_ctrl out = {0};
	out.buttons = c->map.buttons;
	out.stick_x = c->map.stick_x;
	out.stick_y = c->map.stick_y;
	out.trigger = c->map.trigger;
	out.grip = c->map.grip;
	out.qw = 1.f;
	out.battery = 255;
	out.tracking = 0;
	out.connected = 1;
	hsvc_stream(s, c->idx, now_ns(), &out, sizeof(out));
}

int
main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	struct ctrl c = {.fd = -1};
	const char *dev = NULL, *file = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--evdev") && i + 1 < argc)
			dev = argv[++i];
		else if (!strcmp(argv[i], "--file") && i + 1 < argc)
			file = argv[++i];
		else if (!strcmp(argv[i], "--sim"))
			c.sim = 1;
		else if (!strcmp(argv[i], "--index") && i + 1 < argc)
			c.idx = (uint32_t)atoi(argv[++i]);
	}
	if (dev) {
		c.fd = open(dev, O_RDONLY | O_NONBLOCK);
		if (c.fd < 0)
			return 1;
	}
	if (file) {
		c.replay = fopen(file, "r");
		if (!c.replay)
			return 1;
	}
	if (!c.sim && c.fd < 0 && !c.replay) {
		fprintf(stderr,
		        "hsvc-ctrl: need --evdev P, --file F or --sim\n");
		return 2;
	}
	char name[32];
	snprintf(name, sizeof(name), "ctrl%u", c.idx);
	struct hsvc_service *s =
		hsvc_publish(name, HSVC_KIND_CTRL, "kind=2\ndrv=evdev\n");
	if (!s)
		return 1;
	return hsvc_serve(s, 4, emit, &c);
}
