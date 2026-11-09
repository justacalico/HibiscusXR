// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file unit tests for the wire codec + kv helpers */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "hsvc/proto.h"

static void
test_hdr_roundtrip(void)
{
	uint8_t buf[HSVC_HDR_LEN];
	struct hsvc_hdr h;
	hsvc_hdr_encode(buf, HSVC_MSG_STREAM, 0x0102030405060708ull, 1234);
	assert(hsvc_hdr_decode(buf, &h) == 0);
	assert(h.magic == HSVC_MAGIC);
	assert(h.version == HSVC_VERSION);
	assert(h.type == HSVC_MSG_STREAM);
	assert(h.seq == 0x0102030405060708ull);
	assert(h.len == 1234);
}

static void
test_hdr_bad(void)
{
	uint8_t buf[HSVC_HDR_LEN];
	struct hsvc_hdr h;
	hsvc_hdr_encode(buf, HSVC_MSG_STREAM, 0, 0);
	buf[0] ^= 0xff;
	assert(hsvc_hdr_decode(buf, &h) < 0);
	hsvc_hdr_encode(buf, HSVC_MSG_STREAM, 0, HSVC_MAX_PAYLOAD + 1);
	assert(hsvc_hdr_decode(buf, &h) < 0);
}

static void
test_feed_partial(void)
{
	uint8_t buf[128], frame[64];
	size_t used = 0, n;
	n = hsvc_hdr_encode(frame, HSVC_MSG_SUB, 7, 4);
	hsvc_put32(frame + n, 42);
	/* feed in two halves */
	assert(hsvc_feed(buf, sizeof(buf), &used, frame, 10) == 0);
	int r = hsvc_feed(buf, sizeof(buf), &used, frame + 10, 14);
	assert(r == 24);
	struct hsvc_hdr h;
	assert(hsvc_hdr_decode(buf, &h) == 0);
	assert(h.type == HSVC_MSG_SUB && h.seq == 7 && h.len == 4);
	assert(hsvc_get32(buf + HSVC_HDR_LEN) == 42);
}

static void
test_feed_errors(void)
{
	uint8_t buf[64];
	size_t used = 0;
	uint8_t big[80];
	memset(big, 0, sizeof(big));
	assert(hsvc_feed(buf, sizeof(buf), &used, big, sizeof(big)) == -1);
	used = 0;
	uint8_t bad[HSVC_HDR_LEN] = {0};
	assert(hsvc_feed(buf, sizeof(buf), &used, bad, sizeof(bad)) == -1);
}

static void
test_feed_byte_at_a_time(void)
{
	uint8_t buf[128], frame[40];
	size_t used = 0, n;
	n = hsvc_hdr_encode(frame, HSVC_MSG_PING, 1, 3);
	memcpy(frame + n, "abc", 3);
	int r = 0;
	for (size_t i = 0; i < n + 3; i++)
		r = hsvc_feed(buf, sizeof(buf), &used, frame + i, 1);
	assert(r == (int)n + 3);
}

static void
test_kv(void)
{
	char kv[256];
	size_t used = 0;
	assert(hsvc_kv_put(kv, sizeof(kv), &used, "name", "eye") == 0);
	assert(hsvc_kv_put_u32(kv, sizeof(kv), &used, "kind", 3) == 0);
	char out[64];
	assert(hsvc_kv_get_str(kv, "name", out, sizeof(out)) == 0);
	assert(strcmp(out, "eye") == 0);
	uint32_t v;
	assert(hsvc_kv_get_u32(kv, "kind", &v) == 0 && v == 3);
	assert(hsvc_kv_get_str(kv, "nope", out, sizeof(out)) < 0);
	/* truncation guard */
	assert(hsvc_kv_get(kv, used, "name", out, 2) < 0);
	/* value with missing = is not a match for a prefix key */
	char kv2[64];
	size_t u2 = 0;
	hsvc_kv_put(kv2, sizeof(kv2), &u2, "namesake", "x");
	assert(hsvc_kv_get_str(kv2, "name", out, sizeof(out)) < 0);
}

int
main(void)
{
	test_hdr_roundtrip();
	test_hdr_bad();
	test_feed_partial();
	test_feed_errors();
	test_feed_byte_at_a_time();
	test_kv();
	puts("test_proto OK");
	return 0;
}
