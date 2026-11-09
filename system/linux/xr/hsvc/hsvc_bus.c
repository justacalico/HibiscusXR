// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc bus client.
 * @ingroup drv_hsvc
 */

#include "hsvc_bus.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int
hsvcx_connect_path(const char *path, struct hsvcx_conn *out)
{
	int fd = hsvc_dial(path);
	if (fd < 0)
		return -1;
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	/* services send INFO on accept; tolerate a stray frame first */
	for (int i = 0; i < 8; i++) {
		if (hsvc_recv_frame(fd, &hdr, &pl) < 0) {
			close(fd);
			return -1;
		}
		if (hdr.type == HSVC_MSG_INFO)
			break;
		free(pl);
		pl = NULL;
	}
	if (pl == NULL || hdr.type != HSVC_MSG_INFO) {
		free(pl);
		close(fd);
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
