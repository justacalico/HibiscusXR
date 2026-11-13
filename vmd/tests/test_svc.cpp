// vmd_svc_serve wire test: a loopback client speaks the hsvc frame
// protocol the way a guest's drv_hsvc would - HELLO names the service,
// INFO answers, SUB'd STREAMs then carry hsvc_pose / hsvc_ctrl.

#include "vmd.hpp"

#include "hsvc/proto.h"

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>


static void
send_all(int fd, const void *buf, size_t n)
{
	const char *p = (const char *)buf;
	while (n > 0) {
		ssize_t w = send(fd, p, n, 0);
		assert(w > 0);
		p += w;
		n -= (size_t)w;
	}
}

static void
send_frame(int fd, uint16_t type, const void *pl, uint32_t len)
{
	uint8_t h[HSVC_HDR_LEN];
	hsvc_hdr_encode(h, type, 1, len);
	send_all(fd, h, sizeof(h));
	if (len) {
		send_all(fd, pl, len);
	}
}

static void
recv_all(int fd, void *buf, size_t n)
{
	char *p = (char *)buf;
	while (n > 0) {
		ssize_t r = recv(fd, p, n, 0);
		assert(r > 0);
		p += r;
		n -= (size_t)r;
	}
}

static uint16_t
recv_frame(int fd, std::vector<uint8_t> *pl)
{
	uint8_t h[HSVC_HDR_LEN];
	recv_all(fd, h, sizeof(h));
	struct hsvc_hdr hdr;
	assert(hsvc_hdr_decode(h, &hdr) == 0);
	pl->resize(hdr.len);
	if (hdr.len) {
		recv_all(fd, pl->data(), hdr.len);
	}
	return hdr.type;
}

static int
dial(uint16_t port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd >= 0);
	struct sockaddr_in a = {};
	a.sin_family = AF_INET;
	a.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
	assert(connect(fd, (struct sockaddr *)&a, sizeof(a)) == 0);
	return fd;
}

// HELLO + INFO for one service, returns the descriptor text
static std::string
connect_svc(int fd, const char *name)
{
	send_frame(fd, HSVC_MSG_HELLO, name, (uint32_t)strlen(name));
	std::vector<uint8_t> pl;
	assert(recv_frame(fd, &pl) == HSVC_MSG_INFO);
	return std::string((const char *)pl.data(), pl.size());
}

int
main(void)
{
	vmd_pose_state state;
	{
		std::lock_guard<std::mutex> g(state.lock);
		state.quat[1] = 0.5f;
		state.pos[1] = 1.7f;
		state.position_valid = false;
		state.ctrigger = 1.0f;
		state.cbuttons = VMD_BTN_A | VMD_BTN_TRIG;
		state.cpos[2] = -0.35f;
		state.cconnected = true;
	}
	std::atomic<bool> stop{false};
	uint16_t port = 0;
	std::thread srv(vmd_svc_serve, &state, &stop, &port);
	while (port == 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	int hmd = dial(port);
	std::string desc = connect_svc(hmd, VMD_SVC_HMD);
	assert(desc.find("kind=1") != std::string::npos);
	assert(desc.find("w=2160") != std::string::npos);

	uint8_t s4[4];
	hsvc_put32(s4, 0);
	send_frame(hmd, HSVC_MSG_SUB, s4, sizeof(s4));

	std::vector<uint8_t> pl;
	struct hsvc_pose pose = {};
	for (int i = 0; i < 50; i++) {
		assert(recv_frame(hmd, &pl) == HSVC_MSG_STREAM);
		if (pl.size() != 12 + sizeof(pose)) {
			continue;
		}
		memcpy(&pose, pl.data() + 12, sizeof(pose));
		if (pose.state != 0) {
			break;
		}
	}
	assert(pose.qy == 0.5f && pose.py == 1.7f && pose.state == 1);

	int ctrl = dial(port);
	desc = connect_svc(ctrl, VMD_SVC_CTRL);
	assert(desc.find("kind=2") != std::string::npos);
	send_frame(ctrl, HSVC_MSG_SUB, s4, sizeof(s4));
	struct hsvc_ctrl c = {};
	for (int i = 0; i < 50; i++) {
		assert(recv_frame(ctrl, &pl) == HSVC_MSG_STREAM);
		if (pl.size() != 12 + sizeof(c)) {
			continue;
		}
		memcpy(&c, pl.data() + 12, sizeof(c));
		break;
	}
	assert(c.trigger == 1.0f);
	assert((c.buttons & VMD_BTN_A) && (c.buttons & VMD_BTN_TRIG));
	assert(c.pz == -0.35f && c.connected == 1 && c.tracking == 1);

	send_frame(ctrl, HSVC_MSG_PING, "ab", 2);
	for (int i = 0; i < 50 && recv_frame(ctrl, &pl) != HSVC_MSG_PONG; i++) {
		assert(i < 49);
	}

	int bad = dial(port);
	send_frame(bad, HSVC_MSG_HELLO, "nope", 4);
	(void)recv_frame(bad, &pl); // ERR or close - either is a refuse

	close(hmd);
	close(ctrl);
	close(bad);
	stop.store(true);
	srv.join();
	printf("svc test passed\n");
	return 0;
}
