// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
//
// Pose channel server: the guest's vmd driver connects here (qemu user
// networking maps 10.0.2.2 to the host) and we stream head poses down.

#include "vmd.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>


int
vmd_pose_serve(vmd_pose_state *state, std::atomic<bool> *stop)
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
	a.sin_port = htons(VMD_PROTO_PORT);
	if (bind(srv, (struct sockaddr *)&a, sizeof(a)) != 0 || listen(srv, 2) != 0) {
		perror("bind/listen 7781");
		close(srv);
		return 1;
	}
	fprintf(stderr, "vmd: pose channel listening on :%d\n", VMD_PROTO_PORT);

	uint32_t seq = 0;
	while (!stop->load()) {
		struct pollfd pfd = {srv, POLLIN, 0};
		if (poll(&pfd, 1, 200) <= 0) {
			continue;
		}
		int c = accept(srv, nullptr, nullptr);
		if (c < 0) {
			continue;
		}
		fprintf(stderr, "vmd: guest connected\n");
		while (!stop->load()) {
			vmd_pose_packet pkt = {};
			{
				std::lock_guard<std::mutex> g(state->lock);
				pkt.magic = VMD_PROTO_MAGIC;
				pkt.version = VMD_PROTO_VERSION;
				pkt.flags = state->position_valid
				                ? (VMD_POSE_POSITION_VALID | VMD_POSE_TRACKED)
				                : VMD_POSE_TRACKED;
				pkt.seq = seq++;
				pkt.host_ts_ns =
				    std::chrono::duration_cast<std::chrono::nanoseconds>(
				        std::chrono::steady_clock::now().time_since_epoch())
				        .count();
				memcpy(pkt.quat, state->quat, sizeof(pkt.quat));
				memcpy(pkt.pos, state->pos, sizeof(pkt.pos));
				memcpy(pkt.linvel, state->linvel, sizeof(pkt.linvel));
				memcpy(pkt.angvel, state->angvel, sizeof(pkt.angvel));
			}
			ssize_t w = send(c, &pkt, sizeof(pkt), MSG_NOSIGNAL);
			if (w != (ssize_t)sizeof(pkt)) {
				fprintf(stderr, "vmd: guest disconnected\n");
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(14));
		}
		close(c);
	}
	close(srv);
	return 0;
}
