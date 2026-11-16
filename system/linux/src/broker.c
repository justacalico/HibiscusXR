// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  The hsvcd broker loop, callable so it can be embedded:
 *         hsvcd's main is a one-liner over this.
 *         Owns the rundir, listens on daemon.sock, and answers:
 *         REGISTER, UNREGISTER, RESOLVE, LIST, WATCH. Drivers register
 *         their svc socket; the OS resolves kinds to sockets; watchers
 *         get EVENT frames as services come and go.
 */
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hsvc/hsvc.h"
#include "hsvc/registry.h"

#define MAX_CONN 64

struct conn
{
	int fd;
	int watch;
};

static struct hsvc_registry reg;
static struct conn conns[MAX_CONN];
static int nconn;
static uint64_t seq;

static void
conn_drop(int i)
{
	hsvc_reg_drop_owner(&reg, conns[i].fd);
	close(conns[i].fd);
	conns[i] = conns[--nconn];
}

static void
reply(int fd, uint32_t call, int status, const void *data, uint32_t len)
{
	uint8_t *pl = malloc(8 + (len ? len : 0));
	hsvc_put32(pl, call);
	hsvc_put32(pl + 4, (uint32_t)status);
	if (len)
		memcpy(pl + 8, data, len);
	hsvc_send_frame(fd, HSVC_MSG_REPLY, ++seq, pl, 8 + len);
	free(pl);
}

static void
event(const char *name, uint32_t kind, int up)
{
	char body[HSVC_NAME_LEN + 16];
	snprintf(body, sizeof(body), "%s %u %s\n", name, kind, up ? "up" : "down");
	for (int i = 0; i < nconn; i++) {
		if (conns[i].watch &&
		    hsvc_send_frame(conns[i].fd, HSVC_MSG_EVENT, ++seq, body,
		                    strlen(body)) < 0)
			conn_drop(i--);
	}
}

static void
handle_call(int ci, const uint8_t *pl, uint32_t len)
{
	struct conn *c = &conns[ci];
	if (len < 8)
		return;
	uint32_t call = hsvc_get32(pl);
	uint32_t op = hsvc_get32(pl + 4);
	const char *args = (const char *)pl + 8;
	uint32_t alen = len - 8;

	switch (op) {
	case HSVC_OP_REGISTER: {
		/* args: "name\nkind\npath" as text */
		char name[HSVC_NAME_LEN], path[HSVC_PATH_LEN], kb[16];
		const char *nl = memchr(args, '\n', alen);
		if (!nl || (size_t)(nl - args) >= sizeof(name))
			return reply(c->fd, call, -1, NULL, 0);
		memcpy(name, args, nl - args);
		name[nl - args] = '\0';
		const char *rest = nl + 1;
		const char *nl2 = memchr(rest, '\n', args + alen - rest);
		if (!nl2 || (size_t)(nl2 - rest) >= sizeof(kb))
			return reply(c->fd, call, -1, NULL, 0);
		memcpy(kb, rest, nl2 - rest);
		kb[nl2 - rest] = '\0';
		const char *nl3 = memchr(nl2 + 1, '\n', args + alen - nl2 - 1);
		if (nl3) {
			snprintf(path, sizeof(path), "%.*s",
			         (int)(nl3 - nl2 - 1), nl2 + 1);
		} else {
			snprintf(path, sizeof(path), "%s", nl2 + 1);
		}
		int persist = nl3 && nl3[1] == '1';
		uint32_t kind = (uint32_t)strtoul(kb, NULL, 10);
		if (hsvc_reg_add(&reg, name, kind, path,
		                 persist ? -1 : c->fd) < 0)
			return reply(c->fd, call, -1, NULL, 0);
		event(name, kind, 1);
		reply(c->fd, call, 0, NULL, 0);
		break;
	}
	case HSVC_OP_UNREGISTER: {
		char name[HSVC_NAME_LEN];
		snprintf(name, sizeof(name), "%.*s", (int)alen, args);
		if (hsvc_reg_del(&reg, name) == 0)
			event(name, 0, 0);
		reply(c->fd, call, 0, NULL, 0);
		break;
	}
	case HSVC_OP_RESOLVE: {
		uint32_t kind = (uint32_t)strtoul(args, NULL, 10);
		char path[HSVC_PATH_LEN];
		if (hsvc_reg_resolve(&reg, kind, path, sizeof(path)) < 0)
			return reply(c->fd, call, -1, NULL, 0);
		reply(c->fd, call, 0, path, strlen(path) + 1);
		break;
	}
	case HSVC_OP_LIST: {
		char *buf = malloc(HSVC_REG_MAX * (HSVC_NAME_LEN + HSVC_PATH_LEN + 16));
		int n = hsvc_reg_list(&reg, buf,
		                      HSVC_REG_MAX * (HSVC_NAME_LEN + HSVC_PATH_LEN + 16));
		if (n < 0)
			n = 0;
		reply(c->fd, call, 0, buf, n);
		free(buf);
		break;
	}
	case HSVC_OP_WATCH:
		c->watch = 1;
		reply(c->fd, call, 0, NULL, 0);
		break;
	default:
		reply(c->fd, call, -1, NULL, 0);
	}
}

int
hsvc_broker_run(void)
{
	char rundir[256], sock[512];
	snprintf(rundir, sizeof(rundir), "%s", hsvc_rundir());
	{
		char tmp[512];
		snprintf(tmp, sizeof(tmp), "%s", rundir);
		for (char *p = tmp + 1; *p; p++) {
			if (*p == '/') {
				*p = '\0';
				mkdir(tmp, 0755);
				*p = '/';
			}
		}
		mkdir(tmp, 0755);
	}
	snprintf(sock, sizeof(sock), "%s/%s", rundir, HSVC_DAEMON_SOCK);
	int lfd = hsvc_listen_at(sock);
	if (lfd < 0) {
		perror("hsvcd: listen");
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);
	hsvc_reg_init(&reg);
	struct pollfd pfds[1 + MAX_CONN];
	for (;;) {
		pfds[0].fd = lfd;
		pfds[0].events = POLLIN;
		for (int i = 0; i < nconn; i++) {
			pfds[1 + i].fd = conns[i].fd;
			pfds[1 + i].events = POLLIN;
		}
		if (poll(pfds, 1 + nconn, -1) < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (pfds[0].revents & POLLIN) {
			int c = accept(lfd, NULL, NULL);
			if (c >= 0 && nconn < MAX_CONN) {
				conns[nconn].fd = c;
				conns[nconn].watch = 0;
				nconn++;
			} else if (c >= 0) {
				close(c);
			}
		}
		for (int i = 0; i < nconn; i++) {
			if (!(pfds[1 + i].revents & (POLLIN | POLLHUP | POLLERR)))
				continue;
			struct hsvc_hdr hdr;
			uint8_t *pl = NULL;
			if (hsvc_recv_frame(conns[i].fd, &hdr, &pl) < 0) {
				conn_drop(i);
				i--;
				continue;
			}
			if (hdr.type == HSVC_MSG_CALL)
				handle_call(i, pl, hdr.len);
			else if (hdr.type == HSVC_MSG_PING)
				hsvc_send_frame(conns[i].fd, HSVC_MSG_PONG, hdr.seq,
				                NULL, 0);
			free(pl);
		}
	}
	close(lfd);
	unlink(sock);
	return 0;
}
