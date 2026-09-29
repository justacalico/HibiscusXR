// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvr driver kit registry.
 * @ingroup drv_hsvr
 */

#include "hsvr_kit.h"

#include <string.h>


const struct hsvr_driver *
hsvr_driver_pick(void)
{
	const struct hsvr_driver *best = NULL;
	int best_score = 0;
	for (size_t i = 0; hsvr_drivers[i] != NULL; i++) {
		const struct hsvr_driver *d = hsvr_drivers[i];
		if (d->probe == NULL) {
			continue;
		}
		int score = d->probe();
		if (score > best_score) {
			best_score = score;
			best = d;
		}
	}
	return best;
}

const struct hsvr_driver *
hsvr_driver_find(const char *name)
{
	if (name == NULL) {
		return NULL;
	}
	for (size_t i = 0; hsvr_drivers[i] != NULL; i++) {
		if (strcmp(hsvr_drivers[i]->name, name) == 0) {
			return hsvr_drivers[i];
		}
	}
	return NULL;
}

const struct hsvr_controller *
hsvr_controller_find(const char *name)
{
	if (name == NULL) {
		return NULL;
	}
	for (size_t i = 0; hsvr_controllers[i] != NULL; i++) {
		if (strcmp(hsvr_controllers[i]->name, name) == 0) {
			return hsvr_controllers[i];
		}
	}
	return NULL;
}
