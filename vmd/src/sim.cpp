// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
//
// Desktop sim: a window that shows the VM's framebuffer while the mouse
// and WASD pretend to be the headset. Drag to look (or hold RMB), WASD to
// move, Q/E roll, F toggles position tracking, Esc quits.

#include "vmd.hpp"

#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstring>


static void
quat_from_angles(float yaw, float pitch, float roll, float out[4])
{
	float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
	float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
	float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
	// YXZ order: yaw around +Y, pitch around +X, roll around +Z
	out[3] = cy * cp * cr + sy * sp * sr;
	out[0] = cy * sp * cr + sy * cp * sr;
	out[1] = sy * cp * cr - cy * sp * sr;
	out[2] = cy * cp * sr - sy * sp * cr;
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
	glfwSwapInterval(1);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float yaw = 0, pitch = 0, roll = 0;
	double lx = 0, ly = 0;
	glfwGetCursorPos(win, &lx, &ly);

	auto last = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(win) && !stop->load()) {
		glfwPollEvents();
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
			yaw -= dx * 0.004f;
			pitch -= dy * 0.004f;
			pitch = fmaxf(-1.45f, fminf(1.45f, pitch));
		}
		if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) roll += dt;
		if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) roll -= dt;
		if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

		float q[4];
		quat_from_angles(yaw, pitch, roll, q);

		// WASD moves the head in the yaw-facing plane
		float fx = 0, fz = 0;
		if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) fz -= 1;
		if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) fz += 1;
		if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) fx -= 1;
		if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) fx += 1;
		float speed = 1.7f * dt;
		float wx = (fx * cosf(yaw) - fz * sinf(yaw)) * speed;
		float wz = (fx * sinf(yaw) + fz * cosf(yaw)) * speed;

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
			float pq[4];
			memcpy(pq, state->quat, sizeof(pq));
			float pp[3];
			memcpy(pp, state->pos, sizeof(pp));
			memcpy(state->quat, q, sizeof(q));
			state->pos[0] += wx;
			state->pos[2] += wz;
			// cheap finite-diff velocities for prediction
			state->angvel[0] = 0;
			state->angvel[1] = -dx * 0.004f / fmaxf(dt, 1e-3f);
			state->angvel[2] = 0;
			state->linvel[0] = wx / fmaxf(dt, 1e-3f);
			state->linvel[2] = wz / fmaxf(dt, 1e-3f);
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
