// vmd_chan reader regression tests. A wedged-but-connected host used to
// park vmd_chan_read inside recv forever, so the driver's stop flag
// could never wake it and vmd_destroy hung joining the thread. Runs
// natively - vmd_chan.c is plain POSIX, unlike the freestanding tests.
#include "vmd_chan.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>


struct reader
{
	struct vmd_chan *chan;
	struct vmd_pose_packet pkt;
	volatile bool stop;
	bool got;
};

static void *
reader_main(void *ptr)
{
	struct reader *r = ptr;
	r->got = vmd_chan_read(r->chan, &r->pkt, &r->stop);
	return NULL;
}

static int
listen_any(int *port_out)
{
	int l = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in a = {0};
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (l < 0 || bind(l, (struct sockaddr *)&a, sizeof(a)) != 0 ||
	    listen(l, 4) != 0) {
		return -1;
	}
	socklen_t n = sizeof(a);
	if (getsockname(l, (struct sockaddr *)&a, &n) != 0) {
		close(l);
		return -1;
	}
	*port_out = ntohs(a.sin_port);
	return l;
}

static struct reader *
reader_start(int port, pthread_t *out)
{
	char addr[64];
	snprintf(addr, sizeof(addr), "127.0.0.1:%d", port);
	struct reader *r = calloc(1, sizeof(*r));
	r->chan = vmd_chan_open(addr);
	if (r->chan == NULL || pthread_create(out, NULL, reader_main, r) != 0) {
		return NULL;
	}
	return r;
}

// bounded join: the whole point is the reader must come back
static int
reader_join(pthread_t t, struct reader *r, int secs)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += secs;
	int err = pthread_timedjoin_np(t, NULL, &ts);
	vmd_chan_close(r->chan);
	free(r);
	return err == 0 ? 0 : -1;
}

static void
nap_ms(long ms)
{
	struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
	nanosleep(&ts, NULL);
}

static int
test_stop_wakes_idle_read(void)
{
	int port, l = listen_any(&port);
	if (l < 0) {
		printf("FAIL listen\n");
		return 1;
	}
	pthread_t t;
	struct reader *r = reader_start(port, &t);
	if (r == NULL) {
		printf("FAIL reader start\n");
		return 1;
	}
	int conn = accept(l, NULL, NULL);
	if (conn < 0) {
		printf("FAIL accept\n");
		return 1;
	}
	// accepted but never sends: the wedged host from the bug report
	nap_ms(600);
	r->stop = true;
	if (reader_join(t, r, 5) != 0 || r->got) {
		printf("FAIL idle read did not wake on stop\n");
		return 1;
	}
	close(conn);
	close(l);
	printf("PASS stop wakes idle read\n");
	return 0;
}

static int
test_packet_delivered(void)
{
	int port, l = listen_any(&port);
	if (l < 0) {
		printf("FAIL listen\n");
		return 1;
	}
	pthread_t t;
	struct reader *r = reader_start(port, &t);
	if (r == NULL) {
		printf("FAIL reader start\n");
		return 1;
	}
	int conn = accept(l, NULL, NULL);
	if (conn < 0) {
		printf("FAIL accept\n");
		return 1;
	}
	struct vmd_pose_packet pkt = {0};
	pkt.magic = VMD_PROTO_MAGIC;
	pkt.version = VMD_PROTO_VERSION;
	pkt.seq = 42;
	pkt.quat[3] = 1.0f;
	pkt.pos[1] = 1.5f;
	if (send(conn, &pkt, sizeof(pkt), 0) != (ssize_t)sizeof(pkt)) {
		printf("FAIL send\n");
		return 1;
	}
	if (reader_join(t, r, 5) != 0 || !r->got ||
	    r->pkt.seq != 42 || r->pkt.quat[3] != 1.0f ||
	    r->pkt.pos[1] != 1.5f) {
		printf("FAIL packet not delivered\n");
		return 1;
	}
	close(conn);
	close(l);
	printf("PASS packet delivered\n");
	return 0;
}

static int
test_reconnect_after_drop(void)
{
	int port, l = listen_any(&port);
	if (l < 0) {
		printf("FAIL listen\n");
		return 1;
	}
	pthread_t t;
	struct reader *r = reader_start(port, &t);
	if (r == NULL) {
		printf("FAIL reader start\n");
		return 1;
	}
	int conn = accept(l, NULL, NULL);
	if (conn < 0) {
		printf("FAIL accept first\n");
		return 1;
	}
	struct vmd_pose_packet half = {0};
	send(conn, &half, sizeof(half) / 2, 0);
	close(conn);
	conn = accept(l, NULL, NULL);
	if (conn < 0) {
		printf("FAIL accept second\n");
		return 1;
	}
	struct vmd_pose_packet pkt = {0};
	pkt.magic = VMD_PROTO_MAGIC;
	pkt.version = VMD_PROTO_VERSION;
	pkt.seq = 7;
	pkt.quat[3] = 1.0f;
	if (send(conn, &pkt, sizeof(pkt), 0) != (ssize_t)sizeof(pkt)) {
		printf("FAIL send\n");
		return 1;
	}
	if (reader_join(t, r, 5) != 0 || !r->got || r->pkt.seq != 7) {
		printf("FAIL no reconnect after drop\n");
		return 1;
	}
	close(conn);
	close(l);
	printf("PASS reconnect after drop\n");
	return 0;
}

static int
test_stop_without_host(void)
{
	int port, l = listen_any(&port);
	if (l < 0) {
		printf("FAIL listen\n");
		return 1;
	}
	close(l);
	pthread_t t;
	struct reader *r = reader_start(port, &t);
	if (r == NULL) {
		printf("FAIL reader start\n");
		return 1;
	}
	nap_ms(600);
	r->stop = true;
	if (reader_join(t, r, 5) != 0 || r->got) {
		printf("FAIL refused-connect loop ignored stop\n");
		return 1;
	}
	printf("PASS stop with no host\n");
	return 0;
}

int
main(void)
{
	int fails = 0;
	fails += test_stop_wakes_idle_read();
	fails += test_packet_delivered();
	fails += test_reconnect_after_drop();
	fails += test_stop_without_host();
	return fails == 0 ? 0 : 1;
}
