// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 driver-kit registration: claims the device only on
 *         PICOA7B10 hardware.
 * @ingroup drv_pn2
 */

#include "hsvr_kit.h"
#include "pn2_hmd.h"
#include "pn2_ctrl.h"

#include <stdlib.h>
#include <string.h>


static bool
pn2_is_neo2(void)
{
	if (getenv("PN2_FORCE") != NULL) {
		return true;
	}
	extern int __system_property_get(const char *, char *);
	static const char *keys[] = {
	    "ro.product.model", "ro.product.device", "ro.product.name", "ro.build.product",
	};
	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		char val[92] = {0};
		if (__system_property_get(keys[i], val) <= 0) {
			continue;
		}
		if (strstr(val, "A7B10") != NULL || strstr(val, "Pico Neo 2") != NULL ||
		    strstr(val, "PICOA7B10") != NULL) {
			return true;
		}
	}
	return false;
}

static int
pn2_probe(void)
{
	// the props alone cannot tell real hardware from the identical image
	// running in vmd, so the score stays below vmd's channel check
	return pn2_is_neo2() ? 100 : 0;
}

static struct xrt_device *
pn2_ctrl_for(int index)
{
	if (index < 0 || index > 1) {
		return NULL;
	}
	// NULL from create means the sharemem channel for that side is down;
	// the kit prober stops at the first NULL, so index 1 is still reached
	return pn2_ctrl_create(index);
}

const struct hsvr_driver hsvr_drv_pn2 = {
    .name = "pn2",
    .probe = pn2_probe,
    .create_hmd = pn2_hmd_create,
    .create_controller = pn2_ctrl_for,
};
