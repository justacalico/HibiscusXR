// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvr auto prober: asks the driver kit which headset this is and
 *         instantiates its devices. The only monado-side driver - every
 *         device lives in drivers/<name>/ and registers through the kit.
 * @ingroup drv_hsvr
 */

#include "hsvr_interface.h"
#include "hsvr_kit.h"

#include "util/u_misc.h"
#include "util/u_debug.h"

#include <stdlib.h>


DEBUG_GET_ONCE_OPTION(hsvr_driver, "HSVR_DRIVER", NULL)

/*!
 * @implements xrt_auto_prober
 */
struct hsvr_prober
{
	struct xrt_auto_prober base;
};

static inline struct hsvr_prober *
hsvr_prober(struct xrt_auto_prober *p)
{
	return (struct hsvr_prober *)p;
}

static void
hsvr_prober_destroy(struct xrt_auto_prober *p)
{
	struct hsvr_prober *dp = hsvr_prober(p);
	free(dp);
}

static int
hsvr_prober_autoprobe(struct xrt_auto_prober *xap,
                      cJSON *attached_data,
                      bool no_hmds,
                      struct xrt_prober *xp,
                      struct xrt_device **out_xdevs)
{
	if (no_hmds) {
		return 0;
	}

	const struct hsvr_driver *drv = NULL;
	// HSVR_DRIVER pins the pick for bring-up and for vmd runs where the
	// host always wants the same driver regardless of probe order.
	const char *forced = debug_get_option_hsvr_driver();
	if (forced != NULL && forced[0] != '\0') {
		drv = hsvr_driver_find(forced);
	} else {
		drv = hsvr_driver_pick();
	}
	if (drv == NULL || drv->create_hmd == NULL) {
		return 0;
	}

	struct xrt_device *hmd = drv->create_hmd();
	if (hmd == NULL) {
		return 0;
	}

	int n = 0;
	out_xdevs[n++] = hmd;
	if (drv->create_controller != NULL) {
		for (int i = 0; i < 8; i++) {
			struct xrt_device *c = drv->create_controller(i);
			if (c == NULL) {
				break;
			}
			out_xdevs[n++] = c;
		}
	}
	return n;
}

struct xrt_auto_prober *
hsvr_create_auto_prober(void)
{
	struct hsvr_prober *p = U_TYPED_CALLOC(struct hsvr_prober);
	p->base.name = "hsvr";
	p->base.destroy = hsvr_prober_destroy;
	p->base.lelo_dallas_autoprobe = hsvr_prober_autoprobe;
	return &p->base;
}
