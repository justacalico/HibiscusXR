// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvcd - the service broker daemon. The loop itself lives in
 *         broker.c so it can be embedded elsewhere.
 */
#include "hsvc/hsvc.h"

int
main(void)
{
	return hsvc_broker_run();
}
