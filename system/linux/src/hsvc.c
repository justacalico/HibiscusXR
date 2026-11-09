// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file libhsvc - see include/hsvc/hsvc.h */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "hsvc/hsvc.h"

const char *
hsvc_rundir(void)
{
	const char *r = getenv("HSVC_RUNDIR");
	return (r && *r) ? r : HSVC_RUNDIR_DEFAULT;
}

int
hsvc_send_all(int fd, const void *buf, size_t n)
{
	const uint8_t *p = buf;
	while (n > 0) {
		ssize_t w = send(fd, p, n, MSG_NOSIGNAL);
		if (w < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		p += w;
		n -= (size_t)w;
	}
	return 0;
}

int
hsvc_send_frame(int fd, uint16_t type, uint64_t seq, const void *payload,
                uint32_t len)
{
	uint8_t hdr[HSVC_HDR_LEN];
	hsvc_hdr_encode(hdr, type, seq, len);
	if (hsvc_send_all(fd, hdr, sizeof(hdr)) < 0)
		return -1;
	if (len > 0 && hsvc_send_all(fd, payload, len) < 0)
		return -1;
	return 0;
}

static int
recv_all(int fd, uint8_t *buf, size_t n)
{
	while (n > 0) {
		ssize_t r = recv(fd, buf, n, 0);
		if (r <= 0) {
			if (r < 0 && errno == EINTR)
				continue;
			return -1;
		}
		buf += r;
		n -= (size_t)r;
	}
	return 0;
}

int
hsvc_recv_frame(int fd, struct hsvc_hdr *hdr, uint8_t **payload)
{
	uint8_t raw[HSVC_HDR_LEN];
	*payload = NULL;
	if (recv_all(fd, raw, sizeof(raw)) < 0)
		return -1;
	if (hsvc_hdr_decode(raw, hdr) < 0)
		return -1;
	if (hdr->len > 0) {
		*payload = malloc(hdr->len);
		if (!*payload)
			return -1;
		if (recv_all(fd, *payload, hdr->len) < 0) {
			free(*payload);
			*payload = NULL;
			return -1;
		}
	}
	return 0;
}

static int
sockaddr(const char *path, struct sockaddr_un *sa)
{
	if (strlen(path) >= sizeof(sa->sun_path))
		return -1;
	memset(sa, 0, sizeof(*sa));
	sa->sun_family = AF_UNIX;
	strcpy(sa->sun_path, path);
	return 0;
}

int
hsvc_listen_at(const char *path)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	struct sockaddr_un sa;
	if (sockaddr(path, &sa) < 0)
		goto fail;
	unlink(path);
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		goto fail;
	if (listen(fd, 8) < 0)
		goto fail;
	return fd;
fail:
	close(fd);
	return -1;
}

int
hsvc_dial(const char *path)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	struct sockaddr_un sa;
	if (sockaddr(path, &sa) < 0 ||
	    connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static void
daemon_path(char *out, size_t n)
{
	snprintf(out, n, "%s/%s", hsvc_rundir(), HSVC_DAEMON_SOCK);
}

int
hsvc_daemon_call(uint32_t op, const void *args, uint32_t args_len,
                 uint8_t **reply, uint32_t *reply_len)
{
	char path[256];
	daemon_path(path, sizeof(path));
	int fd = hsvc_dial(path);
	if (fd < 0)
		return -1;
	uint8_t *req = malloc(8 + args_len);
	if (!req) {
		close(fd);
		return -1;
	}
	hsvc_put32(req, 1); /* call id */
	hsvc_put32(req + 4, op);
	if (args_len)
		memcpy(req + 8, args, args_len);
	int rc = -1;
	if (hsvc_send_frame(fd, HSVC_MSG_CALL, 1, req, 8 + args_len) == 0) {
		struct hsvc_hdr hdr;
		uint8_t *pl = NULL;
		if (hsvc_recv_frame(fd, &hdr, &pl) == 0 &&
		    hdr.type == HSVC_MSG_REPLY && hdr.len >= 8) {
			rc = (int32_t)hsvc_get32(pl + 4);
			if (reply) {
				*reply_len = hdr.len - 8;
				*reply = malloc(*reply_len ? *reply_len : 1);
				if (*reply_len)
					memcpy(*reply, pl + 8, *reply_len);
			}
			free(pl);
		}
	}
	free(req);
	close(fd);
	return rc;
}

char *
hsvc_resolve(enum hsvc_kind kind)
{
	char arg[16];
	snprintf(arg, sizeof(arg), "%u", kind);
	uint8_t *reply = NULL;
	uint32_t reply_len = 0;
	if (hsvc_daemon_call(HSVC_OP_RESOLVE, arg, strlen(arg) + 1, &reply,
	                     &reply_len) != 0 ||
	    reply_len == 0) {
		free(reply);
		return NULL;
	}
	return (char *)reply;
}

int
hsvc_dial_kind(enum hsvc_kind kind)
{
	char *path = hsvc_resolve(kind);
	if (!path)
		return -1;
	int fd = hsvc_dial(path);
	free(path);
	return fd;
}

int
hsvc_register(const char *name, enum hsvc_kind kind, const char *path)
{
	char args[1024];
	snprintf(args, sizeof(args), "%s\n%u\n%s\n1", name, kind, path);
	return hsvc_daemon_call(HSVC_OP_REGISTER, args, strlen(args) + 1, NULL,
	                        NULL);
}

/* Register on a connection that stays open: the daemon owns the entry
 * for the connection's life, so a dead driver's services drop out of
 * the registry on their own. */
static int
register_hold(const char *name, enum hsvc_kind kind, const char *path)
{
	char sock[256];
	snprintf(sock, sizeof(sock), "%s/%s", hsvc_rundir(),
	         HSVC_DAEMON_SOCK);
	int fd = hsvc_dial(sock);
	if (fd < 0)
		return -1;
	char args[1024];
	snprintf(args, sizeof(args), "%s\n%u\n%s\n0", name, kind, path);
	uint8_t req[8 + sizeof(args)];
	hsvc_put32(req, 1);
	hsvc_put32(req + 4, HSVC_OP_REGISTER);
	memcpy(req + 8, args, strlen(args) + 1);
	int rc = -1;
	if (hsvc_send_frame(fd, HSVC_MSG_CALL, 1, req,
	                    8 + strlen(args) + 1) == 0) {
		struct hsvc_hdr hdr;
		uint8_t *pl = NULL;
		if (hsvc_recv_frame(fd, &hdr, &pl) == 0 &&
		    hdr.type == HSVC_MSG_REPLY && hdr.len >= 8)
			rc = (int32_t)hsvc_get32(pl + 4);
		free(pl);
	}
	if (rc != 0) {
		close(fd);
		return -1;
	}
	return fd;
}

int
hsvc_unregister(const char *name)
{
	return hsvc_daemon_call(HSVC_OP_UNREGISTER, name, strlen(name) + 1, NULL,
	                        NULL);
}

/* ------------------------- service ------------------------- */

struct hsvc_client
{
	int fd;
	uint32_t subs[16];
	int nsub;
};

struct hsvc_service
{
	int listen_fd;
	char *sock_path;
	char *desc;
	char *name;
	enum hsvc_kind kind;
	uint64_t seq;
	struct hsvc_client clients[32];
	int nclients;
	int reg_fd;
	hsvc_call_cb call_cb;
	void *call_ud;
};

static void
svc_dir(char *out, size_t n)
{
	snprintf(out, n, "%s/%s", hsvc_rundir(), HSVC_SVCDIR);
}

static void
mkpath(const char *dir)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s", dir);
	for (char *p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
	mkdir(tmp, 0755);
}

struct hsvc_service *
hsvc_publish(const char *name, enum hsvc_kind kind, const char *desc_kv)
{
	char dir[256], path[512];
	svc_dir(dir, sizeof(dir));
	mkpath(dir);
	snprintf(path, sizeof(path), "%s/%s.sock", dir, name);
	int fd = hsvc_listen_at(path);
	if (fd < 0)
		return NULL;
	struct hsvc_service *s = calloc(1, sizeof(*s));
	s->listen_fd = fd;
	s->sock_path = strdup(path);
	s->desc = strdup(desc_kv ? desc_kv : "");
	s->name = strdup(name);
	s->kind = kind;
	s->reg_fd = register_hold(name, kind, path);
	return s;
}

static void
client_drop(struct hsvc_service *s, int i)
{
	close(s->clients[i].fd);
	s->clients[i] = s->clients[--s->nclients];
}

static int
client_sub(struct hsvc_client *c, uint32_t stream, int on)
{
	for (int i = 0; i < c->nsub; i++) {
		if (c->subs[i] == stream) {
			if (!on) {
				c->subs[i] = c->subs[--c->nsub];
				return 0;
			}
			return 0;
		}
	}
	if (!on)
		return 0;
	if (c->nsub >= 16)
		return -1;
	c->subs[c->nsub++] = stream;
	return 0;
}

static int
client_frame(struct hsvc_service *s, int i)
{
	struct hsvc_client *c = &s->clients[i];
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	if (hsvc_recv_frame(c->fd, &hdr, &pl) < 0) {
		client_drop(s, i);
		return -1;
	}
	switch (hdr.type) {
	case HSVC_MSG_PING:
		hsvc_send_frame(c->fd, HSVC_MSG_PONG, hdr.seq, pl, hdr.len);
		break;
	case HSVC_MSG_SUB:
	case HSVC_MSG_UNSUB:
		if (hdr.len >= 4)
			client_sub(c, hsvc_get32(pl), hdr.type == HSVC_MSG_SUB);
		break;
	case HSVC_MSG_CALL:
		if (hdr.len >= 8 && s->call_cb) {
			uint8_t *repl = NULL;
			uint32_t repl_len = 0;
			int st = s->call_cb(s->call_ud, hsvc_get32(pl + 4),
			                    pl + 8, hdr.len - 8, &repl, &repl_len);
			uint8_t *out = malloc(8 + repl_len);
			hsvc_put32(out, hsvc_get32(pl));
			hsvc_put32(out + 4, (uint32_t)st);
			if (repl_len)
				memcpy(out + 8, repl, repl_len);
			hsvc_send_frame(c->fd, HSVC_MSG_REPLY, hdr.seq, out,
			                8 + repl_len);
			free(out);
			free(repl);
		}
		break;
	default:
		break;
	}
	free(pl);
	return 0;
}

int
hsvc_stream(struct hsvc_service *s, uint32_t stream, uint64_t ts_ns,
            const void *data, uint32_t len)
{
	uint8_t *pl = malloc(12 + len);
	hsvc_put32(pl, stream);
	hsvc_put64(pl + 4, ts_ns);
	if (len)
		memcpy(pl + 12, data, len);
	for (int i = 0; i < s->nclients; i++) {
		struct hsvc_client *c = &s->clients[i];
		int want = 0;
		for (int j = 0; j < c->nsub; j++)
			want |= c->subs[j] == stream;
		if (want &&
		    hsvc_send_frame(c->fd, HSVC_MSG_STREAM, ++s->seq, pl,
		                    12 + len) < 0) {
			client_drop(s, i);
			i--;
		}
	}
	free(pl);
	return 0;
}

int
hsvc_subscribers(struct hsvc_service *s, uint32_t stream)
{
	int n = 0;
	for (int i = 0; i < s->nclients; i++)
		for (int j = 0; j < s->clients[i].nsub; j++)
			n += s->clients[i].subs[j] == stream;
	return n;
}

void
hsvc_set_call(struct hsvc_service *s, hsvc_call_cb cb, void *ud)
{
	s->call_cb = cb;
	s->call_ud = ud;
}

int
hsvc_serve(struct hsvc_service *s, int tick_ms, hsvc_emit_cb emit,
           void *userdata)
{
	struct pollfd pfds[1 + 32];
	for (;;) {
		pfds[0].fd = s->listen_fd;
		pfds[0].events = POLLIN;
		for (int i = 0; i < s->nclients; i++) {
			pfds[1 + i].fd = s->clients[i].fd;
			pfds[1 + i].events = POLLIN;
		}
		int r = poll(pfds, 1 + s->nclients, tick_ms);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (pfds[0].revents & POLLIN) {
			int c = accept(s->listen_fd, NULL, NULL);
			if (c >= 0 && s->nclients < 32) {
				s->clients[s->nclients].fd = c;
				s->clients[s->nclients].nsub = 0;
				s->nclients++;
				hsvc_send_frame(c, HSVC_MSG_INFO, ++s->seq,
				                s->desc, strlen(s->desc));
			} else if (c >= 0) {
				close(c);
			}
		}
		for (int i = 0; i < s->nclients; i++) {
			if (pfds[1 + i].revents & (POLLIN | POLLHUP | POLLERR) &&
			    client_frame(s, i) < 0)
				i--;
		}
		if (r == 0 && emit)
			emit(s, userdata);
	}
}

void
hsvc_destroy(struct hsvc_service *s)
{
	if (!s)
		return;
	if (s->reg_fd >= 0)
		close(s->reg_fd);
	hsvc_unregister(s->name);
	for (int i = 0; i < s->nclients; i++)
		close(s->clients[i].fd);
	close(s->listen_fd);
	if (s->sock_path)
		unlink(s->sock_path);
	free(s->sock_path);
	free(s->desc);
	free(s->name);
	free(s);
}
