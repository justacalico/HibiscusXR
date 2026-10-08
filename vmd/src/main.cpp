// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
//
// vmd - Hibiscus virtual machine driver, host side.
//
//   vmd -desktopsim | -novr | -pc   desktop window, mouse drives the HMD
//   vmd -openxr                    OpenXR app mode (WiVRn/Monado runtime)
//   vmd -selftest                  pose channel smoke test, no window
//
//   -img <dir>     image set dir (system-hibiscus-full-neo2.img, vendor.img,
//                  boot.img) - default $PN2_ROOT/out
//   -novm          skip qemu, pose channel + UI only
//   -kernel/-dtb/-append   kernel overrides for the guest

#include "vmd.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>


static std::atomic<bool> g_stop{false};

static void
on_sigint(int)
{
	g_stop.store(true);
}

static int
selftest(vmd_pose_state *state)
{
	// local client reads a handful of packets off the pose channel and
	// checks the wire format - same path the in-OS driver takes
	state->quat[3] = 1.0f;
	state->pos[1] = 1.6f;
	std::atomic<bool> stop{false};
	std::thread srv([&] { vmd_pose_serve(state, &stop); });
	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in a = {};
	a.sin_family = AF_INET;
	a.sin_port = htons(VMD_PROTO_PORT);
	inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
	if (connect(fd, (struct sockaddr *)&a, sizeof(a)) != 0) {
		perror("selftest connect");
		stop.store(true);
		srv.join();
		return 1;
	}
	vmd_pose_packet pkt = {};
	ssize_t got = 0;
	char *p = (char *)&pkt;
	while (got < (ssize_t)sizeof(pkt)) {
		ssize_t r = read(fd, p + got, sizeof(pkt) - got);
		if (r <= 0) break;
		got += r;
	}
	close(fd);
	stop.store(true);
	srv.join();

	if (got != sizeof(pkt) || pkt.magic != VMD_PROTO_MAGIC ||
	    pkt.version != VMD_PROTO_VERSION) {
		fprintf(stderr, "selftest: bad packet (got %zd)\n", got);
		return 1;
	}
	printf("selftest: packet ok seq=%u flags=0x%x q=(%.2f %.2f %.2f %.2f) "
	       "p=(%.2f %.2f %.2f)\n",
	       pkt.seq, pkt.flags, pkt.quat[0], pkt.quat[1], pkt.quat[2],
	       pkt.quat[3], pkt.pos[0], pkt.pos[1], pkt.pos[2]);
	return 0;
}

int
main(int argc, char **argv)
{
	enum { SIM, XR, SELFTEST } mode = SIM;
	vmd_vm_opts opts;
	bool novm = false;

	const char *env_root = getenv("PN2_ROOT");
	opts.imgdir = env_root ? std::string(env_root) + "/out" : "out";
	if (getenv("VMD_QEMU") != nullptr)
		opts.qemu = getenv("VMD_QEMU");

	for (int i = 1; i < argc; i++) {
		std::string a = argv[i];
		if (a == "-openxr") mode = XR;
		else if (a == "-desktopsim" || a == "-novr" || a == "-pc") mode = SIM;
		else if (a == "-selftest") mode = SELFTEST;
		else if (a == "-novm") novm = true;
		else if (a == "-img" && i + 1 < argc) opts.imgdir = argv[++i];
		else if (a == "-kernel" && i + 1 < argc) opts.kernel = argv[++i];
		else if (a == "-dtb" && i + 1 < argc) opts.dtb = argv[++i];
		else if (a == "-append" && i + 1 < argc) opts.cmdline = argv[++i];
		else if (a == "-qemu" && i + 1 < argc) opts.qemu = argv[++i];
		else if (a == "-iso" && i + 1 < argc) opts.iso = argv[++i];
		else if (a == "-initrd" && i + 1 < argc) opts.initrd = argv[++i];
		else if (a == "-disk" && i + 1 < argc) opts.disk = argv[++i];
		else if (a == "-serial" && i + 1 < argc) opts.serial = argv[++i];
		else if (a == "-machine" && i + 1 < argc) opts.machine = argv[++i];
		else {
			fprintf(stderr, "usage: vmd [-openxr|-desktopsim|-novr|-pc|-selftest] "
			        "[-img dir] [-novm] [-qemu bin] [-machine m] "
			        "[-kernel k] [-dtb d] [-initrd r] [-disk f] "
			        "[-serial s] [-append c]\n");
			return a == "-h" || a == "--help" ? 0 : 2;
		}
	}

	vmd_pose_state state;
	if (mode == SELFTEST) {
		return selftest(&state);
	}

	signal(SIGINT, on_sigint);
	signal(SIGTERM, on_sigint);

	vmd_vm *vm = nullptr;
	if (!novm) {
		vm = vmd_vm_start(opts);
		if (vm == nullptr) {
			fprintf(stderr, "vmd: no VM - pose channel + sim still up\n");
		}
	}

	std::thread pose_thread(vmd_pose_serve, &state, &g_stop);
	int rc = mode == XR ? vmd_xr_run(vm, &state, &g_stop)
	                    : vmd_sim_run(vm, &state, &g_stop);
	g_stop.store(true);
	pose_thread.join();
	vmd_vm_stop(vm);
	return rc;
}
