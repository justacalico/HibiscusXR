// Copyright 2025, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// Desktop sim: a window that shows the VM's framebuffer while the mouse
// and WASD pretend to be the headset - and the wand. Drag to look (or
// hold RMB), WASD to move, wheel or Q/E roll, left click is the trigger,
// right A, middle menu, the side button or R recenters, F toggles
// position tracking, Esc quits.

#include "vmd.hpp"
#include "mousemap.hpp"

#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstring>


static double g_scroll = 0;

static void
on_scroll(GLFWwindow *, double, double y)
{
	g_scroll += y;
}

int
vmd_sim_run(vmd_vm *vm, vmd_pose_state *state, std::atomic<bool> *stop)
{
	if (!glfwInit()) {
		fprintf(stderr, "vmd: glfwInit failed\n");
		return 1;
	}
	GLFWwindow *win = glfwCreateWindow(1280, 720, "vmd desktopsim", nullptr, nullptr);
	if (!win) {
		fprintf(stderr, "vmd: window failed (no display?)\n");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(win);
	// no vsync: eglSwapBuffers waits on compositor frame callbacks, and
	// a host that stops issuing them (occluded window, stalled
	// compositor) freezes the event pump past its ping timeout - the
	// window gets flagged not-responding while the VM runs fine
	glfwSwapInterval(0);
	glfwSetScrollCallback(win, on_scroll);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	struct vmd_mm mm = {};
	static int btn_prev[4] = {};
	double lx = 0, ly = 0;
	glfwGetCursorPos(win, &lx, &ly);

	auto last = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(win) && !stop->load()) {
		// the event wait is the frame cap now that nothing throttles
		// the swap: ~60fps, input still dispatches the moment it lands
		glfwWaitEventsTimeout(1.0 / 60.0);
		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>(now - last).count();
		last = now;

		double mx, my;
		glfwGetCursorPos(win, &mx, &my);
		float dx = (float)(mx - lx), dy = (float)(my - ly);
		lx = mx;
		ly = my;
		if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ||
		    glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			vmd_mm_move(&mm, dx, dy);
		}
		vmd_mm_scroll(&mm, (float)g_scroll);
		g_scroll = 0;
		if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) mm.roll += dt;
		if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) mm.roll -= dt;
		if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) {
			mm.yaw = mm.pitch = mm.roll = 0;
		}
		if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

		for (int b = 0; b < 4; b++) {
			int down = glfwGetMouseButton(win, b) == GLFW_PRESS;
			if (down == btn_prev[b]) {
				continue;
			}
			btn_prev[b] = down;
			if (vmd_mm_button(&mm, b, down)) {
				fprintf(stderr, "vmd: recentered\n");
			}
		}

		float q[4];
		vmd_mm_quat(&mm, q);

		// WASD moves the head in the yaw-facing plane
		float fx = 0, fz = 0;
		if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) fz -= 1;
		if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) fz += 1;
		if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) fx -= 1;
		if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) fx += 1;
		float speed = 1.7f * dt;
		float wx = (fx * cosf(mm.yaw) - fz * sinf(mm.yaw)) * speed;
		float wz = (fx * sinf(mm.yaw) + fz * cosf(mm.yaw)) * speed;

		static bool f_down = false;
		if (glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS && !f_down) {
			std::lock_guard<std::mutex> g(state->lock);
			state->position_valid = !state->position_valid;
			fprintf(stderr, "vmd: position tracking %s\n",
			        state->position_valid ? "on" : "off (3dof)");
		}
		f_down = glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS;

		{
			std::lock_guard<std::mutex> g(state->lock);
			memcpy(state->quat, q, sizeof(q));
			state->pos[0] += wx;
			state->pos[2] += wz;
			// cheap finite-diff velocities for prediction
			state->angvel[0] = 0;
			state->angvel[1] = -dx * VMD_MM_SENS / fmaxf(dt, 1e-3f);
			state->angvel[2] = 0;
			state->linvel[0] = wx / fmaxf(dt, 1e-3f);
			state->linvel[2] = wz / fmaxf(dt, 1e-3f);
			// the wand follows the head so the trigger ray tracks gaze
			memcpy(state->cquat, q, sizeof(q));
			vmd_mm_wand(&mm, state->pos, state->cpos);
			state->ctrigger = mm.trigger;
			state->cgrip = mm.grip;
			state->cstick[0] = mm.stick[0];
			state->cstick[1] = mm.stick[1];
			state->cbuttons = mm.buttons;
			state->cconnected = true;
		}

		int w = 0, h = 0;
		std::vector<uint8_t> fb;
		if (vm && vmd_vm_framebuffer(vm, &w, &h, &fb)) {
			glBindTexture(GL_TEXTURE_2D, tex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
			             GL_UNSIGNED_BYTE, fb.data());
		}

		int vw, vh;
		glfwGetFramebufferSize(win, &vw, &vh);
		glViewport(0, 0, vw, vh);
		glClearColor(0.05f, 0.05f, 0.07f, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		if (w > 0) {
			// no shaders: fixed pipeline blit is plenty for a debug window
			glEnable(GL_TEXTURE_2D);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2f(-1, -1);
			glTexCoord2f(1, 1); glVertex2f(1, -1);
			glTexCoord2f(1, 0); glVertex2f(1, 1);
			glTexCoord2f(0, 0); glVertex2f(-1, 1);
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}

		glfwSwapBuffers(win);
	}
	glfwDestroyWindow(win);
	glfwTerminate();
	return 0;
}
