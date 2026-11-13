// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
//
// hsvcx_vmd_dial test: a stub vmd service endpoint on loopback answers
// HELLO with INFO and streams hsvc_pose frames, the client side dials,
// subs and reads a sample - the same path drv_hsvc takes into a vmd
// host.

#include "hsvc_bus.h"
#include "vmd_proto.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void *
srv_run(void *arg)
{
	int l = *(int *)arg;
	int c = accept(l, NULL, NULL);
	assert(c >= 0);
	struct hsvc_hdr hdr;
	uint8_t *pl = NULL;
	assert(hsvc_recv_frame(c, &hdr, &pl) == 0);
	assert(hdr.type == HSVC_MSG_HELLO);
	assert(hdr.len == strlen(VMD_SVC_HMD) &&
	       memcmp(pl, VMD_SVC_HMD, hdr.len) == 0);
	free(pl);
	const char *desc = "kind=1\ndrv=vmd\nw=2160\nh=2160\n";
	assert(hsvc_send_frame(c, HSVC_MSG_INFO, hdr.seq, desc,
	                       (uint32_t)strlen(desc)) == 0);
	for (;;) {
		if (hsvc_recv_frame(c, &hdr, &pl) < 0)
			break;
		if (hdr.type == HSVC_MSG_SUB) {
			struct hsvc_pose p = {.py = 1.7f, .qw = 1.f, .state = 3};
			uint8_t pay[12 + sizeof(p)];
			hsvc_put32(pay, 0);
			hsvc_put64(pay + 4, 1234);
			memcpy(pay + 12, &p, sizeof(p));
			hsvc_send_frame(c, HSVC_MSG_STREAM, 1, pay,
			                sizeof(pay));
		}
		free(pl);
		pl = NULL;
	}
	close(c);
	return NULL;
}

int
main(void)
{
	int l = socket(AF_INET, SOCK_STREAM, 0);
	assert(l >= 0);
	struct sockaddr_in a = {0};
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	assert(bind(l, (struct sockaddr *)&a, sizeof(a)) == 0);
	assert(listen(l, 1) == 0);
	socklen_t n = sizeof(a);
	assert(getsockname(l, (struct sockaddr *)&a, &n) == 0);

	pthread_t t;
	assert(pthread_create(&t, NULL, srv_run, &l) == 0);

	char addr[64];
	snprintf(addr, sizeof(addr), "127.0.0.1:%d", ntohs(a.sin_port));
	setenv("VMD_ADDR", addr, 1);

	struct hsvcx_conn conn;
	assert(hsvcx_vmd_dial(VMD_SVC_HMD, &conn) == 0);
	assert(strstr(conn.desc, "kind=1") != NULL);
	assert(hsvcx_sub(&conn, 0) == 0);

	uint32_t stream = 99;
	uint64_t ts = 0;
	struct hsvc_pose p = {0};
	assert(hsvcx_next(&conn, &stream, &ts, &p, sizeof(p)) ==
	       (int)sizeof(p));
	assert(stream == 0 && ts == 1234);
	assert(p.py == 1.7f && p.qw == 1.f && p.state == 3);
	hsvcx_close(&conn);

	/* dead endpoint fails fast, not after a long wait */
	setenv("VMD_ADDR", "127.0.0.1:1", 1);
	assert(hsvcx_vmd_dial(VMD_SVC_HMD, &conn) < 0);

	pthread_join(t, NULL);
	close(l);
	printf("tcp dial test passed\n");
	return 0;
}
