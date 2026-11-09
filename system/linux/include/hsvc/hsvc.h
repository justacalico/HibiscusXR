// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  libhsvc - the socket half of the hsvc service layer.
 *
 * Two sides:
 *
 *   service: hsvc_publish() binds svc/<name>.sock and registers with
 *            hsvcd; hsvc_serve() polls clients, answers INFO, tracks
 *            subscriptions, and pushes STREAM frames via the driver's
 *            emit() callback.
 *
 *   client:  hsvc_resolve() asks hsvcd who serves a kind, hsvc_dial()
 *            connects, hsvc_call/hsvc_sub and a read loop consume it.
 *
 * Everything honours HSVC_RUNDIR so tests and non-root runs get a
 * private bus under /tmp.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "hsvc/proto.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *hsvc_rundir(void);

/* Low-level frame io. Both block; hsvc_send_all retries short writes.
 * hsvc_recv returns the malloc'd payload (caller frees), or NULL on
 * EOF/error; hdr gets the decoded header. */
int hsvc_send_all(int fd, const void *buf, size_t n);
int hsvc_send_frame(int fd, uint16_t type, uint64_t seq, const void *payload,
                    uint32_t len);
int hsvc_recv_frame(int fd, struct hsvc_hdr *hdr, uint8_t **payload);

/* Unix socket plumbing. */
int hsvc_listen_at(const char *path);
int hsvc_dial(const char *path);

/* Resolve `kind` via hsvcd. Returns malloc'd sock path, NULL if absent. */
char *hsvc_resolve(enum hsvc_kind kind);

/* Connect to the service socket that serves `kind`. -1 when none. */
int hsvc_dial_kind(enum hsvc_kind kind);

/* REGISTER a service socket with hsvcd. 0 ok. */
int hsvc_register(const char *name, enum hsvc_kind kind, const char *path);
int hsvc_unregister(const char *name);

/*
 * One call on the daemon socket: sends CALL(op, args), reads REPLY.
 * Returns reply status (0 ok); `reply` gets malloc'd reply payload.
 */
int hsvc_daemon_call(uint32_t op, const void *args, uint32_t args_len,
                     uint8_t **reply, uint32_t *reply_len);

/* The broker loop: hsvcd's whole main, kept callable so a driver can
 * fork a private bus of its own (hsvc-vmd standalone mode). Listens on
 * hsvc_rundir()/daemon.sock forever; returns on fatal error. */
int hsvc_broker_run(void);

/* --------------------- service side --------------------- */

struct hsvc_service;

typedef void (*hsvc_emit_cb)(struct hsvc_service *s, void *userdata);

struct hsvc_service *
hsvc_publish(const char *name, enum hsvc_kind kind, const char *desc_kv);

/* Send one stream sample to every subscribed client. */
int hsvc_stream(struct hsvc_service *s, uint32_t stream, uint64_t ts_ns,
                const void *data, uint32_t len);

/*
 * Poll loop: accepts clients, serves INFO, processes SUB/UNSUB/PING and
 * CALL (driver ops are answered by the service's own handler, see
 * hsvc_set_call). Every `tick_ms` without traffic the emit callback
 * runs so drivers can push the next sample.
 */
typedef int (*hsvc_call_cb)(void *userdata, uint32_t op,
                            const uint8_t *args, uint32_t args_len,
                            uint8_t **reply, uint32_t *reply_len);

void hsvc_set_call(struct hsvc_service *s, hsvc_call_cb cb, void *userdata);

int hsvc_serve(struct hsvc_service *s, int tick_ms, hsvc_emit_cb emit,
               void *userdata);

/* Number of clients currently subscribed to `stream`. */
int hsvc_subscribers(struct hsvc_service *s, uint32_t stream);

void hsvc_destroy(struct hsvc_service *s);

#ifdef __cplusplus
}
#endif
