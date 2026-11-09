// Copyright 2025, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Guest set handling for the Linux image: the guest.env manifest an
// image dir carries, and the embedded guest bundle a dist binary
// appends to itself (<tar><u64le size><"HIBGUEST"> at EOF).

#include "vmd.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ftw.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


static const char VMD_GUEST_MAGIC[8] = {'H','I','B','G','U','E','S','T'};

bool
vmd_guest_probe(FILE *f, uint64_t *off, uint64_t *len)
{
	if (fseek(f, 0, SEEK_END) != 0) {
		return false;
	}
	long size = ftell(f);
	if (size < 16 || fseek(f, -16L, SEEK_END) != 0) {
		return false;
	}
	uint8_t tail[16];
	if (fread(tail, 1, sizeof(tail), f) != sizeof(tail)) {
		return false;
	}
	if (memcmp(tail + 8, VMD_GUEST_MAGIC, 8) != 0) {
		return false;
	}
	uint64_t n = 0;
	for (int i = 0; i < 8; i++) {
		n |= (uint64_t)tail[i] << (8 * i);
	}
	if (n == 0 || n > (uint64_t)size - 16) {
		return false;
	}
	*off = (uint64_t)size - 16 - n;
	*len = n;
	return true;
}

bool
vmd_guest_probe_self(uint64_t *off, uint64_t *len)
{
	FILE *f = fopen("/proc/self/exe", "rb");
	if (!f) {
		return false;
	}
	bool ok = vmd_guest_probe(f, off, len);
	fclose(f);
	return ok;
}

bool
vmd_guest_env_read(const std::string &dir, vmd_guest_env *env)
{
	FILE *f = fopen((dir + "/guest.env").c_str(), "r");
	if (!f) {
		return false;
	}
	char line[1024];
	while (fgets(line, sizeof(line), f)) {
		char *eq = strchr(line, '=');
		if (!eq || line[0] == '#') {
			continue;
		}
		*eq++ = 0;
		char *nl = strpbrk(eq, "\r\n");
		if (nl) *nl = 0;
		std::string key = line, val = eq;
		if (key == "GUEST_ARCH") env->arch = val;
		else if (key == "GUEST_QEMU") env->qemu = val;
		else if (key == "GUEST_MACHINE") env->machine = val;
		else if (key == "GUEST_CPU_KVM") env->cpu_kvm = val;
		else if (key == "GUEST_CPU_TCG") env->cpu_tcg = val;
		else if (key == "GUEST_KERNEL") env->kernel = val;
		else if (key == "GUEST_INITRD") env->initrd = val;
		else if (key == "GUEST_ROOTFS") env->rootfs = val;
		else if (key == "GUEST_CMDLINE") env->cmdline = val;
	}
	fclose(f);
	return !env->arch.empty();
}

// copy <len> bytes of the bundle out of the binary into a plain file,
// then untar it next to it - tar can unpack every format we produce
static int
vmd_guest_extract_to(const std::string &dir)
{
	uint64_t off = 0, len = 0;
	if (!vmd_guest_probe_self(&off, &len)) {
		return -1;
	}
	FILE *self = fopen("/proc/self/exe", "rb");
	std::string tpath = dir + "/bundle.tar";
	FILE *out = fopen(tpath.c_str(), "wb");
	if (!self || !out || fseek(self, (long)off, SEEK_SET) != 0) {
		if (self) fclose(self);
		if (out) fclose(out);
		return -1;
	}
	char buf[1 << 20];
	uint64_t left = len;
	bool io = true;
	while (left && io) {
		size_t want = left < sizeof(buf) ? (size_t)left : sizeof(buf);
		size_t got = fread(buf, 1, want, self);
		io = got && fwrite(buf, 1, got, out) == got;
		left -= got;
	}
	fclose(self);
	int bad = ferror(out) || left;
	if (fclose(out) != 0) bad = 1;
	if (bad) {
		unlink(tpath.c_str());
		return -1;
	}

	pid_t pid = fork();
	if (pid == 0) {
		execlp("tar", "tar", "-xf", tpath.c_str(), "-C", dir.c_str(),
		       (char *)nullptr);
		_exit(127);
	}
	int st = 0;
	if (pid <= 0 || waitpid(pid, &st, 0) < 0) {
		unlink(tpath.c_str());
		return -1;
	}
	unlink(tpath.c_str());
	return (WIFEXITED(st) && WEXITSTATUS(st) == 0) ? 0 : -1;
}

int
vmd_guest_extract(std::string *dir_out)
{
	uint64_t off = 0, len = 0;
	if (!vmd_guest_probe_self(&off, &len)) {
		return -1;
	}
	// rootfs images are big - stage on real disk, not tmpfs /tmp
	const char *base = getenv("VMD_GUEST_TMPDIR");
	if (!base || !*base) {
		base = "/var/tmp";
	}
	std::string dir = std::string(base) + "/vmd-guest-XXXXXX";
	if (!mkdtemp(dir.data())) {
		return -1;
	}
	if (vmd_guest_extract_to(dir) != 0) {
		vmd_guest_rmtree(dir.c_str());
		return -1;
	}
	*dir_out = dir;
	fprintf(stderr, "vmd: unpacked embedded guest set (%llu bytes) to %s\n",
	        (unsigned long long)len, dir.c_str());
	return 0;
}

static int
rm_cb(const char *p, const struct stat *, int, struct FTW *)
{
	remove(p);
	return 0;
}

void
vmd_guest_rmtree(const char *dir)
{
	if (dir && *dir) {
		nftw(dir, rm_cb, 16, FTW_DEPTH | FTW_PHYS);
	}
}
