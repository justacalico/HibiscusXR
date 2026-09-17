// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pico Neo 2 prober: claims the device only on PICOA7B10 hardware.
 * @ingroup drv_pn2
 */

#include "pn2_interface.h"
#include "pn2_hmd.h"

#include "util/u_misc.h"

#include <stdlib.h>
#include <string.h>


/*!
 * @implements xrt_auto_prober
 */
struct pn2_prober
{
	struct xrt_auto_prober base;
};

static inline struct pn2_prober *
pn2_prober(struct xrt_auto_prober *p)
{
	return (struct pn2_prober *)p;
}

static void
pn2_prober_destroy(struct xrt_auto_prober *p)
{
	struct pn2_prober *dp = pn2_prober(p);
	free(dp);
}

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
pn2_prober_autoprobe(struct xrt_auto_prober *xap,
                     cJSON *attached_data,
                     bool no_hmds,
                     struct xrt_prober *xp,
                     struct xrt_device **out_xdevs)
{
	if (no_hmds || !pn2_is_neo2()) {
		return 0;
	}
	struct xrt_device *d = pn2_hmd_create();
	if (d == NULL) {
		return 0;
	}
	out_xdevs[0] = d;
	return 1;
}

struct xrt_auto_prober *
pn2_create_auto_prober(void)
{
	struct pn2_prober *p = U_TYPED_CALLOC(struct pn2_prober);
	p->base.name = "Pico Neo 2";
	p->base.destroy = pn2_prober_destroy;
	p->base.lelo_dallas_autoprobe = pn2_prober_autoprobe;
	return &p->base;
}
