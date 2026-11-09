// Copyright 2025, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Shared declarations for the vmd host tool.

#pragma once

#include "vmd_proto.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>


struct vmd_pose_state
{
	std::mutex lock;
	float quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float pos[3] = {0.0f, 1.6f, 0.0f};
	float angvel[3] = {0.0f, 0.0f, 0.0f};
	float linvel[3] = {0.0f, 0.0f, 0.0f};
	bool position_valid = true;
};

// pose.cpp: TCP server on VMD_PROTO_PORT, streams packets at ~72Hz to
// whatever guest connects. Runs until stop is set.
int vmd_pose_serve(vmd_pose_state *state, std::atomic<bool> *stop);

// vm.cpp: qemu launcher + QMP screendump framebuffer reader.
struct vmd_vm;

struct vmd_vm_opts
{
	std::string imgdir;              // dir holding the image set
	std::string qemu;                // VMD_QEMU or -qemu; else the
	                                 // guest.env value or the old
	                                 // aarch64 default
	std::string machine;             // -machine
	std::string cpu;                 // -cpu
	std::string kernel, dtb, cmdline;
	std::string initrd;              // -initrd: guest ramdisk image
	std::string disk;                // -disk: single GPT disk image
	std::string serial;              // -serial: null|stdio|file:<path>
	std::string iso;                 // -iso: attach as -cdrom + -boot d
};

// guest.cpp: the Linux guest set - manifest parsing and the embedded
// bundle a dist binary carries at EOF.
struct vmd_guest_env
{
	std::string arch, qemu, machine, cpu_kvm, cpu_tcg;
	std::string kernel, initrd, rootfs, cmdline;
};

// probe a file for the appended bundle trailer: <tar><u64le><HIBGUEST>
bool vmd_guest_probe(FILE *f, uint64_t *off, uint64_t *len);
bool vmd_guest_probe_self(uint64_t *off, uint64_t *len);
// parse <dir>/guest.env; false when missing or unusable
bool vmd_guest_env_read(const std::string &dir, vmd_guest_env *env);
// extract this binary's embedded guest set into a fresh dir (caller
// frees with vmd_guest_rmtree); -1 when there is no bundle or it fails
int vmd_guest_extract(std::string *dir_out);
void vmd_guest_rmtree(const char *dir);

vmd_vm *vmd_vm_start(const vmd_vm_opts &opts);
// newest guest framebuffer as RGB888, or false when nothing has arrived yet
bool vmd_vm_framebuffer(vmd_vm *vm, int *w, int *h, std::vector<uint8_t> *rgb);
void vmd_vm_stop(vmd_vm *vm);

// sim.cpp: desktop window, mouse-look + WASD drives the pose state.
int vmd_sim_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop);

// xr.cpp: OpenXR app mode - HMD pose from the active runtime (WiVRn,
// Monado) feeds the VM, VM framebuffer is submitted to the swapchains.
int vmd_xr_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop);
