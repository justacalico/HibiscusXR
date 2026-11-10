// Copyright 2025, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Unit tests for the embedded guest bundle trailer and the guest.env
// manifest parser. Uses tmpfile/mkdtemp so nothing touches the real
// filesystem layout.

#include "../src/vmd.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>


static int fails = 0;

#define CHECK(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
		fails++; \
	} \
} while (0)

static FILE *
bundle_file(const void *payload, size_t n, uint64_t claim, bool magic)
{
	FILE *f = tmpfile();
	fwrite(payload, 1, n, f);
	uint8_t tail[16] = {};
	for (int i = 0; i < 8; i++) {
		tail[i] = (claim >> (8 * i)) & 0xff;
	}
	memcpy(tail + 8, magic ? "HIBGUEST" : "XXXXXXXX", 8);
	fwrite(tail, 1, 16, f);
	fflush(f);
	return f;
}

int
main()
{
	const char *payload = "guest-set-tar-bytes";
	size_t n = strlen(payload);

	uint64_t off = 0, len = 0;
	FILE *f = bundle_file(payload, n, n, true);
	CHECK(vmd_guest_probe(f, &off, &len), "probe good bundle");
	CHECK(off == 0 && len == n, "probe offsets");

	// trailing junk after the trailer is not allowed: probe only
	// reads the last 16 bytes
	f = bundle_file(payload, n, n, true);
	fseek(f, 0, SEEK_END);
	fputc('x', f);
	CHECK(!vmd_guest_probe(f, &off, &len), "reject trailing byte");

	f = bundle_file(payload, n, n, false);
	CHECK(!vmd_guest_probe(f, &off, &len), "reject bad magic");

	f = bundle_file(payload, n, n + 4096, true); // len > file - 16
	CHECK(!vmd_guest_probe(f, &off, &len), "reject oversized len");

	f = bundle_file(payload, n, 0, true);
	CHECK(!vmd_guest_probe(f, &off, &len), "reject zero len");

	f = tmpfile();
	fputs("tiny", f);
	CHECK(!vmd_guest_probe(f, &off, &len), "reject short file");
	fclose(f);

	// manifest parse
	char dir[] = "/tmp/vmd-guestenv-XXXXXX";
	CHECK(mkdtemp(dir) != nullptr, "mkdtemp");
	std::string envp = std::string(dir) + "/guest.env";
	f = fopen(envp.c_str(), "w");
	fputs("# comment\nGUEST_ARCH=aarch64\nGUEST_MACHINE=virt\n"
	      "GUEST_QEMU=qemu-system-aarch64\nGUEST_CPU_KVM=host\n"
	      "GUEST_CPU_TCG=max\nGUEST_KERNEL=vmlinuz\n"
	      "GUEST_INITRD=initramfs.img\nGUEST_ROOTFS=rootfs.img\n"
	      "GUEST_CMDLINE=console=tty0 console=ttyAMA0 root=/dev/vda rw\n",
	      f);
	fclose(f);

	vmd_guest_env env;
	CHECK(vmd_guest_env_read(dir, &env), "env read");
	CHECK(env.arch == "aarch64", "env arch");
	CHECK(env.machine == "virt", "env machine");
	CHECK(env.qemu == "qemu-system-aarch64", "env qemu");
	CHECK(env.cpu_kvm == "host" && env.cpu_tcg == "max", "env cpus");
	CHECK(env.kernel == "vmlinuz" && env.initrd == "initramfs.img" &&
	      env.rootfs == "rootfs.img", "env files");
	CHECK(env.cmdline == "console=tty0 console=ttyAMA0 root=/dev/vda rw",
	      "env cmdline keeps spaces");
	CHECK(!vmd_guest_env_read("/nonexistent", &env), "env missing");
	vmd_guest_rmtree(dir);

	// PATH resolution: a private PATH dir with one executable and one
	// non-executable entry covers hit, miss, X_OK and absolute paths
	char pdir[] = "/tmp/vmd-which-XXXXXX";
	CHECK(mkdtemp(pdir) != nullptr, "mkdtemp which");
	std::string tool = std::string(pdir) + "/fakeqemu";
	f = fopen(tool.c_str(), "w");
	fputs("#!/bin/sh\n", f);
	fclose(f);
	CHECK(chmod(tool.c_str(), 0755) == 0, "chmod tool");
	std::string noexec = std::string(pdir) + "/noexec";
	f = fopen(noexec.c_str(), "w");
	fputs("x", f);
	fclose(f);
	CHECK(chmod(noexec.c_str(), 0644) == 0, "chmod noexec");

	std::string oldpath = getenv("PATH") ? getenv("PATH") : "";
	setenv("PATH", pdir, 1);
	CHECK(vmd_which("fakeqemu") == tool, "which hits PATH");
	CHECK(vmd_which("noexec").empty(), "which skips non-executable");
	CHECK(vmd_which("nothere").empty(), "which misses PATH");
	setenv("PATH", oldpath.c_str(), 1);
	CHECK(vmd_which(tool) == tool, "which absolute executable");
	CHECK(vmd_which(noexec).empty(), "which absolute non-executable");
	CHECK(vmd_which("").empty(), "which empty");
	vmd_guest_rmtree(pdir);

	if (fails == 0) {
		printf("test_guest: all passed\n");
	}
	return fails ? 1 : 0;
}
