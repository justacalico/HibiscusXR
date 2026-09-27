// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Reconnecting read side of the vmd pose channel.
 * @ingroup drv_vmd
 */

#include "vmd_chan.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <poll.h>


struct vmd_chan
{
	char host[64];
	char port[8];
	int fd;
};

static void
vmd_chan_split(const char *addr, char *host, size_t hlen, char *port, size_t plen)
{
	snprintf(host, hlen, "%s", VMD_PROTO_HOST);
	snprintf(port, plen, "%d", VMD_PROTO_PORT);
	if (addr == NULL || addr[0] == '\0') {
		return;
	}
	const char *colon = strrchr(addr, ':');
	if (colon == NULL) {
		snprintf(host, hlen, "%s", addr);
		return;
	}
	snprintf(host, hlen, "%.*s", (int)(colon - addr), addr);
	snprintf(port, plen, "%s", colon + 1);
}

// one connect attempt; @p timeout_ms bounds the wait so a probe against
// hardware that can never route to the host still returns quickly
static int
vmd_chan_connect(const char *host, const char *port, int timeout_ms)
{
	struct addrinfo hints = {0}, *res = NULL;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hints, &res) != 0) {
		return -1;
	}
	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd < 0) {
		freeaddrinfo(res);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
	if (connect(fd, res->ai_addr, res->ai_addrlen) != 0 && errno != EINPROGRESS) {
		freeaddrinfo(res);
		close(fd);
		return -1;
	}
	freeaddrinfo(res);
	struct pollfd pfd = {fd, POLLOUT, 0};
	if (poll(&pfd, 1, timeout_ms) <= 0 || !(pfd.revents & POLLOUT)) {
		close(fd);
		return -1;
	}
	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0) {
		close(fd);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
	return fd;
}

bool
vmd_chan_probe(const char *addr)
{
	char host[64], port[8];
	vmd_chan_split(addr, host, sizeof(host), port, sizeof(port));
	int fd = vmd_chan_connect(host, port, 150);
	if (fd < 0) {
		return false;
	}
	close(fd);
	return true;
}

struct vmd_chan *
vmd_chan_open(const char *addr)
{
	struct vmd_chan *c = calloc(1, sizeof(*c));
	if (c == NULL) {
		return NULL;
	}
	vmd_chan_split(addr, c->host, sizeof(c->host), c->port, sizeof(c->port));
	c->fd = -1;
	return c;
}

bool
vmd_chan_read(struct vmd_chan *c, struct vmd_pose_packet *out, volatile bool *stop)
{
	while (!*stop) {
		if (c->fd < 0) {
			c->fd = vmd_chan_connect(c->host, c->port, 2000);
			if (c->fd < 0) {
				// host not up yet (or VM booted before vmd); retry
				struct pollfd z = {-1, 0, 0};
				poll(&z, 0, 500);
				continue;
			}
		}
		ssize_t got = 0;
		char *p = (char *)out;
		while (got < (ssize_t)sizeof(*out)) {
			ssize_t r = recv(c->fd, p + got, sizeof(*out) - got, 0);
			if (r > 0) {
				got += r;
				continue;
			}
			if (r < 0 && errno == EINTR) {
				continue;
			}
			close(c->fd);
			c->fd = -1;
			break;
		}
		if (got < (ssize_t)sizeof(*out)) {
			continue;
		}
		if (out->magic != VMD_PROTO_MAGIC || out->version != VMD_PROTO_VERSION) {
			// out of sync: drop the connection, resync on reconnect
			close(c->fd);
			c->fd = -1;
			continue;
		}
		return true;
	}
	return false;
}

void
vmd_chan_close(struct vmd_chan *c)
{
	if (c == NULL) {
		return;
	}
	if (c->fd >= 0) {
		close(c->fd);
	}
	free(c);
}
