// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file registry - see include/hsvc/registry.h */

#include <stdio.h>
#include <string.h>

#include "hsvc/registry.h"

void
hsvc_reg_init(struct hsvc_registry *r)
{
	memset(r, 0, sizeof(*r));
	r->next_id = 1;
}

static int
find(const struct hsvc_registry *r, const char *name)
{
	for (int i = 0; i < r->n; i++)
		if (strcmp(r->e[i].name, name) == 0)
			return i;
	return -1;
}

int64_t
hsvc_reg_add(struct hsvc_registry *r, const char *name, uint32_t kind,
             const char *path, int owner)
{
	if (!name || !*name || strlen(name) >= HSVC_NAME_LEN ||
	    !path || strlen(path) >= HSVC_PATH_LEN || kind == 0)
		return -1;
	int i = find(r, name);
	if (i < 0) {
		if (r->n >= HSVC_REG_MAX)
			return -1;
		i = r->n++;
	}
	struct hsvc_reg_entry *e = &r->e[i];
	snprintf(e->name, sizeof(e->name), "%s", name);
	e->kind = kind;
	snprintf(e->path, sizeof(e->path), "%s", path);
	e->id = r->next_id++;
	e->owner = owner;
	return (int64_t)e->id;
}

int
hsvc_reg_del(struct hsvc_registry *r, const char *name)
{
	int i = find(r, name);
	if (i < 0)
		return -1;
	r->e[i] = r->e[--r->n];
	return 0;
}

int
hsvc_reg_drop_owner(struct hsvc_registry *r, int owner)
{
	int n = 0;
	for (int i = 0; i < r->n; i++) {
		if (r->e[i].owner == owner) {
			r->e[i] = r->e[--r->n];
			i--;
			n++;
		}
	}
	return n;
}

int
hsvc_reg_resolve(const struct hsvc_registry *r, uint32_t kind, char *path,
                 int path_len)
{
	const struct hsvc_reg_entry *best = NULL;
	for (int i = 0; i < r->n; i++) {
		if (r->e[i].kind != kind)
			continue;
		if (!best || r->e[i].id < best->id)
			best = &r->e[i];
	}
	if (!best)
		return -1;
	snprintf(path, path_len, "%s", best->path);
	return 0;
}

int
hsvc_reg_list(const struct hsvc_registry *r, char *out, int out_len)
{
	int used = 0;
	for (uint64_t id = 1; id < r->next_id; id++) {
		for (int i = 0; i < r->n; i++) {
			if (r->e[i].id != id)
				continue;
			int n = snprintf(out + used, out_len - used,
			                 "%s %u %s\n", r->e[i].name, r->e[i].kind,
			                 r->e[i].path);
			if (n < 0 || n >= out_len - used)
				return -1;
			used += n;
		}
	}
	return used;
}
