// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvr auto prober: asks the driver kit which headset this is and
 *         instantiates its devices, then attaches every generic
 *         controller that claimed the run. Headsets live in
 *         drivers/<name>/, shared controllers in controllers/<name>/ -
 *         both register through the kit.
 * @ingroup drv_hsvr
 */

#include "hsvr_interface.h"
#include "hsvr_kit.h"

#include "util/u_misc.h"
#include "util/u_debug.h"

#include <stdlib.h>
#include <string.h>


DEBUG_GET_ONCE_OPTION(hsvr_driver, "HSVR_DRIVER", NULL)
DEBUG_GET_ONCE_OPTION(hsvr_ctrl, "HSVR_CTRL", NULL)

// per-type create() cap, same bound the kit uses for native controllers
#define HSVR_CTRL_MAX_PER_TYPE 8

// is name one of the entries in a space/comma separated HSVR_CTRL list
static bool
hsvr_ctrl_forced(const char *list, const char *name)
{
	if (list == NULL || name == NULL) {
		return false;
	}
	size_t len = strlen(name);
	for (const char *p = list; *p != '\0';) {
		while (*p == ' ' || *p == ',' || *p == ';' || *p == ':') {
			p++;
		}
		const char *e = p;
		while (*e != '\0' && *e != ' ' && *e != ',' && *e != ';' && *e != ':') {
			e++;
		}
		if ((size_t)(e - p) == len && strncmp(p, name, len) == 0) {
			return true;
		}
		p = e;
	}
	return false;
}

// attach every generic controller that probed or was force-listed,
// filling out_xdevs from *io_n up to the probe capacity
static void
hsvr_prober_attach_controllers(struct xrt_device **out_xdevs, int *io_n)
{
	const char *forced = debug_get_option_hsvr_ctrl();
	for (size_t c = 0; hsvr_controllers[c] != NULL; c++) {
		const struct hsvr_controller *ctrl = hsvr_controllers[c];
		bool on = hsvr_ctrl_forced(forced, ctrl->name);
		if (!on && (ctrl->probe == NULL || ctrl->probe() <= 0)) {
			continue;
		}
		if (ctrl->create == NULL) {
			continue;
		}
		for (int i = 0; i < HSVR_CTRL_MAX_PER_TYPE; i++) {
			if (*io_n >= XRT_MAX_DEVICES_PER_PROBE) {
				return;
			}
			struct xrt_device *dev = ctrl->create(i);
			if (dev == NULL) {
				break;
			}
			out_xdevs[(*io_n)++] = dev;
		}
	}
}

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
	// native controllers first, generic ones fill whatever slots remain
	if (drv->create_controller != NULL) {
		for (int i = 0; i < HSVR_CTRL_MAX_PER_TYPE && n < XRT_MAX_DEVICES_PER_PROBE; i++) {
			struct xrt_device *c = drv->create_controller(i);
			if (c == NULL) {
				break;
			}
			out_xdevs[n++] = c;
		}
	}
	hsvr_prober_attach_controllers(out_xdevs, &n);
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
