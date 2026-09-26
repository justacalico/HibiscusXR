// Copyright 2025, PN2Lineage
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
	std::string qemu = "qemu-system-aarch64"; // VMD_QEMU or -qemu overrides
	std::string machine = "virt";    // -machine
	std::string kernel, dtb, cmdline;
};

vmd_vm *vmd_vm_start(const vmd_vm_opts &opts);
// newest guest framebuffer as RGB888, or false when nothing has arrived yet
bool vmd_vm_framebuffer(vmd_vm *vm, int *w, int *h, std::vector<uint8_t> *rgb);
void vmd_vm_stop(vmd_vm *vm);

// sim.cpp: desktop window, mouse-look + WASD drives the pose state.
int vmd_sim_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop);

// xr.cpp: OpenXR app mode - HMD pose from the active runtime (WiVRn,
// Monado) feeds the VM, VM framebuffer is submitted to the swapchains.
int vmd_xr_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop);
