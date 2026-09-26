// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
//
// QEMU launcher + framebuffer reader.
//
// The guest gets the same artifacts fastboot flashes: boot.img, vendor.img
// and system-hibiscus-full.img on virtio-blk, user networking (which is
// what gives the guest its 10.0.2.2 route back to us), and a virtio-gpu
// whose framebuffer we pull through QMP screendump.
//
// Reality check: the stock sdm845 kernel only runs on Qualcomm hardware,
// so until the LineageOS fork produces a generic kernel you pass a
// virtio-capable one with -kernel/-dtb. The pose channel and desktop sim
// work without the guest booted all the way.

#include "vmd.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>


struct vmd_vm
{
	pid_t pid = -1;
	std::string qmp;    // unix socket path
	std::string ppm;    // screendump target
	int fb_w = 0, fb_h = 0;
	std::mutex fb_lock;
	std::vector<uint8_t> fb;
	std::atomic<bool> poll_stop{false};
	std::thread poll_thread;
};

static bool
file_exists(const std::string &p)
{
	struct stat st;
	return stat(p.c_str(), &st) == 0;
}

// minimal QMP exchange: handshake, then one command; returns reply text
static std::string
qmp_cmd(int fd, const std::string &json)
{
	std::string out;
	if (write(fd, json.c_str(), json.size()) != (ssize_t)json.size()) {
		return out;
	}
	char buf[8192];
	struct pollfd pfd = {fd, POLLIN, 0};
	// replies can take a moment on a busy guest
	for (int i = 0; i < 50; i++) {
		if (poll(&pfd, 1, 100) <= 0) {
			continue;
		}
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n <= 0) {
			break;
		}
		out.append(buf, n);
		if (out.find("return") != std::string::npos ||
		    out.find("error") != std::string::npos) {
			break;
		}
	}
	return out;
}

static int
qmp_open(const std::string &path)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un a = {};
	a.sun_family = AF_UNIX;
	snprintf(a.sun_path, sizeof(a.sun_path), "%s", path.c_str());
	if (connect(fd, (struct sockaddr *)&a, sizeof(a)) != 0) {
		close(fd);
		return -1;
	}
	char buf[4096];
	read(fd, buf, sizeof(buf)); // greeting
	write(fd, "{\"execute\":\"qmp_capabilities\"}\n", 29);
	read(fd, buf, sizeof(buf)); // ack
	return fd;
}

// screendump writes a binary PPM: "P6\n<w> <h>\n255\n" + rgb
static bool
read_ppm(const std::string &path, int *w, int *h, std::vector<uint8_t> *rgb)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) {
		return false;
	}
	char magic[3] = {};
	int maxv = 0;
	if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0 ||
	    fscanf(f, "%d %d %d", w, h, &maxv) != 3 || fgetc(f) == EOF) {
		fclose(f);
		return false;
	}
	rgb->resize((size_t)*w * *h * 3);
	size_t n = fread(rgb->data(), 1, rgb->size(), f);
	fclose(f);
	return n == rgb->size();
}

static void
vmd_vm_poll(vmd_vm *vm)
{
	while (!vm->poll_stop.load()) {
		int fd = qmp_open(vm->qmp);
		if (fd < 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		std::string cmd = "{\"execute\":\"screendump\",\"arguments\":{\"filename\":\"" +
		                  vm->ppm + "\"}}\n";
		std::string rep = qmp_cmd(fd, cmd);
		close(fd);
		if (rep.find("error") != std::string::npos) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		int w = 0, h = 0;
		std::vector<uint8_t> rgb;
		if (read_ppm(vm->ppm, &w, &h, &rgb) && w > 0 && h > 0) {
			std::lock_guard<std::mutex> g(vm->fb_lock);
			vm->fb_w = w;
			vm->fb_h = h;
			vm->fb.swap(rgb);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(66));
	}
}

vmd_vm *
vmd_vm_start(const std::string &imgdir, const std::string &kernel,
             const std::string &dtb, const std::string &cmdline)
{
	vmd_vm *vm = new vmd_vm();
	vm->qmp = "/tmp/vmd-qmp-" + std::to_string(getpid()) + ".sock";
	vm->ppm = "/tmp/vmd-fb-" + std::to_string(getpid()) + ".ppm";
	unlink(vm->qmp.c_str());

	std::string system_img = imgdir + "/system-hibiscus-full.img";
	if (!file_exists(system_img)) {
		system_img = imgdir + "/system-pn2-full.img";
	}
	if (!file_exists(system_img)) {
		fprintf(stderr, "vmd: no system image under %s\n", imgdir.c_str());
		delete vm;
		return nullptr;
	}

	std::vector<std::string> args = {
	    "qemu-system-aarch64", "-M",    "virt", "-cpu", "max", "-smp", "4",
	    "-m", "4096",
	    "-drive", "file=" + system_img + ",format=raw,if=none,id=system",
	    "-device", "virtio-blk-device,drive=system",
	    "-netdev", "user,id=n0",
	    "-device", "virtio-net-device,netdev=n0",
	    "-device", "virtio-gpu-device",
	    "-display", "none",
	    "-qmp", "unix:" + vm->qmp + ",server=on,wait=off",
	    "-serial", "null",
	};
	std::string vendor_img = imgdir + "/vendor.img";
	if (file_exists(vendor_img)) {
		args.push_back("-drive");
		args.push_back("file=" + vendor_img + ",format=raw,if=none,id=vendor");
		args.push_back("-device");
		args.push_back("virtio-blk-device,drive=vendor");
	}
	if (!kernel.empty()) {
		args.push_back("-kernel");
		args.push_back(kernel);
	} else {
		std::string boot = imgdir + "/boot.img";
		if (file_exists(boot)) {
			args.push_back("-kernel");
			args.push_back(boot);
		}
	}
	if (!dtb.empty()) {
		args.push_back("-dtb");
		args.push_back(dtb);
	}
	if (!cmdline.empty()) {
		args.push_back("-append");
		args.push_back(cmdline);
	}

	std::vector<char *> argv;
	for (auto &a : args) {
		argv.push_back(a.data());
	}
	argv.push_back(nullptr);

	pid_t pid = fork();
	if (pid == 0) {
		int devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, STDOUT_FILENO);
		dup2(devnull, STDERR_FILENO);
		execvp(argv[0], argv.data());
		_exit(127);
	}
	if (pid < 0) {
		perror("fork");
		delete vm;
		return nullptr;
	}
	vm->pid = pid;
	fprintf(stderr, "vmd: qemu pid %d, qmp %s\n", (int)pid, vm->qmp.c_str());
	vm->poll_thread = std::thread(vmd_vm_poll, vm);
	return vm;
}

bool
vmd_vm_framebuffer(vmd_vm *vm, int *w, int *h, std::vector<uint8_t> *rgb)
{
	std::lock_guard<std::mutex> g(vm->fb_lock);
	if (vm->fb.empty()) {
		return false;
	}
	*w = vm->fb_w;
	*h = vm->fb_h;
	*rgb = vm->fb;
	return true;
}

void
vmd_vm_stop(vmd_vm *vm)
{
	if (vm == nullptr) {
		return;
	}
	vm->poll_stop.store(true);
	if (vm->poll_thread.joinable()) {
		vm->poll_thread.join();
	}
	if (vm->pid > 0) {
		kill(vm->pid, SIGTERM);
		waitpid(vm->pid, nullptr, 0);
	}
	unlink(vm->qmp.c_str());
	unlink(vm->ppm.c_str());
	delete vm;
}
