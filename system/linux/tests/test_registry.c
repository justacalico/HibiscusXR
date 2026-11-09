// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*! @file unit tests for the registry */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "hsvc/registry.h"
#include "hsvc/proto.h"

static void
test_add_resolve(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	assert(hsvc_reg_add(&r, "eye", HSVC_KIND_EYE, "/run/x/eye.sock", 5) > 0);
	assert(hsvc_reg_add(&r, "hmd", HSVC_KIND_HMD, "/run/x/hmd.sock", 5) > 0);
	char path[256];
	assert(hsvc_reg_resolve(&r, HSVC_KIND_EYE, path, sizeof(path)) == 0);
	assert(strcmp(path, "/run/x/eye.sock") == 0);
	assert(hsvc_reg_resolve(&r, HSVC_KIND_CAM, path, sizeof(path)) < 0);
}

static void
test_first_wins(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	hsvc_reg_add(&r, "a", HSVC_KIND_IMU, "/a.sock", -1);
	hsvc_reg_add(&r, "b", HSVC_KIND_IMU, "/b.sock", -1);
	char path[256];
	assert(hsvc_reg_resolve(&r, HSVC_KIND_IMU, path, sizeof(path)) == 0);
	assert(strcmp(path, "/a.sock") == 0);
}

static void
test_replace_and_del(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	hsvc_reg_add(&r, "eye", HSVC_KIND_EYE, "/old.sock", -1);
	hsvc_reg_add(&r, "eye", HSVC_KIND_EYE, "/new.sock", -1);
	assert(r.n == 1);
	char path[256];
	assert(hsvc_reg_resolve(&r, HSVC_KIND_EYE, path, sizeof(path)) == 0);
	assert(strcmp(path, "/new.sock") == 0);
	assert(hsvc_reg_del(&r, "eye") == 0);
	assert(hsvc_reg_del(&r, "eye") < 0);
	assert(hsvc_reg_resolve(&r, HSVC_KIND_EYE, path, sizeof(path)) < 0);
}

static void
test_drop_owner(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	hsvc_reg_add(&r, "eye", HSVC_KIND_EYE, "/e.sock", 9);
	hsvc_reg_add(&r, "imu", HSVC_KIND_IMU, "/i.sock", 9);
	hsvc_reg_add(&r, "hmd", HSVC_KIND_HMD, "/h.sock", 4);
	assert(hsvc_reg_drop_owner(&r, 9) == 2);
	assert(r.n == 1);
	char path[256];
	assert(hsvc_reg_resolve(&r, HSVC_KIND_HMD, path, sizeof(path)) == 0);
}

static void
test_list_order(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	hsvc_reg_add(&r, "b", HSVC_KIND_CTRL, "/b", -1);
	hsvc_reg_add(&r, "a", HSVC_KIND_EYE, "/a", -1);
	char out[1024];
	int n = hsvc_reg_list(&r, out, sizeof(out));
	/* b registered first so it lists first */
	assert(n > 0 && strncmp(out, "b 2 /b\na 3 /a\n", n) == 0);
}

static void
test_limits(void)
{
	struct hsvc_registry r;
	hsvc_reg_init(&r);
	char name[16], path[16];
	for (int i = 0; i < HSVC_REG_MAX; i++) {
		snprintf(name, sizeof(name), "s%d", i);
		snprintf(path, sizeof(path), "/s%d", i);
		assert(hsvc_reg_add(&r, name, 1, path, -1) > 0);
	}
	assert(hsvc_reg_add(&r, "overflow", 1, "/x", -1) < 0);
	assert(hsvc_reg_add(&r, "", 1, "/x", -1) < 0);
	assert(hsvc_reg_add(&r, "x", 0, "/x", -1) < 0);
}

int
main(void)
{
	test_add_resolve();
	test_first_wins();
	test_replace_and_del();
	test_drop_owner();
	test_list_order();
	test_limits();
	puts("test_registry OK");
	return 0;
}
