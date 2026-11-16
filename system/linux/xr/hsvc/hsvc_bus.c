// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc bus client.
 * @ingroup drv_hsvc
 */

#include "hsvc_bus.h"

#include "vmd_proto.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* services send INFO on accept; tolerate a stray frame first */
static int
hsvcx_read_info(int fd, struct hsvcx_conn *out)
{
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	for (int i = 0; i < 8; i++) {
		if (hsvc_recv_frame(fd, &hdr, &pl) < 0)
			return -1;
		if (hdr.type == HSVC_MSG_INFO)
			break;
		free(pl);
		pl = NULL;
	}
	if (pl == NULL || hdr.type != HSVC_MSG_INFO) {
		free(pl);
		return -1;
	}
	char *desc = malloc(hdr.len + 1);
	memcpy(desc, pl, hdr.len);
	desc[hdr.len] = '\0';
	free(pl);
	out->fd = fd;
	out->desc = desc;
	return 0;
}

int
hsvcx_connect_path(const char *path, struct hsvcx_conn *out)
{
	int fd = hsvc_dial(path);
	if (fd < 0)
		return -1;
	if (hsvcx_read_info(fd, out) < 0) {
		close(fd);
		return -1;
	}
	return 0;
}

int
hsvcx_connect_kind(enum hsvc_kind kind, struct hsvcx_conn *out)
{
	char *path = hsvc_resolve(kind);
	if (path == NULL)
		return -1;
	int r = hsvcx_connect_path(path, out);
	free(path);
	return r;
}

int
hsvcx_sub(struct hsvcx_conn *c, uint32_t stream)
{
	uint8_t pl[4];
	hsvc_put32(pl, stream);
	return hsvc_send_frame(c->fd, HSVC_MSG_SUB, 1, pl, sizeof(pl));
}

int
hsvcx_list(enum hsvc_kind kind, struct hsvcx_svc *out, int max)
{
	uint8_t *reply = NULL;
	uint32_t reply_len = 0;
	if (hsvc_daemon_call(HSVC_OP_LIST, NULL, 0, &reply, &reply_len) < 0 ||
	    reply == NULL)
		return 0;
	int n = 0;
	size_t i = 0;
	while (i < reply_len && n < max) {
		size_t end = i;
		while (end < reply_len && reply[end] != '\n')
			end++;
		char line[512];
		size_t l = end - i;
		if (l >= sizeof(line))
			l = sizeof(line) - 1;
		memcpy(line, reply + i, l);
		line[l] = '\0';
		char name[64], path[256];
		unsigned k;
		if (sscanf(line, "%63s %u %255s", name, &k, path) == 3 &&
		    k == (unsigned)kind) {
			snprintf(out[n].name, sizeof(out[n].name), "%s", name);
			snprintf(out[n].path, sizeof(out[n].path), "%s", path);
			n++;
		}
		i = end + 1;
	}
	free(reply);
	return n;
}

int
hsvcx_next(struct hsvcx_conn *c, uint32_t *stream, uint64_t *ts_ns,
           void *data, uint32_t cap)
{
	for (;;) {
		struct hsvc_hdr hdr;
		uint8_t *pl = NULL;
		if (hsvc_recv_frame(c->fd, &hdr, &pl) < 0)
			return -1;
		if (hdr.type == HSVC_MSG_PING) {
			hsvc_send_frame(c->fd, HSVC_MSG_PONG, hdr.seq, pl,
			                hdr.len);
			free(pl);
			continue;
		}
		/* wire prefix is packed stream:u32 + ts:u64 (12 bytes);
		 * struct hsvc_sample would read across padding */
		if (hdr.type != HSVC_MSG_STREAM || hdr.len < 12) {
			free(pl);
			continue;
		}
		uint32_t len = hdr.len - 12;
		uint32_t take = len > cap ? cap : len;
		memcpy(data, pl + 12, take);
		*stream = hsvc_get32(pl);
		*ts_ns = hsvc_get64(pl + 4);
		free(pl);
		return (int)take;
	}
}

void
hsvcx_close(struct hsvcx_conn *c)
{
	if (c->fd >= 0) {
		/* close alone doesn't wake a reader blocked in recv; the
		 * shutdown makes it return EOF so destroy joins cleanly */
		shutdown(c->fd, SHUT_RDWR);
		close(c->fd);
	}
	free(c->desc);
	c->fd = -1;
	c->desc = NULL;
}

/* one nonblocking connect with a deadline; a guest that can never
 * route to the host must fail fast, not park the prober */
static int
hsvcx_tcp_dial(const char *host, const char *port, int timeout_ms)
{
	struct addrinfo hints = {0}, *res = NULL;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		return -1;
	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd < 0) {
		freeaddrinfo(res);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
	if (connect(fd, res->ai_addr, res->ai_addrlen) != 0 &&
	    errno != EINPROGRESS) {
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
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 ||
	    err != 0) {
		close(fd);
		return -1;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
	return fd;
}

/* "host[:port]" -> host + port strings (port defaults to the service
 * endpoint's) */
static void
hsvcx_addr_split(const char *addr, char *host, size_t hlen, char *port,
                 size_t plen)
{
	snprintf(port, plen, "%d", VMD_SVC_PORT);
	const char *colon = strrchr(addr, ':');
	if (colon == NULL) {
		snprintf(host, hlen, "%s", addr);
		return;
	}
	snprintf(host, hlen, "%.*s", (int)(colon - addr), addr);
	snprintf(port, plen, "%s", colon + 1);
}

static int
hsvcx_vmd_dial_addr(const char *addr, const char *name,
                    struct hsvcx_conn *out)
{
	char host[64], port[8];
	hsvcx_addr_split(addr, host, sizeof(host), port, sizeof(port));
	int fd = hsvcx_tcp_dial(host, port, 150);
	if (fd < 0)
		return -1;
	if (hsvc_send_frame(fd, HSVC_MSG_HELLO, 1, name,
	                    (uint32_t)strlen(name)) < 0 ||
	    hsvcx_read_info(fd, out) < 0) {
		close(fd);
		return -1;
	}
	return 0;
}

/* the last address that answered, so later dials skip the dead
 * guesses */
static char vmd_addr_hit[80];

int
hsvcx_vmd_dial(const char *name, struct hsvcx_conn *out)
{
	const char *env = getenv("VMD_ADDR");
	char addrs[3][80];
	int na = 0;
	if (env != NULL && env[0] != '\0') {
		snprintf(addrs[na++], sizeof(addrs[0]), "%s", env);
	} else {
		if (vmd_addr_hit[0] != '\0')
			snprintf(addrs[na++], sizeof(addrs[0]), "%s",
			         vmd_addr_hit);
		snprintf(addrs[na++], sizeof(addrs[0]), "%s:%d",
		         VMD_PROTO_HOST, VMD_SVC_PORT);
		snprintf(addrs[na++], sizeof(addrs[0]), "127.0.0.1:%d",
		         VMD_SVC_PORT);
	}
	for (int i = 0; i < na; i++) {
		if (hsvcx_vmd_dial_addr(addrs[i], name, out) == 0) {
			snprintf(vmd_addr_hit, sizeof(vmd_addr_hit), "%s",
			         addrs[i]);
			return 0;
		}
	}
	return -1;
}
