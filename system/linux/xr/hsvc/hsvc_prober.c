// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  drv_hsvc auto prober: whatever the bus serves is the device.
 *         An HMD service answers RESOLVE hmd, controllers are every
 *         CTRL service (or every stream a multi-controller service
 *         publishes). Nothing hardware-specific lives here - the device
 *         profile chose the drivers, this just reflects them.
 * @ingroup drv_hsvc
 */

#include "hsvc_interface.h"
#include "hsvc_internal.h"

#include "util/u_debug.h"

#include <stdlib.h>
#include <string.h>


#define HSVCX_CTRL_MAX 8

/*!
 * @implements xrt_auto_prober
 */
struct hsvcx_prober
{
	struct xrt_auto_prober base;
};

static inline struct hsvcx_prober *
hsvcx_prober(struct xrt_auto_prober *p)
{
	return (struct hsvcx_prober *)p;
}

static void
hsvcx_prober_destroy(struct xrt_auto_prober *p)
{
	free(hsvcx_prober(p));
}

/* controllers already claimed by earlier entries in `svcs` must not be
 * re-served, so each (service, stream) pair opens its own connection */
static int
hsvcx_prober_ctrls(struct xrt_device **out_xdevs, int *io_n)
{
	struct hsvcx_svc svcs[HSVCX_CTRL_MAX];
	int nsvc = hsvcx_list(HSVC_KIND_CTRL, svcs, HSVCX_CTRL_MAX);
	int idx = 0;
	for (int s = 0; s < nsvc; s++) {
		struct hsvcx_conn probe;
		if (hsvcx_connect_path(svcs[s].path, &probe) < 0)
			continue;
		uint32_t n = 1;
		hsvc_kv_get_u32(probe.desc, "n", &n);
		hsvcx_close(&probe);
		if (n > HSVCX_CTRL_MAX)
			n = HSVCX_CTRL_MAX;
		for (uint32_t i = 0; i < n; i++) {
			if (*io_n >= XRT_MAX_DEVICES_PER_PROBE)
				return idx;
			struct hsvcx_conn conn;
			if (hsvcx_connect_path(svcs[s].path, &conn) < 0)
				break;
			struct xrt_device *dev =
			    hsvcx_ctrl_create(&conn, i, idx);
			if (dev == NULL) {
				hsvcx_close(&conn);
				break;
			}
			out_xdevs[(*io_n)++] = dev;
			idx++;
		}
	}
	return idx;
}

static int
hsvcx_prober_autoprobe(struct xrt_auto_prober *xap,
                       cJSON *attached_data,
                       bool no_hmds,
                       struct xrt_prober *xp,
                       struct xrt_device **out_xdevs)
{
	int n = 0;
	if (!no_hmds) {
		struct hsvcx_conn conn;
		if (hsvcx_connect_kind(HSVC_KIND_HMD, &conn) == 0) {
			struct xrt_device *hmd = hsvcx_hmd_create(&conn);
			if (hmd != NULL) {
				out_xdevs[n++] = hmd;
			}
			hsvcx_close(&conn);
		}
	}
	hsvcx_prober_ctrls(out_xdevs, &n);
	return n;
}

struct xrt_auto_prober *
hsvc_create_auto_prober(void)
{
	struct hsvcx_prober *p = calloc(1, sizeof(*p));
	p->base.name = "hsvc";
	p->base.lelo_dallas_autoprobe = hsvcx_prober_autoprobe;
	p->base.destroy = hsvcx_prober_destroy;
	return &p->base;
}
