// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc wire protocol - the one contract between hardware
 *         services and the OS.
 *
 * Drivers are daemons that own hardware and serve typed interfaces
 * over unix sockets under /run/hibiscus/svc/. The OS links libhsvc and
 * only ever sees this protocol, never the device. A headset, a dev PC,
 * or the simulator all look identical from the OS side.
 *
 * Frame layout, little-endian, 20-byte header:
 *
 *   u32 magic    'H''S''V''C'
 *   u16 version  HSVC_VERSION
 *   u16 type     HSVC_MSG_*
 *   u64 seq      sender's own sequence counter
 *   u32 len      payload bytes that follow
 *   u8  payload[len]
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HSVC_MAGIC 0x43565348u /* "HSVC" */
#define HSVC_VERSION 1u
#define HSVC_HDR_LEN 20u
#define HSVC_MAX_PAYLOAD (4u * 1024u * 1024u)

#define HSVC_RUNDIR_DEFAULT "/run/hibiscus"
#define HSVC_DAEMON_SOCK "daemon.sock"
#define HSVC_SVCDIR "svc"

enum hsvc_msg
{
	HSVC_MSG_HELLO = 0,  /* client -> service: client name */
	HSVC_MSG_INFO = 1,   /* service -> client: descriptor kv text */
	HSVC_MSG_SUB = 2,    /* client -> service: u32 stream id */
	HSVC_MSG_UNSUB = 3,  /* client -> service: u32 stream id */
	HSVC_MSG_STREAM = 4, /* service -> client: stream sample */
	HSVC_MSG_CALL = 5,   /* client -> service: u32 call id + u32 op + args */
	HSVC_MSG_REPLY = 6,  /* service -> client: u32 call id + i32 status + data */
	HSVC_MSG_EVENT = 7,  /* hsvcd -> watchers: service up/down */
	HSVC_MSG_PING = 8,
	HSVC_MSG_PONG = 9,
	HSVC_MSG_ERR = 15
};

/* Daemon control ops (payload of CALL frames to hsvcd). */
enum hsvc_op
{
	HSVC_OP_REGISTER = 1,   /* args: name, kind, sock path */
	HSVC_OP_RESOLVE = 2,    /* args: kind -> data: sock path or empty */
	HSVC_OP_LIST = 3,       /* -> data: "name kind path" lines */
	HSVC_OP_WATCH = 4,      /* connection upgrades to event stream */
	HSVC_OP_UNREGISTER = 5  /* args: name */
};

/* Well-known service kinds. Drivers pick one; the OS resolves by kind,
 * never by driver name - that is what makes drivers interchangeable. */
enum hsvc_kind
{
	HSVC_KIND_HMD = 1,  /* head pose + display/lens info */
	HSVC_KIND_CTRL = 2, /* controller pose + buttons + battery */
	HSVC_KIND_EYE = 3,  /* eye tracking / gaze */
	HSVC_KIND_IMU = 4,  /* raw gyro/accel samples */
	HSVC_KIND_CAM = 5,  /* camera frames (passthrough, tracking) */
	HSVC_KIND_DISP = 6  /* display panel state */
};

struct hsvc_hdr
{
	uint32_t magic;
	uint16_t version;
	uint16_t type;
	uint64_t seq;
	uint32_t len;
};

/* Stream payload common prefix inside HSVC_MSG_STREAM payloads. */
struct hsvc_sample
{
	uint32_t stream;
	uint64_t ts_ns;
	/* stream-specific data follows */
};

/* stream payloads */
struct hsvc_pose /* kind HMD, stream 0 */
{
	float px, py, pz;
	float qx, qy, qz, qw;
	uint32_t state; /* 0=none 1=orientation 3=position+orientation */
};

struct hsvc_gaze /* kind EYE, stream 0 */
{
	float lx, ly; /* left eye gaze, -1..1 panel space */
	float rx, ry;
	uint32_t valid; /* bitmask: 1 left, 2 right */
};

struct hsvc_imu /* kind IMU, stream 0 */
{
	float gx, gy, gz; /* rad/s */
	float ax, ay, az; /* m/s^2 */
};

struct hsvc_ctrl /* kind CTRL, stream 0; one per controller (stream = idx) */
{
	float px, py, pz;
	float qx, qy, qz, qw;
	float stick_x, stick_y, trigger, grip;
	uint32_t buttons; /* bitmask, HSVC_BTN_* */
	uint8_t battery;  /* 0..100, 255 unknown */
	uint8_t tracking; /* 0=none 1=orientation 3=6dof */
	uint8_t connected;
};

#define HSVC_BTN_A (1u << 0)
#define HSVC_BTN_B (1u << 1)
#define HSVC_BTN_MENU (1u << 2)
#define HSVC_BTN_SYS (1u << 3)
#define HSVC_BTN_STICK (1u << 4)
#define HSVC_BTN_TRIG (1u << 5)
#define HSVC_BTN_GRIP (1u << 6)

struct hsvc_frame_hdr
{
	uint32_t width, height, fourcc;
	uint32_t stride, data_len;
	/* pixel data follows */
};

/* ------------------------- codec ------------------------- */

static inline void
hsvc_put16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
}

static inline void
hsvc_put32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

static inline void
hsvc_put64(uint8_t *p, uint64_t v)
{
	for (int i = 0; i < 8; i++)
		p[i] = (uint8_t)(v >> (8 * i));
}

static inline uint16_t
hsvc_get16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static inline uint32_t
hsvc_get32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint64_t
hsvc_get64(const uint8_t *p)
{
	uint64_t v = 0;
	for (int i = 0; i < 8; i++)
		v |= (uint64_t)p[i] << (8 * i);
	return v;
}

/* Encode a header. Returns HSVC_HDR_LEN. */
static inline size_t
hsvc_hdr_encode(uint8_t *out, uint16_t type, uint64_t seq, uint32_t len)
{
	hsvc_put32(out + 0, HSVC_MAGIC);
	hsvc_put16(out + 4, HSVC_VERSION);
	hsvc_put16(out + 6, type);
	hsvc_put64(out + 8, seq);
	hsvc_put32(out + 16, len);
	return HSVC_HDR_LEN;
}

/* Decode a header. 0 ok, -1 bad magic/version. */
static inline int
hsvc_hdr_decode(const uint8_t *in, struct hsvc_hdr *h)
{
	h->magic = hsvc_get32(in + 0);
	h->version = hsvc_get16(in + 4);
	h->type = hsvc_get16(in + 6);
	h->seq = hsvc_get64(in + 8);
	h->len = hsvc_get32(in + 16);
	if (h->magic != HSVC_MAGIC || h->version != HSVC_VERSION ||
	    h->len > HSVC_MAX_PAYLOAD)
		return -1;
	return 0;
}

/*
 * Feed bytes into a stream decoder. State is just a byte counter:
 * call with each chunk; returns the payload length when a full frame
 * is buffered in `buf` (buf holds the whole frame, header first), 0
 * when more bytes are needed, -1 on protocol error. `cap` bounds the
 * frame size the caller will accept.
 */
static inline int
hsvc_feed(uint8_t *buf, size_t cap, size_t *used, const uint8_t *in,
          size_t in_len)
{
	if (in_len == 0)
		return 0;
	if (*used + in_len > cap)
		return -1;
	memcpy(buf + *used, in, in_len);
	*used += in_len;
	if (*used < HSVC_HDR_LEN)
		return 0;
	struct hsvc_hdr h;
	if (hsvc_hdr_decode(buf, &h) < 0)
		return -1;
	if (*used < HSVC_HDR_LEN + h.len)
		return 0;
	return (int)(HSVC_HDR_LEN + h.len);
}

/* --------------------- descriptor kv text --------------------- */
/*
 * INFO payloads and daemon REGISTER args are newline-separated
 * "key=value" lines. Helpers below read/write that format.
 */

/* Find `key` in a kv blob, copy its value to out (NUL-terminated).
 * Returns 0 on hit, -1 on miss or truncation. */
static inline int
hsvc_kv_get(const char *kv, size_t kv_len, const char *key, char *out,
            size_t out_len)
{
	size_t klen = strlen(key);
	size_t i = 0;
	while (i < kv_len) {
		size_t end = i;
		while (end < kv_len && kv[end] != '\n')
			end++;
		if (end - i > klen && kv[i + klen] == '=' &&
		    memcmp(kv + i, key, klen) == 0) {
			size_t vlen = end - i - klen - 1;
			if (vlen >= out_len)
				return -1;
			memcpy(out, kv + i + klen + 1, vlen);
			out[vlen] = '\0';
			return 0;
		}
		i = end + 1;
	}
	return -1;
}

static inline int
hsvc_kv_get_str(const char *kv, const char *key, char *out, size_t out_len)
{
	return hsvc_kv_get(kv, strlen(kv), key, out, out_len);
}

static inline int
hsvc_kv_get_u32(const char *kv, const char *key, uint32_t *out)
{
	char tmp[16];
	if (hsvc_kv_get_str(kv, key, tmp, sizeof(tmp)) < 0)
		return -1;
	char *end = NULL;
	unsigned long v = strtoul(tmp, &end, 10);
	if (end == tmp || *end != '\0')
		return -1;
	*out = (uint32_t)v;
	return 0;
}

/* Append "key=value\n" to a growing kv blob. */
static inline int
hsvc_kv_put(char *kv, size_t cap, size_t *used, const char *key,
            const char *value)
{
	int n = snprintf(kv + *used, cap - *used, "%s=%s\n", key, value);
	if (n < 0 || (size_t)n >= cap - *used)
		return -1;
	*used += (size_t)n;
	return 0;
}

static inline int
hsvc_kv_put_u32(char *kv, size_t cap, size_t *used, const char *key,
                uint32_t value)
{
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%u", value);
	return hsvc_kv_put(kv, cap, used, key, tmp);
}
