// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvcctl - inspect and poke the hsvc bus.
 *
 *   hsvcctl list             services from hsvcd
 *   hsvcctl resolve <kind>   sock path for a kind
 *   hsvcctl info <service>   service descriptor (connect by svc name)
 *   hsvcctl cat <service>    subscribe stream 0 and dump samples
 *   hsvcctl watch            daemon event stream
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hsvc/hsvc.h"

static int
cmd_list(void)
{
	uint8_t *reply = NULL;
	uint32_t n = 0;
	if (hsvc_daemon_call(HSVC_OP_LIST, NULL, 0, &reply, &n) != 0)
		return 1;
	fwrite(reply, 1, n, stdout);
	free(reply);
	return 0;
}

static int
cmd_resolve(const char *kind)
{
	char *path = hsvc_resolve((enum hsvc_kind)strtoul(kind, NULL, 10));
	if (!path)
		return 1;
	puts(path);
	free(path);
	return 0;
}

static int
svc_connect(const char *name)
{
	char path[512];
	snprintf(path, sizeof(path), "%s/%s/%s.sock", hsvc_rundir(),
	         HSVC_SVCDIR, name);
	return hsvc_dial(path);
}

static int
cmd_info(const char *name)
{
	int fd = svc_connect(name);
	if (fd < 0)
		return 1;
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	if (hsvc_recv_frame(fd, &hdr, &pl) < 0) {
		close(fd);
		return 1;
	}
	if (pl)
		fwrite(pl, 1, hdr.len, stdout);
	free(pl);
	close(fd);
	return 0;
}

static int
cmd_cat(const char *name, uint32_t stream)
{
	int fd = svc_connect(name);
	if (fd < 0)
		return 1;
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	if (hsvc_recv_frame(fd, &hdr, &pl) < 0) /* INFO */
		return 1;
	free(pl);
	uint8_t sub[4];
	hsvc_put32(sub, stream);
	if (hsvc_send_frame(fd, HSVC_MSG_SUB, 1, sub, 4) < 0)
		return 1;
	for (;;) {
		if (hsvc_recv_frame(fd, &hdr, &pl) < 0)
			return 1;
		if (hdr.type == HSVC_MSG_STREAM && hdr.len >= 12) {
			uint32_t s = hsvc_get32(pl);
			uint64_t ts = hsvc_get64(pl + 4);
			printf("stream=%u ts=%llu len=%u data=",
			       s, (unsigned long long)ts, hdr.len - 12);
			for (uint32_t i = 12; i < hdr.len && i < 12 + 16; i++)
				printf("%02x", pl[i]);
			puts(hdr.len > 28 ? "..." : "");
			fflush(stdout);
		}
		free(pl);
	}
}

static int
cmd_watch(void)
{
	uint8_t op[8];
	hsvc_put32(op, 1);
	hsvc_put32(op + 4, HSVC_OP_WATCH);
	char path[256];
	snprintf(path, sizeof(path), "%s/%s", hsvc_rundir(),
	         HSVC_DAEMON_SOCK);
	int fd = hsvc_dial(path);
	if (fd < 0)
		return 1;
	if (hsvc_send_frame(fd, HSVC_MSG_CALL, 1, op, 8) < 0)
		return 1;
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	if (hsvc_recv_frame(fd, &hdr, &pl) < 0) /* watch REPLY ack */
		return 1;
	free(pl);
	for (;;) {
		if (hsvc_recv_frame(fd, &hdr, &pl) < 0)
			return 1;
		if (hdr.type == HSVC_MSG_EVENT && pl)
			fwrite(pl, 1, hdr.len, stdout);
		fflush(stdout);
		free(pl);
	}
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr,
		        "usage: hsvcctl list|resolve K|info S|cat S [stream]|watch\n");
		return 2;
	}
	if (!strcmp(argv[1], "list"))
		return cmd_list();
	if (!strcmp(argv[1], "resolve") && argc > 2)
		return cmd_resolve(argv[2]);
	if (!strcmp(argv[1], "info") && argc > 2)
		return cmd_info(argv[2]);
	if (!strcmp(argv[1], "cat") && argc > 2)
		return cmd_cat(argv[2], argc > 3 ? (uint32_t)atoi(argv[3]) : 0);
	if (!strcmp(argv[1], "watch"))
		return cmd_watch();
	fprintf(stderr, "hsvcctl: unknown command\n");
	return 2;
}
