// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvcd's service table, kept pure: no sockets, no globals.
 *         Who serves what, first-registered-wins per (name) identity
 *         and per kind resolution order.
 */
#pragma once

#include <stdint.h>

#define HSVC_REG_MAX 64
#define HSVC_NAME_LEN 64
#define HSVC_PATH_LEN 256

struct hsvc_reg_entry
{
	char name[HSVC_NAME_LEN];
	uint32_t kind;
	char path[HSVC_PATH_LEN];
	uint64_t id; /* registration order, for LIST sort */
	int owner;   /* control-connection that registered it, -1 = cli */
};

struct hsvc_registry
{
	struct hsvc_reg_entry e[HSVC_REG_MAX];
	int n;
	uint64_t next_id;
};

void hsvc_reg_init(struct hsvc_registry *r);

/* Register a service. Re-registering the same name replaces the entry.
 * Returns entry id, or -1 when full / bad args. */
int64_t hsvc_reg_add(struct hsvc_registry *r, const char *name,
                     uint32_t kind, const char *path, int owner);

int hsvc_reg_del(struct hsvc_registry *r, const char *name);

/* Drop everything a disconnected control connection owned. */
int hsvc_reg_drop_owner(struct hsvc_registry *r, int owner);

/* Resolve a kind to a socket path: lowest registration id wins.
 * Returns 0 and fills `path`, or -1 when nothing serves the kind. */
int hsvc_reg_resolve(const struct hsvc_registry *r, uint32_t kind,
                     char *path, int path_len);

/* Serialize the table as "name kind path" lines into out. */
int hsvc_reg_list(const struct hsvc_registry *r, char *out, int out_len);
