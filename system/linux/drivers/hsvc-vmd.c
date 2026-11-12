// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc-vmd - the virtual motion device. A mouse stands in for
 *         a 3-dof headset: motion steers the head, left click is the
 *         trigger, wheel rolls, side button recenters. Publishes
 *         kind=hmd and kind=ctrl like a real tracked setup, so the OS
 *         and its tooling can be driven by hand on any machine.
 *
 *         --evdev P reads input_event structs ("auto" scans for the
 *         first mouse, and is the default - none found sways like
 *         --sim); --file replays
 *         "type code value" lines; --sim sways. The parent pumps events
 *         into shared state; one child per published service, like
 *         hsvc-sim.
 *
 *         Standalone: when no hsvcd answers, the driver forks the
 *         embedded broker on a private rundir, so the single binary is
 *         the whole stack. Point tools at it with the HSVC_RUNDIR it
 *         prints.
 */
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <linux/input.h>

#include "hsvc/hsvc.h"
#include "vmdmap.h"

struct vmd
{
	int fd; /* evdev */
	FILE *replay;
	int sim;
	struct hsvc_vmd_map *map; /* shared with the service children */
};

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int
find_mouse(char *out, size_t n)
{
	for (int i = 0; i < 64; i++) {
		char p[64];
		snprintf(p, sizeof(p), "/dev/input/event%d", i);
		int fd = open(p, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;
		unsigned long rel[1] = {0}, key[8] = {0};
		int ok = ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel) >= 0 &&
		         (rel[0] & (1ul << REL_X)) &&
		         (rel[0] & (1ul << REL_Y)) &&
		         ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key)), key) >= 0 &&
		         (key[BTN_LEFT / 64] & (1ul << (BTN_LEFT % 64)));
		close(fd);
		if (ok) {
			snprintf(out, n, "%s", p);
			return 0;
		}
	}
	return -1;
}

/* Event source: the parent side. Applies every event to the shared
 * map; children stream whatever the map currently says. */
static int
pump_evdev(struct vmd *v)
{
	for (;;) {
		struct input_event batch[32];
		ssize_t n = read(v->fd, batch, sizeof(batch));
		if (n < (ssize_t)sizeof(struct input_event)) {
			if (n < 0 && errno == EINTR)
				continue;
			return 1;
		}
		for (int i = 0; i < n / (ssize_t)sizeof(struct input_event); i++)
			hsvc_vmd_apply(v->map, batch[i].type, batch[i].code,
			               batch[i].value);
	}
}

static int
pump_file(struct vmd *v)
{
	for (;;) {
		unsigned ty, co;
		int val, got = 0;
		while (got < 32 &&
		       fscanf(v->replay, "%u %u %d", &ty, &co, &val) == 3) {
			hsvc_vmd_apply(v->map, (uint16_t)ty, (uint16_t)co, val);
			got++;
		}
		if (got == 0)
			for (;;)
				pause(); /* hold last pose */
		usleep(10000);
	}
	return 0;
}

static int
pump_sim(struct vmd *v)
{
	double t = 0;
	for (;;) {
		int dx = (int)lrint(sin(t * 0.4) * 3.0);
		if (dx)
			hsvc_vmd_apply(v->map, EV_REL, REL_X, dx);
		t += 0.01;
		usleep(10000);
	}
	return 0;
}

static void
hmd_emit(struct hsvc_service *s, void *ud)
{
	struct vmd *v = ud;
	struct hsvc_pose p = {0};
	hsvc_vmd_quat(v->map, &p.qx, &p.qy, &p.qz, &p.qw);
	p.state = 1; /* orientation only: 3-dof, no position */
	hsvc_stream(s, 0, now_ns(), &p, sizeof(p));
}

static void
ctrl_emit(struct hsvc_service *s, void *ud)
{
	struct vmd *v = ud;
	struct hsvc_ctrl c = {0};
	/* the wand follows the head so the trigger ray tracks gaze */
	hsvc_vmd_quat(v->map, &c.qx, &c.qy, &c.qz, &c.qw);
	c.pz = -0.3f;
	c.trigger = v->map->trigger;
	c.buttons = v->map->buttons;
	c.battery = 255;
	c.tracking = 1;
	c.connected = 1;
	hsvc_stream(s, 0, now_ns(), &c, sizeof(c));
}

/* No daemon answering -> bring a private bus up in-process. With no
 * HSVC_RUNDIR the bus lives under /tmp/hibiscus-$UID so clients can
 * find it without root. */
static int
ensure_bus(void)
{
	char sock[512];
	snprintf(sock, sizeof(sock), "%s/%s", hsvc_rundir(),
	         HSVC_DAEMON_SOCK);
	int fd = hsvc_dial(sock);
	if (fd >= 0) {
		close(fd);
		return 0;
	}
	if (!getenv("HSVC_RUNDIR")) {
		char dir[256];
		snprintf(dir, sizeof(dir), "/tmp/hibiscus-%d", (int)getuid());
		setenv("HSVC_RUNDIR", dir, 1);
	}
	pid_t p = fork();
	if (p == 0) {
		prctl(PR_SET_PDEATHSIG, SIGTERM);
		_exit(hsvc_broker_run());
	}
	if (p < 0)
		return -1;
	snprintf(sock, sizeof(sock), "%s/%s", hsvc_rundir(),
	         HSVC_DAEMON_SOCK);
	for (int i = 0; i < 100; i++) {
		fd = hsvc_dial(sock);
		if (fd >= 0) {
			close(fd);
			fprintf(stderr,
			        "hsvc-vmd: own bus at %s "
			        "(HSVC_RUNDIR=%s for tools)\n",
			        hsvc_rundir(), hsvc_rundir());
			return 0;
		}
		usleep(10000);
	}
	fprintf(stderr, "hsvc-vmd: embedded bus did not come up\n");
	return -1;
}

static int
run_hmd(struct vmd *v)
{
	struct hsvc_service *s =
		hsvc_publish("vmd-hmd", HSVC_KIND_HMD,
		             "kind=1\ndrv=vmd\nw=2160\nh=2160\n");
	if (!s)
		return 1;
	return hsvc_serve(s, 4, hmd_emit, v) < 0 ? 1 : 0;
}

static int
run_ctrl(struct vmd *v)
{
	struct hsvc_service *s =
		hsvc_publish("vmd-ctrl", HSVC_KIND_CTRL,
		             "kind=2\ndrv=vmd\nn=1\n");
	if (!s)
		return 1;
	return hsvc_serve(s, 4, ctrl_emit, v) < 0 ? 1 : 0;
}

int
main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	struct vmd v = {.fd = -1};
	const char *dev = NULL, *file = NULL;
	char found[64];
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--evdev") && i + 1 < argc)
			dev = argv[++i];
		else if (!strcmp(argv[i], "--file") && i + 1 < argc)
			file = argv[++i];
		else if (!strcmp(argv[i], "--sim"))
			v.sim = 1;
	}
	if (!dev && !file && !v.sim)
		dev = "auto";
	if (dev && !strcmp(dev, "auto")) {
		if (find_mouse(found, sizeof(found)) < 0) {
			fprintf(stderr, "hsvc-vmd: no mouse node found - "
			        "swaying on its own like --sim\n");
			v.sim = 1;
			dev = NULL;
		} else {
			dev = found;
		}
	}
	if (dev) {
		v.fd = open(dev, O_RDONLY);
		if (v.fd < 0) {
			fprintf(stderr, "hsvc-vmd: cannot open %s\n", dev);
			return 1;
		}
	}
	if (file) {
		v.replay = fopen(file, "r");
		if (!v.replay)
			return 1;
	}
	if (!v.sim && v.fd < 0 && !v.replay) {
		fprintf(stderr,
		        "hsvc-vmd: need --evdev P, --file F or --sim\n");
		return 2;
	}
	if (ensure_bus() < 0)
		return 1;
	v.map = mmap(NULL, sizeof(*v.map), PROT_READ | PROT_WRITE,
	             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (v.map == MAP_FAILED)
		return 1;
	for (int i = 0; i < 2; i++) {
		pid_t p = fork();
		if (p == 0) {
			prctl(PR_SET_PDEATHSIG, SIGTERM);
			return i == 0 ? run_hmd(&v) : run_ctrl(&v);
		}
	}
	if (v.sim)
		return pump_sim(&v);
	if (v.replay)
		return pump_file(&v);
	return pump_evdev(&v);
}
