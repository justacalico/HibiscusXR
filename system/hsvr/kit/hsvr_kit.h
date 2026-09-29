// Copyright 2025, PN2Lineage
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvr driver kit - the ABI every Hibiscus headset driver is
 *         written against.
 *
 * Each device under drivers/<name>/monado/ exports exactly one symbol,
 * `hsvr_drv_<name>`, describing how to detect the hardware and how to
 * create its devices. The single monado-side prober (monado/driver/hsvr)
 * walks hsvr_drivers[], calls each probe(), and instantiates the winner.
 * Nothing in system/ or in the monado patch changes when a device is
 * added - the build globs the drivers tree into drv_hsvr and generates
 * the table.
 *
 * Input devices that are not tied to one headset get their own tree:
 * controllers/<name>/monado/ exports `hsvr_ctrl_<name>`, and the prober
 * attaches every generic controller that probed (or was forced) after
 * the winning driver's native controllers. Porting the OS to a new
 * headset then never means porting its controllers.
 *
 * Devices still return plain monado xrt_device objects: the kit is the
 * seam for discovery and ownership, not a second device model. Fuse
 * sensors, track and distort inside the driver exactly like before.
 * @ingroup drv_hsvr
 */

#pragma once

#include "xrt/xrt_device.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * One headset driver. Drivers live in drivers/<name>/monado/ and export
 * this as `const struct hsvr_driver hsvr_drv_<name>`.
 */
struct hsvr_driver
{
	//! Directory name under drivers/, e.g. "pn2".
	const char *name;

	/*!
	 * Hardware match strength: >0 when this device is present, highest
	 * score wins. Must be cheap and side-effect free - it runs for every
	 * linked driver at each probe.
	 */
	int (*probe)(void);

	/*!
	 * Create the HMD xrt_device. NULL leaves the seat open for a lower
	 * scoring driver.
	 */
	struct xrt_device *(*create_hmd)(void);

	/*!
	 * Create controller @p index (0,1,2,...). Return NULL when the driver
	 * has no more controllers - may be NULL itself for HMD-only drivers.
	 */
	struct xrt_device *(*create_controller)(int index);
};

/*!
 * Generated table of every driver linked into the build - emitted as
 * hsvr_drivers.c by monado/build.sh from the drivers/ tree.
 */
extern const struct hsvr_driver *const hsvr_drivers[];

/*!
 * Runs probe() on every linked driver and returns the highest scorer,
 * or NULL when nothing claims this hardware.
 */
const struct hsvr_driver *
hsvr_driver_pick(void);

/*!
 * Looks up a driver by name - debugging and forced selection.
 */
const struct hsvr_driver *
hsvr_driver_find(const char *name);


/*!
 * One headset-independent controller type. Controllers live in
 * controllers/<name>/monado/ and export this as
 * `const struct hsvr_controller hsvr_ctrl_<name>`.
 *
 * A paired Wii Remote, a Bluetooth gamepad, anything that is not part
 * of the headset's own tracking stack belongs here: it attaches to
 * whatever driver won the HMD probe and carries over to new hardware
 * unchanged.
 */
struct hsvr_controller
{
	//! Directory name under controllers/, e.g. "wii".
	const char *name;

	/*!
	 * >0 when this controller is usable right now (paired, in range).
	 * Must be cheap and side-effect free like hsvr_driver::probe. NULL
	 * means the controller only activates through the HSVR_CTRL
	 * force list.
	 */
	int (*probe)(void);

	/*!
	 * Create controller @p index (0,1,2,...). Same contract as
	 * hsvr_driver::create_controller: NULL ends the list.
	 */
	struct xrt_device *(*create)(int index);
};

/*!
 * Generated table of every controller linked into the build - emitted
 * as hsvr_controllers.c by monado/build.sh from the controllers/ tree.
 */
extern const struct hsvr_controller *const hsvr_controllers[];

/*!
 * Looks up a controller by name - debugging and forced selection.
 */
const struct hsvr_controller *
hsvr_controller_find(const char *name);


#ifdef __cplusplus
}
#endif
