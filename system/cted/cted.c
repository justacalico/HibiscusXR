// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  cted - CTE daemon: the on-headset half of HCTE.
 *
 * Listens on TCP (default port 7340) and serves the line-based protocol
 * applications/desktop/cte speaks. One command per connection; the client gets a
 * `CTE/1` banner, writes a command, reads the reply:
 *
 *   PING           -> +PONG
 *   INFO           -> +JSON <len>\n<json>     device summary + key props
 *   PROPS          -> +TEXT <len>\n<getprop dump>
 *   FRAMES         -> FRAME <len>\n<png>      repeats while connected
 *   POSE           -> POSELOG <line>\n        logcat pose tags, forwarded
 *   CTRL           -> CTRL <idx> k=v ...\n    decoded CtrlShareMem stream
 *   LOG            -> LOG <line>\n            full logcat
 *   INSTALL <len>  -> reads <len> apk bytes, pm install -r, then +OK/+ERR
 *
 * Compiles for Android (NDK, aarch64) and for a desktop host, where the
 * Android-facing commands just report empty/failed. CTE_CTRL_PATH and
 * CTE_PORT override the sharemem path and listen port for host tests.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ctrl_state.h"

#define CTE_PORT_DEFAULT 7340
#define CTE_BANNER "CTE/1\n"

static uint64_t
mono_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int
send_all(int fd, const void *buf, size_t n)
{
	const uint8_t *p = buf;
	while (n > 0) {
		ssize_t w = send(fd, p, n, MSG_NOSIGNAL);
		if (w <= 0) {
			return -1;
		}
		p += w;
		n -= (size_t)w;
	}
	return 0;
}

static int
send_str(int fd, const char *s)
{
	return send_all(fd, s, strlen(s));
}

static int
sendf(int fd, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (n < 0) {
		return -1;
	}
	if (n >= (int)sizeof(buf)) {
		n = (int)sizeof(buf) - 1;
	}
	return send_all(fd, buf, (size_t)n);
}

// nonzero while the peer is still there - MSG_PEEK sees the close without
// eating data the protocol layer may still want
static int
sock_alive(int fd)
{
	uint8_t b;
	ssize_t r = recv(fd, &b, 1, MSG_PEEK | MSG_DONTWAIT);
	if (r == 0) {
		return 0;
	}
	if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return 1;
	}
	return r > 0;
}

static char *
read_stream(FILE *f, size_t *out_len)
{
	size_t cap = 1 << 16, len = 0;
	char *buf = malloc(cap);
	if (buf == NULL) {
		return NULL;
	}
	for (;;) {
		if (len + 8192 + 1 > cap) {
			cap *= 2;
			char *nb = realloc(buf, cap);
			if (nb == NULL) {
				free(buf);
				return NULL;
			}
			buf = nb;
		}
		size_t got = fread(buf + len, 1, 8192, f);
		len += got;
		if (got < 8192) {
			break;
		}
	}
	buf[len] = 0;
	if (out_len) {
		*out_len = len;
	}
	return buf;
}

static char *
read_cmd(const char *cmd)
{
	FILE *f = popen(cmd, "r");
	if (f == NULL) {
		return NULL;
	}
	char *out = read_stream(f, NULL);
	pclose(f);
	return out;
}

static char *
prop(const char *key, char *buf, size_t cap)
{
	char cmd[160];
	snprintf(cmd, sizeof(cmd), "getprop %s 2>/dev/null", key);
	char *v = read_cmd(cmd);
	if (v == NULL) {
		buf[0] = 0;
		return buf;
	}
	size_t n = strcspn(v, "\r\n");
	v[n] = 0;
	snprintf(buf, cap, "%s", v);
	free(v);
	return buf;
}

// append a JSON-escaped string (quotes and backslashes escaped)
static void
json_esc(char *dst, size_t cap, size_t *off, const char *s)
{
	for (; *s && *off + 3 < cap; s++) {
		if (*s == '"' || *s == '\\') {
			dst[(*off)++] = '\\';
		}
		dst[(*off)++] = *s;
	}
	dst[*off] = 0;
}

static void
json_prop(char *dst, size_t cap, size_t *off, const char *key)
{
	char v[256];
	prop(key, v, sizeof(v));
	if (v[0] == 0) {
		return;
	}
	size_t n = snprintf(dst + *off, cap - *off, "\"%s\":\"", key);
	*off += (size_t)n;
	json_esc(dst, cap, off, v);
	n = snprintf(dst + *off, cap - *off, "\",");
	*off += (size_t)n;
}

static const char *const INFO_PROPS[] = {
    "ro.product.model",       "ro.product.device",      "ro.product.brand",
    "ro.product.manufacturer", "ro.build.version.release", "ro.build.version.sdk",
    "ro.build.display.id",    "ro.hibiscus.version",    "persist.pn2.dof",
    "persist.hibiscus.dof",   "ro.serialno",
};

static void
cmd_info(int fd)
{
	char *js = malloc(1 << 16);
	if (js == NULL) {
		send_str(fd, "+ERR oom\n");
		return;
	}
	size_t off = 0;
	off += snprintf(js + off, (1 << 16) - off, "{");

	char model[128], device[128], dof[64];
	prop("ro.product.model", model, sizeof(model));
	prop("ro.product.device", device, sizeof(device));
	char dof_pn[64], dof_hib[64];
	prop("persist.pn2.dof", dof_pn, sizeof(dof_pn));
	prop("persist.hibiscus.dof", dof_hib, sizeof(dof_hib));
	snprintf(dof, sizeof(dof), "%s", dof_pn[0] ? dof_pn : dof_hib);

	int battery = -1;
	char *b = read_cmd("dumpsys battery 2>/dev/null");
	if (b) {
		char *p = strstr(b, "level:");
		if (p) {
			battery = atoi(p + 6);
		}
		free(b);
	}

	off += snprintf(js + off, (1 << 16) - off, "\"model\":\"");
	json_esc(js, 1 << 16, &off, model);
	off += snprintf(js + off, (1 << 16) - off, "\",\"device\":\"");
	json_esc(js, 1 << 16, &off, device);
	off += snprintf(js + off, (1 << 16) - off,
	                "\",\"tracking\":\"%s\",\"battery\":%d,\"props\":{",
	                strstr(dof, "6") ? "dof6" : strstr(dof, "3") ? "dof3" : "unknown",
	                battery);
	for (size_t i = 0; i < sizeof(INFO_PROPS) / sizeof(INFO_PROPS[0]); i++) {
		json_prop(js, 1 << 16, &off, INFO_PROPS[i]);
	}
	// drop the trailing comma
	if (off > 0 && js[off - 1] == ',') {
		off--;
	}
	off += snprintf(js + off, (1 << 16) - off, "}}");
	(void)off;

	sendf(fd, "+JSON %zu\n", strlen(js));
	send_all(fd, js, strlen(js));
	send_str(fd, "\n");
	free(js);
}

static void
cmd_props(int fd)
{
	char *out = read_cmd("getprop");
	size_t n = out ? strlen(out) : 0;
	sendf(fd, "+TEXT %zu\n", n);
	if (n) {
		send_all(fd, out, n);
	}
	send_str(fd, "\n");
	free(out);
}

// forward every line a child prints, wrapped in `prefix line\n`; stops when
// the peer goes away or the child exits. poll() so a quiet child can't pin
// the thread after the client disconnects.
static int
stream_cmd_lines(int fd, const char *cmd, const char *prefix)
{
	FILE *f = popen(cmd, "r");
	if (f == NULL) {
		return sendf(fd, "+ERR %s failed\n", prefix);
	}
	int pfd = fileno(f);
	fcntl(pfd, F_SETFL, fcntl(pfd, F_GETFL) | O_NONBLOCK);

	char line[2048];
	size_t used = 0;
	for (;;) {
		struct pollfd p = {.fd = pfd, .events = POLLIN};
		int pr = poll(&p, 1, 500);
		if (pr < 0 || (p.revents & (POLLERR | POLLHUP))) {
			break;
		}
		if (!sock_alive(fd)) {
			break;
		}
		if (!(p.revents & POLLIN)) {
			continue;
		}
		ssize_t r = read(pfd, line + used, sizeof(line) - 1 - used);
		if (r <= 0) {
			break;
		}
		used += (size_t)r;
		line[used] = 0;
		// emit complete lines, keep the tail
		char *start = line;
		char *nl;
		while ((nl = memchr(start, '\n', used - (size_t)(start - line)))) {
			*nl = 0;
			sendf(fd, "%s %s\n", prefix, start);
			start = nl + 1;
		}
		used -= (size_t)(start - line);
		if (used) {
			memmove(line, start, used);
		}
	}
	pclose(f);
	return 0;
}

static void
cmd_frames(int fd)
{
	while (sock_alive(fd)) {
		FILE *f = popen("screencap -p", "r");
		if (f == NULL) {
			break;
		}
		size_t n = 0;
		char *png = read_stream(f, &n);
		pclose(f);
		if (png == NULL || n == 0) {
			free(png);
			send_str(fd, "+ERR screencap failed\n");
			break;
		}
		sendf(fd, "FRAME %zu\n", n);
		if (send_all(fd, png, (size_t)n) != 0) {
			free(png);
			break;
		}
		free(png);
		usleep(400 * 1000);
	}
}

static void
cmd_pose(int fd)
{
	// enable the driver dump paths: the pn2 prop/file switch plus the
	// generic feed tag any driver can emit
	system("setprop debug.pn2.posedump 1 2>/dev/null");
	system("mkdir -p /data/local/tmp/xr && touch /data/local/tmp/xr/posedump 2>/dev/null");
	stream_cmd_lines(fd, "logcat pn2pose:I hibiscuspose:I *:S 2>/dev/null",
	                 "POSELOG");
}

static void
cmd_log(int fd)
{
	stream_cmd_lines(fd, "logcat -v brief 2>/dev/null", "LOG");
}

static void
cmd_ctrl(int fd)
{
	struct ctrl_share share;
	if (ctrl_share_open(&share, getenv("CTE_CTRL_PATH")) != 0) {
		// no controller channel on this device; idle so the client sees
		// "not connected" rather than a dropped socket
		while (sock_alive(fd)) {
			usleep(200 * 1000);
		}
		return;
	}

	struct ctrl_live live[2] = {{0}, {0}};
	struct ctrl_probe probe = {0};
	uint64_t probe_ns = 0;
	uint64_t last_emit[2] = {0, 0};
	uint64_t last_hash[2] = {0, 0};
	uint8_t buf[CTRL_SHARE_SIZE];

	while (sock_alive(fd)) {
		if (ctrl_share_snapshot(&share, buf) != 0) {
			break;
		}
		uint64_t now = mono_ns();
		// same wire-edge probe cadence as the pn2 driver
		int wire = ctrl_probe_poll(&probe);
		if (wire == 0 && now >= probe_ns && probe.state == 0) {
			probe_ns = now + CTRL_PROBE_GAP_NS;
			ctrl_probe_start(&probe, share.path, -1, CTRL_PROBE_SPIN_NS);
		}
		for (int i = 0; i < 2; i++) {
			uint64_t h = ctrl_state_hash(buf, i);
			int is_live = ctrl_live_feed(&live[i], h, wire > 0, now) &&
			              ctrl_block_has_data(buf, i);
			// emit on change, plus a ~1s heartbeat so link loss shows up
			if (h == last_hash[i] && now - last_emit[i] < 1000000000ull) {
				continue;
			}
			struct ctrl_state st;
			ctrl_state_decode(buf, i, &st);
			const struct ctrl_pose *p = &st.fuse;
			const struct ctrl_keys *k = &st.keys;
			sendf(fd,
			      "CTRL %d live=%d batt=%d "
			      "px=%.4f py=%.4f pz=%.4f "
			      "qx=%.4f qy=%.4f qz=%.4f qw=%.4f "
			      "trk=%d sx=%d sy=%d a=%d b=%d menu=%d sys=%d "
			      "trig=%d grip=%d\n",
			      i, is_live,
			      (is_live && st.keys_ok) ? k->battery : -1,
			      p->x * 0.001f, p->y * 0.001f, p->z * 0.001f,
			      p->qx, p->qy, p->qz, p->q0,
			      p->status != 0 && is_live,
			      k->touch_x, k->touch_y, !!k->a, !!k->b, !!k->app,
			      !!k->home, !!k->trigger, !!(k->grip_l || k->grip_r));
			last_emit[i] = now;
			last_hash[i] = h;
		}
		usleep(33 * 1000);
	}
	ctrl_share_close(&share);
}

static void
cmd_install(int fd, size_t len)
{
	if (len == 0 || len > (512u << 20)) {
		sendf(fd, "+ERR bad length %zu\n", len);
		return;
	}
	const char *path = "/data/local/tmp/cte-install.apk";
	int out = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (out < 0) {
		sendf(fd, "+ERR open %s: %s\n", path, strerror(errno));
		return;
	}
	uint8_t buf[65536];
	size_t got = 0;
	int fail = 0;
	while (got < len) {
		ssize_t r = recv(fd, buf, sizeof(buf) < len - got ? sizeof(buf) : len - got, 0);
		if (r <= 0) {
			fail = 1;
			break;
		}
		got += (size_t)r;
		if (write(out, buf, (size_t)r) != r) {
			fail = 1;
			break;
		}
	}
	close(out);
	if (fail) {
		unlink(path);
		send_str(fd, "+ERR short apk\n");
		return;
	}
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "pm install -r %s 2>&1", path);
	FILE *f = popen(cmd, "r");
	if (f == NULL) {
		send_str(fd, "+ERR pm failed\n");
		return;
	}
	char line[1024];
	int ok = 0;
	while (fgets(line, sizeof(line), f) != NULL) {
		if (strstr(line, "Success")) {
			ok = 1;
		}
		send_str(fd, "LOG ");
		send_str(fd, line);
	}
	pclose(f);
	unlink(path);
	send_str(fd, ok ? "+OK\n" : "+ERR install rejected\n");
}

static void *
client_main(void *arg)
{
	int fd = (int)(intptr_t)arg;
	send_str(fd, CTE_BANNER);

	// one command line, then the socket belongs to that command
	char cmd[512];
	size_t n = 0;
	while (n < sizeof(cmd) - 1) {
		uint8_t c;
		ssize_t r = recv(fd, &c, 1, 0);
		if (r <= 0) {
			close(fd);
			return NULL;
		}
		if (c == '\n') {
			break;
		}
		if (c != '\r') {
			cmd[n++] = (char)c;
		}
	}
	cmd[n] = 0;

	if (!strcmp(cmd, "PING")) {
		send_str(fd, "+PONG\n");
	} else if (!strcmp(cmd, "INFO")) {
		cmd_info(fd);
	} else if (!strcmp(cmd, "PROPS")) {
		cmd_props(fd);
	} else if (!strcmp(cmd, "FRAMES")) {
		cmd_frames(fd);
	} else if (!strcmp(cmd, "POSE")) {
		cmd_pose(fd);
	} else if (!strcmp(cmd, "CTRL")) {
		cmd_ctrl(fd);
	} else if (!strcmp(cmd, "LOG")) {
		cmd_log(fd);
	} else if (!strncmp(cmd, "INSTALL ", 8)) {
		cmd_install(fd, (size_t)strtoull(cmd + 8, NULL, 10));
	} else if (n) {
		sendf(fd, "+ERR unknown command\n");
	}
	close(fd);
	return NULL;
}

int
main(void)
{
	signal(SIGPIPE, SIG_IGN);
	int port = CTE_PORT_DEFAULT;
	const char *env = getenv("CTE_PORT");
	if (env && atoi(env) > 0) {
		port = atoi(env);
	}

	int srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) {
		perror("socket");
		return 1;
	}
	int one = 1;
	setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}
	if (listen(srv, 8) < 0) {
		perror("listen");
		return 1;
	}
	fprintf(stderr, "cted listening on :%d\n", port);

	for (;;) {
		int fd = accept(srv, NULL, NULL);
		if (fd < 0) {
			continue;
		}
		pthread_t th;
		if (pthread_create(&th, NULL, client_main, (void *)(intptr_t)fd) == 0) {
			pthread_detach(th);
		} else {
			close(fd);
		}
	}
}
