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
#include <sys/utsname.h>
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
	const char *caps = "{\"execute\":\"qmp_capabilities\"}\n";
	write(fd, caps, strlen(caps));
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
			static int conn_err = 0;
			if ((conn_err++ % 10) == 0)
				fprintf(stderr, "vmd: qmp connect %s: %s\n", vm->qmp.c_str(),
				        strerror(errno));
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		std::string cmd = "{\"execute\":\"screendump\",\"arguments\":{\"filename\":\"" +
		                  vm->ppm + "\"}}\n";
		std::string rep = qmp_cmd(fd, cmd);
		close(fd);
		if (rep.find("error") != std::string::npos) {
			static bool logged_err = false;
			if (!logged_err) {
				logged_err = true;
				fprintf(stderr, "vmd: screendump error: %.160s\n", rep.c_str());
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		int w = 0, h = 0;
		std::vector<uint8_t> rgb;
		if (read_ppm(vm->ppm, &w, &h, &rgb) && w > 0 && h > 0) {
			std::lock_guard<std::mutex> g(vm->fb_lock);
			if (vm->fb.empty()) {
				fprintf(stderr, "vmd: first guest framebuffer %dx%d\n", w, h);
			}
			vm->fb_w = w;
			vm->fb_h = h;
			vm->fb.swap(rgb);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(66));
	}
}

vmd_vm *
vmd_vm_start(const vmd_vm_opts &opts)
{
	vmd_vm *vm = new vmd_vm();
	vm->qmp = "/tmp/vmd-qmp-" + std::to_string(getpid()) + ".sock";
	vm->ppm = "/tmp/vmd-fb-" + std::to_string(getpid()) + ".ppm";
	unlink(vm->qmp.c_str());

	// -img selects a guest disk set; with a bare -kernel the image is
	// optional (kernel bring-up testing, arch-agnostic qemu testing)
	std::string system_img;
	for (const char *n : {"system-hibiscus-full.img", "system-pn2-full.img",
	                      "system.img", "rootfs.img"}) {
		if (file_exists(opts.imgdir + "/" + n)) {
			system_img = opts.imgdir + "/" + n;
			break;
		}
	}
	if (system_img.empty() && opts.kernel.empty() && opts.iso.empty()) {
		fprintf(stderr, "vmd: no system image under %s and no -kernel\n",
		        opts.imgdir.c_str());
		delete vm;
		return nullptr;
	}

	// virt exposes virtio-mmio transports (-device), pc needs the pci
	// variants (-pci); the guest side is identical either way
	std::string t = opts.machine == "virt" ? "-device" : "-pci";
	std::vector<std::string> args = {opts.qemu};
	// kvm only accelerates when the guest arch matches the host cpu -
	// anything else runs under tcg and -enable-kvm would just fail
	bool want_kvm = false;
	{
		struct utsname un;
		if (uname(&un) == 0) {
			std::string h = un.machine;
			want_kvm = opts.qemu.find(h) != std::string::npos;
		}
	}
	if (want_kvm && access("/dev/kvm", W_OK) == 0) {
		args.push_back("-enable-kvm");
	}
	args.insert(args.end(), {"-M", opts.machine, "-cpu", "max", "-smp", "4",
	    "-m", "4096",
	    "-netdev", "user,id=n0",
	    "-device", "virtio-net" + t + ",netdev=n0",
	    "-device", "virtio-gpu" + t,
	    "-display", "none",
	    "-qmp", "unix:" + vm->qmp + ",server=on,wait=off",
	});
	if (!opts.serial.empty()) {
		args.push_back("-serial");
		args.push_back(opts.serial);
	} else {
		args.push_back("-serial");
		args.push_back("file:/tmp/vmd-serial-" + std::to_string(getpid()) +
		               ".log");
	}
	if (!opts.iso.empty()) {
		args.push_back("-cdrom");
		args.push_back(opts.iso);
		args.push_back("-boot");
		args.push_back("d");
	}
	if (!opts.disk.empty()) {
		// whole-disk mode: one GPT image with named partitions so the
		// guest sees /dev/vdaN + /dev/block/by-name/* like real UFS
		args.push_back("-drive");
		args.push_back("file=" + opts.disk + ",format=raw,if=none,id=disk0");
		args.push_back("-device");
		args.push_back("virtio-blk" + t + ",drive=disk0");
	} else {
		if (!system_img.empty()) {
			args.push_back("-drive");
			args.push_back("file=" + system_img +
			               ",format=raw,if=none,id=system");
			args.push_back("-device");
			args.push_back("virtio-blk" + t + ",drive=system");
		}
		std::string vendor_img = opts.imgdir + "/vendor.img";
		if (file_exists(vendor_img)) {
			args.push_back("-drive");
			args.push_back("file=" + vendor_img +
			               ",format=raw,if=none,id=vendor");
			args.push_back("-device");
			args.push_back("virtio-blk" + t + ",drive=vendor");
		}
	}
	if (!opts.kernel.empty()) {
		args.push_back("-kernel");
		args.push_back(opts.kernel);
	} else {
		std::string boot = opts.imgdir + "/boot.img";
		if (file_exists(boot)) {
			args.push_back("-kernel");
			args.push_back(boot);
		}
	}
	if (!opts.dtb.empty()) {
		args.push_back("-dtb");
		args.push_back(opts.dtb);
	}
	if (!opts.initrd.empty()) {
		args.push_back("-initrd");
		args.push_back(opts.initrd);
	}
	if (!opts.cmdline.empty()) {
		args.push_back("-append");
		args.push_back(opts.cmdline);
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
		int logfd = open("/tmp/vmd-qemu.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		dup2(logfd >= 0 ? logfd : devnull, STDERR_FILENO);
		close(0);
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
