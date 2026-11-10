// Copyright 2025, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// QEMU launcher + framebuffer reader.
//
// The guest gets raw ext4 partitions on virtio-blk (system, vendor,
// userdata), user networking (which is what gives the guest its
// 10.0.2.2 route back to us), and a virtio-gpu whose framebuffer we
// pull through QMP screendump.
//
// Two image layouts are picked up automatically under -img (or from
// the guest set embedded in a dist binary, see src/guest.cpp):
//
//   Linux guest set (vmd/vm/make-guest.sh): vmlinuz + initramfs.img +
//     rootfs.img + guest.env, which names arch/machine/cpu/cmdline -
//     a Fedora/systemd rootfs with the distro layered on top.
//   Android tree (bootenv/make-bootenv.sh): kernel-gki + the virtual-
//     device module set in vmd-boot.img plus vendor.img/userdata.img,
//     since the stock sdm845 kernel cannot run on qemu's virt machine.
//
// -kernel/-initrd/-append/-cpu/-machine/-qemu still override.

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
	std::string qlog;   // qemu stderr log
	std::string gtmp;   // extracted embedded guest dir, or empty
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

// the distro package most likely to ship a given qemu-system-* binary
static const char *
qemu_pkg_hint(const std::string &qemu)
{
	if (qemu.find("aarch64") != std::string::npos ||
	    qemu.find("-arm") != std::string::npos) {
		return "qemu-system-aarch64 (dnf) or qemu-system-arm (apt)";
	}
	if (qemu.find("x86_64") != std::string::npos ||
	    qemu.find("i386") != std::string::npos) {
		return "qemu-system-x86";
	}
	return "your distro's qemu system emulator";
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
	    fscanf(f, "%d %d %d", w, h, &maxv) != 3 || *w <= 0 || *h <= 0 ||
	    *w > 16384 || *h > 16384 || maxv != 255 || fgetc(f) == EOF) {
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
		// a qemu that died at startup never opens the qmp socket -
		// say so once instead of logging connect failures forever
		int st = 0;
		if (vm->pid > 0 && waitpid(vm->pid, &st, WNOHANG) == vm->pid) {
			if (WIFEXITED(st)) {
				fprintf(stderr, "vmd: qemu exited with status %d "
				        "- see %s\n", WEXITSTATUS(st), vm->qlog.c_str());
			} else {
				fprintf(stderr, "vmd: qemu died on signal %d "
				        "- see %s\n", WTERMSIG(st), vm->qlog.c_str());
			}
			vm->pid = -1;
			break;
		}
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
	vm->qlog = "/tmp/vmd-qemu-" + std::to_string(getpid()) + ".log";
	unlink(vm->qmp.c_str());

	// -img selects a guest disk set; with a bare -kernel the image is
	// optional (kernel bring-up testing, arch-agnostic qemu testing).
	// A dir with a guest.env manifest is a Hibiscus Linux guest set
	// (vmd/vm/make-guest.sh); a dist binary with nothing under -img
	// falls back to the guest set embedded at its own EOF.
	std::string imgdir = opts.imgdir;
	vmd_guest_env env;
	bool linux = vmd_guest_env_read(imgdir, &env);

	auto pick = [&](const char *const *names) -> std::string {
		for (; *names; names++) {
			std::string p = imgdir + "/" + *names;
			if (file_exists(p)) {
				return p;
			}
		}
		return "";
	};
	static const char *const imgs[] = {
	    "system-hibiscus-vmd.img", "system-hibiscus-full-neo2.img",
	    "system-hibiscus-full.img", "system-pn2-full.img",
	    "system.img", "rootfs.img", nullptr};
	static const char *const kernels[] = {
	    "vmlinuz", "vmlinuz-virt", "kernel-virt", "kernel-gki",
	    "kernel-ranchu", "kernel", "boot.img", nullptr};
	static const char *const initrds[] = {
	    "initramfs.img", "initramfs", "vmd-boot.img", "ramdisk.img",
	    nullptr};

	std::string system_img;
	if (!env.rootfs.empty() && file_exists(imgdir + "/" + env.rootfs)) {
		system_img = imgdir + "/" + env.rootfs;
	} else {
		system_img = pick(imgs);
	}
	bool needs_vm_files = opts.kernel.empty() && opts.iso.empty() &&
	                      opts.disk.empty();
	if (system_img.empty() && needs_vm_files && !linux) {
		if (vmd_guest_extract(&vm->gtmp) == 0) {
			imgdir = vm->gtmp;
			linux = vmd_guest_env_read(imgdir, &env);
			if (!env.rootfs.empty() &&
			    file_exists(imgdir + "/" + env.rootfs)) {
				system_img = imgdir + "/" + env.rootfs;
			} else {
				system_img = pick(imgs);
			}
		}
	}
	if (system_img.empty() && needs_vm_files) {
		uint64_t bo = 0, bl = 0;
		fprintf(stderr, "vmd: no system image under %s and no -kernel%s\n",
		        opts.imgdir.c_str(),
		        vmd_guest_probe_self(&bo, &bl)
		            ? " (embedded guest set is unreadable)" : "");
		vmd_guest_rmtree(vm->gtmp.c_str());
		delete vm;
		return nullptr;
	}
	if (linux) {
		// a manifest that names files that are not there boots
		// nothing - fail now and say which ones
		bool bad = false;
		auto want = [&](const std::string &f, const char *what) {
			if (!f.empty() && !file_exists(imgdir + "/" + f)) {
				fprintf(stderr, "vmd: guest.env %s '%s' is missing "
				        "under %s\n", what, f.c_str(), imgdir.c_str());
				bad = true;
			}
		};
		want(env.kernel, "kernel");
		want(env.initrd, "initrd");
		want(env.rootfs, "rootfs");
		if (bad) {
			vmd_guest_rmtree(vm->gtmp.c_str());
			delete vm;
			return nullptr;
		}
	}

	std::string qemu = opts.qemu;
	if (qemu.empty() && !env.qemu.empty()) {
		qemu = env.qemu;
	}
	if (qemu.empty()) {
		qemu = "qemu-system-aarch64";
	}
	std::string qemu_bin = vmd_which(qemu);
	if (qemu_bin.empty()) {
		// the #1 black-window cause: exec fails after stderr was
		// already redirected, so nothing ever said why
		fprintf(stderr, "vmd: qemu binary '%s' is not installed - install "
		        "%s or pass -qemu <path>\n", qemu.c_str(),
		        qemu_pkg_hint(qemu));
		vmd_guest_rmtree(vm->gtmp.c_str());
		delete vm;
		return nullptr;
	}
	std::string machine = !opts.machine.empty() ? opts.machine
	                    : !env.machine.empty()   ? env.machine
	                                             : "virt";

	// virt exposes virtio-mmio transports (-device) which the GKI module
	// set needed; the linux-virt kernel only has the pci transport, so a
	// linux guest set gets -pci devices on either machine type
	std::string t = machine == "virt" && !linux ? "-device" : "-pci";
	std::vector<std::string> args = {qemu};
	// kvm only accelerates when the guest arch matches the host cpu -
	// anything else runs under tcg and -enable-kvm would just fail
	bool want_kvm = false;
	{
		struct utsname un;
		if (uname(&un) == 0) {
			std::string h = un.machine;
			want_kvm = qemu.find(h) != std::string::npos;
		}
	}
	bool use_kvm = want_kvm && access("/dev/kvm", W_OK) == 0;
	if (use_kvm) {
		args.push_back("-enable-kvm");
	}
	// explicit -cpu wins; else the manifest's kvm/tcg pair; else the
	// cortex-a53 the ranchu kernels needed ('max' hung them)
	std::string cpu = opts.cpu;
	if (cpu.empty()) {
		cpu = use_kvm && !env.cpu_kvm.empty() ? env.cpu_kvm
		    : !use_kvm && !env.cpu_tcg.empty() ? env.cpu_tcg
		    : "cortex-a53";
	}
	// user networking: the guest reaches us at 10.0.2.2 (pose channel,
	// adb server lookup). hostfwd exposes the guest's own services back:
	// 15555->adbd once persist.hibiscus.adbwifi=1, 17340->cted when enabled.
	args.insert(args.end(), {"-M", machine, "-cpu", cpu, "-smp", "4",
	    "-m", "4096",
	    "-netdev", "user,id=n0,hostfwd=tcp:127.0.0.1:15555-:5555,"
	               "hostfwd=tcp:127.0.0.1:17340-:7340",
	    "-device", "virtio-net" + t + ",netdev=n0",
	    // GPU goes on PCI even on virt: the GKI virtio_gpu module won't
	    // bind an mmio transport (probe fails with no error), while the
	    // pci variant registers /dev/dri/card0 correctly on both.
	    "-device", "virtio-gpu-pci,xres=1280,yres=800,edid=off",
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
		std::string vendor_img = imgdir + "/vendor.img";
		if (file_exists(vendor_img)) {
			args.push_back("-drive");
			args.push_back("file=" + vendor_img +
			               ",format=raw,if=none,id=vendor");
			args.push_back("-device");
			args.push_back("virtio-blk" + t + ",drive=vendor");
		}
		std::string userdata_img = imgdir + "/userdata.img";
		if (file_exists(userdata_img)) {
			args.push_back("-drive");
			args.push_back("file=" + userdata_img +
			               ",format=raw,if=none,id=userdata");
			args.push_back("-device");
			args.push_back("virtio-blk" + t + ",drive=userdata");
		}
	}
	if (linux) {
		// the guest's evdev nodes, for profiles that drive the headset
		// off a real mouse (devices/headset profiles over evdev)
		args.push_back("-device");
		args.push_back("virtio-keyboard" + t);
		args.push_back("-device");
		args.push_back("virtio-tablet" + t);
	}
	if (!opts.kernel.empty()) {
		args.push_back("-kernel");
		args.push_back(opts.kernel);
	} else {
		std::string k;
		if (!env.kernel.empty() && file_exists(imgdir + "/" + env.kernel)) {
			k = imgdir + "/" + env.kernel;
		} else {
			// bootenv/make-bootenv.sh drops the GKI kernel next to the
			// system image; boot.img stays the last-resort fallback
			k = pick(kernels);
		}
		if (!k.empty()) {
			args.push_back("-kernel");
			args.push_back(k);
		} else if (linux) {
			fprintf(stderr, "vmd: no kernel found under %s - qemu will "
			        "boot to an empty SeaBIOS screen\n", imgdir.c_str());
		}
	}
	if (!opts.dtb.empty()) {
		args.push_back("-dtb");
		args.push_back(opts.dtb);
	}
	{
		std::string initrd = opts.initrd;
		if (initrd.empty() && !env.initrd.empty() &&
		    file_exists(imgdir + "/" + env.initrd)) {
			initrd = imgdir + "/" + env.initrd;
		}
		if (initrd.empty()) {
			initrd = pick(initrds);
		}
		if (!initrd.empty()) {
			args.push_back("-initrd");
			args.push_back(initrd);
		} else if (linux) {
			// the guest kernel needs the initramfs to load virtio_blk
			// before root=/dev/vda can mount - a bare vmlinuz panics
			fprintf(stderr, "vmd: no initramfs found under %s - the "
			        "guest kernel cannot mount its rootfs\n",
			        imgdir.c_str());
		}
	}
	std::string cmdline = opts.cmdline;
	if (cmdline.empty()) {
		cmdline = env.cmdline;
	}
	if (cmdline.empty() && !system_img.empty() && !linux) {
		// defaults for the Android tree: serial console (also the
		// guest shell via androidboot.console), permissive selinux so
		// HALs started under the emulator vendor image don't wedge
		cmdline = "console=ttyAMA0 androidboot.console=ttyAMA0 "
		          "androidboot.hardware=ranchu qemu=1 "
		          "androidboot.selinux=permissive";
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
		int logfd = open(vm->qlog.c_str(),
		                 O_WRONLY | O_CREAT | O_TRUNC, 0644);
		dup2(logfd >= 0 ? logfd : devnull, STDERR_FILENO);
		close(0);
		execv(qemu_bin.c_str(), argv.data());
		dprintf(STDERR_FILENO, "vmd: exec %s: %s\n", qemu_bin.c_str(),
		        strerror(errno));
		_exit(127);
	}
	if (pid < 0) {
		perror("fork");
		vmd_guest_rmtree(vm->gtmp.c_str());
		delete vm;
		return nullptr;
	}
	vm->pid = pid;
	fprintf(stderr, "vmd: qemu pid %d, qmp %s, log %s\n", (int)pid,
	        vm->qmp.c_str(), vm->qlog.c_str());
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
	vmd_guest_rmtree(vm->gtmp.c_str());
	delete vm;
}
