// Copyright 2026, Hibiscus
// SPDX-License-Identifier: BSL-1.0
//
// The hsvc service endpoint: a Linux guest's drv_hsvc dials this port
// and speaks the same frame protocol it would to a bus service. HELLO
// names the service ("vmd-hmd"/"vmd-ctrl"), INFO answers, then STREAM
// frames carry hsvc_pose / hsvc_ctrl samples. This is what replaced
// the in-guest hsvc-vmd daemon - the host tool is the whole virtual
// motion device, so the guest runs nothing extra.

#include "vmd.hpp"

#include "hsvc/proto.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>


static const char HMD_DESC[] = "kind=1\ndrv=vmd\nw=2160\nh=2160\n";
static const char CTRL_DESC[] = "kind=2\ndrv=vmd\nn=1\n";

static uint64_t
now_ns(void)
{
	return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
	           std::chrono::steady_clock::now().time_since_epoch())
	    .count();
}

static bool
send_all(int fd, const void *buf, size_t n)
{
	const char *p = (const char *)buf;
	while (n > 0) {
		ssize_t w = send(fd, p, n, MSG_NOSIGNAL);
		if (w < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		p += w;
		n -= (size_t)w;
	}
	return true;
}

static bool
send_frame(int fd, uint16_t type, uint64_t seq, const void *pl,
           uint32_t len)
{
	uint8_t h[HSVC_HDR_LEN];
	hsvc_hdr_encode(h, type, seq, len);
	return send_all(fd, h, sizeof(h)) &&
	       (len == 0 || send_all(fd, pl, len));
}

static bool
recv_all(int fd, void *buf, size_t n)
{
	char *p = (char *)buf;
	while (n > 0) {
		ssize_t r = recv(fd, p, n, 0);
		if (r <= 0) {
			if (r < 0 && errno == EINTR) {
				continue;
			}
			return false;
		}
		p += r;
		n -= (size_t)r;
	}
	return true;
}

// waits for traffic first so a wedged-but-idle peer cannot park the
// read forever; the serve loop's shutdown also unblocks the recv
static int
recv_frame(int fd, std::atomic<bool> *stop, struct hsvc_hdr *hdr,
           std::vector<uint8_t> *pl)
{
	for (;;) {
		struct pollfd pfd = {fd, POLLIN, 0};
		int pr = poll(&pfd, 1, 200);
		if (stop->load()) {
			return -1;
		}
		if (pr == 0 || (pr < 0 && errno == EINTR)) {
			continue;
		}
		if (pr < 0 || !(pfd.revents & POLLIN)) {
			return -1;
		}
		uint8_t h[HSVC_HDR_LEN];
		if (!recv_all(fd, h, sizeof(h)) || hsvc_hdr_decode(h, hdr) < 0) {
			return -1;
		}
		pl->resize(hdr->len);
		if (hdr->len > 0 && !recv_all(fd, pl->data(), hdr->len)) {
			return -1;
		}
		return 0;
	}
}

static bool
emit_hmd(int fd, vmd_pose_state *state, uint64_t seq)
{
	struct hsvc_pose p = {};
	{
		std::lock_guard<std::mutex> g(state->lock);
		p.px = state->pos[0];
		p.py = state->pos[1];
		p.pz = state->pos[2];
		p.qx = state->quat[0];
		p.qy = state->quat[1];
		p.qz = state->quat[2];
		p.qw = state->quat[3];
		p.state = state->position_valid ? 3u : 1u;
	}
	uint8_t pl[12 + sizeof(p)];
	hsvc_put32(pl, 0);
	hsvc_put64(pl + 4, now_ns());
	memcpy(pl + 12, &p, sizeof(p));
	return send_frame(fd, HSVC_MSG_STREAM, seq, pl, sizeof(pl));
}

static bool
emit_ctrl(int fd, vmd_pose_state *state, uint64_t seq)
{
	struct hsvc_ctrl c = {};
	{
		std::lock_guard<std::mutex> g(state->lock);
		c.px = state->cpos[0];
		c.py = state->cpos[1];
		c.pz = state->cpos[2];
		c.qx = state->cquat[0];
		c.qy = state->cquat[1];
		c.qz = state->cquat[2];
		c.qw = state->cquat[3];
		c.stick_x = state->cstick[0];
		c.stick_y = state->cstick[1];
		c.trigger = state->ctrigger;
		c.grip = state->cgrip;
		c.buttons = state->cbuttons;
		c.battery = 255;
		c.tracking = state->cconnected ? 1 : 0;
		c.connected = state->cconnected ? 1 : 0;
	}
	uint8_t pl[12 + sizeof(c)];
	hsvc_put32(pl, 0);
	hsvc_put64(pl + 4, now_ns());
	memcpy(pl + 12, &c, sizeof(c));
	return send_frame(fd, HSVC_MSG_STREAM, seq, pl, sizeof(pl));
}

// one client connection: HELLO picks the service, then SUB'd STREAMs
// flow until disconnect or stop
static void
svc_client(vmd_pose_state *state, std::atomic<bool> *stop, int fd)
{
	struct hsvc_hdr hdr;
	std::vector<uint8_t> pl;
	if (recv_frame(fd, stop, &hdr, &pl) < 0 ||
	    hdr.type != HSVC_MSG_HELLO) {
		close(fd);
		return;
	}
	std::string name((const char *)pl.data(), pl.size());
	int kind = 0; // 1 hmd, 2 ctrl
	const char *desc = nullptr;
	if (name == VMD_SVC_HMD) {
		kind = 1;
		desc = HMD_DESC;
	} else if (name == VMD_SVC_CTRL) {
		kind = 2;
		desc = CTRL_DESC;
	} else {
		send_frame(fd, HSVC_MSG_ERR, hdr.seq, nullptr, 0);
		close(fd);
		return;
	}
	if (!send_frame(fd, HSVC_MSG_INFO, hdr.seq, desc,
	                (uint32_t)strlen(desc))) {
		close(fd);
		return;
	}

	uint64_t seq = 0;
	bool sub = false;
	auto last = std::chrono::steady_clock::now();
	while (!stop->load()) {
		struct pollfd pfd = {fd, POLLIN, 0};
		int pr = poll(&pfd, 1, 4);
		if (pr > 0 && (pfd.revents & POLLIN)) {
			if (recv_frame(fd, stop, &hdr, &pl) < 0) {
				break;
			}
			if (hdr.type == HSVC_MSG_SUB && hdr.len >= 4) {
				sub = hsvc_get32(pl.data()) == 0;
			} else if (hdr.type == HSVC_MSG_UNSUB && hdr.len >= 4) {
				sub = hsvc_get32(pl.data()) != 0;
			} else if (hdr.type == HSVC_MSG_PING) {
				send_frame(fd, HSVC_MSG_PONG, hdr.seq,
				           pl.data(), hdr.len);
			}
		}
		if (pr > 0 && (pfd.revents & (POLLHUP | POLLERR))) {
			break;
		}
		auto now = std::chrono::steady_clock::now();
		if (sub && now - last >= std::chrono::milliseconds(10)) {
			last = now;
			bool ok = kind == 1 ? emit_hmd(fd, state, seq)
			                    : emit_ctrl(fd, state, seq);
			seq++;
			if (!ok) {
				break;
			}
		}
	}
	close(fd);
}

int
vmd_svc_serve(vmd_pose_state *state, std::atomic<bool> *stop,
              uint16_t *port)
{
	int srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) {
		perror("socket");
		return 1;
	}
	int one = 1;
	setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in a = {};
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(*port);
	if (bind(srv, (struct sockaddr *)&a, sizeof(a)) != 0 ||
	    listen(srv, 4) != 0) {
		perror("bind/listen svc");
		close(srv);
		return 1;
	}
	if (*port == 0) {
		socklen_t n = sizeof(a);
		getsockname(srv, (struct sockaddr *)&a, &n);
		*port = ntohs(a.sin_port);
	}
	fprintf(stderr, "vmd: hsvc service listening on :%d\n", (int)*port);

	std::vector<std::thread> clients;
	std::vector<int> client_fds;
	while (!stop->load()) {
		struct pollfd pfd = {srv, POLLIN, 0};
		if (poll(&pfd, 1, 200) <= 0) {
			continue;
		}
		int c = accept(srv, nullptr, nullptr);
		if (c < 0) {
			continue;
		}
		client_fds.push_back(c);
		clients.emplace_back(svc_client, state, stop, c);
	}
	close(srv);
	// a client parked in recv only wakes when its socket shuts down
	for (int fd : client_fds) {
		shutdown(fd, SHUT_RDWR);
	}
	for (auto &t : clients) {
		t.join();
	}
	return 0;
}
