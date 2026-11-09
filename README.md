<p align="center">
  <img src="hibiscusxr-icon.svg" width="96" alt="Hibiscus">
</p>

# Hibiscus

Hibiscus is a Linux distribution for standalone VR headsets. The idea is
one OS that runs on hardware from different vendors, and on any PC for
development, with drivers kept as services so porting to a new headset
stays manageable.

> [!WARNING]
> The Android-based build is deprecated and being removed as the Linux
> base reaches parity. New work goes into `system/linux/`; the
> `system/dist`, `system/hsvr` and `drivers/` trees remain only as a
> reference for device ports.

The hardware rule is one direction: hardware -> service -> OS. Drivers
are daemons that own devices (cameras, IMUs, controllers, panels) and
publish typed streams over the hsvc bus; the OS only ever talks to
services. See `system/linux/` for the bus, the generic drivers and the
image build, for x86_64 and aarch64.

The original Android-based build (`system/dist`, `system/hsvr`,
`drivers/`) is deprecated and being removed as the Linux base reaches
parity. New work targets `system/linux/`; the Android tree remains only
as the reference for the device ports until it is dropped.

**This is an early work in progress, not a finished product.** The tables
below show what works on each headset under the current build.

## Device compatibility

| Headset | Status |
| --- | --- |
| Pico Neo 2 | Supported, in development |
| Oculus Quest 1 | Planned |
| Pico Neo 3 | Planned |

### Pico Neo 2

| Feature | Status |
| --- | --- |
| Boot, 3840x2160 display | Working |
| Audio | Working |
| Suspend and resume | Working |
| Head tracking | Working, 3DoF and 6DoF SLAM |
| VR compositor | Working, async TimeWarp direct present |
| 2D Pico apps | Working |
| Controllers | Partial: pairing, buttons and battery work, no positional tracking |
| Passthrough | Not working |
| Software IPD adjustment | Working |
| Wireless adb | Working, opt-in |
| Setup wizard | Broken, disabled automatically on first boot |

## Before you install

Installing Hibiscus means flashing new software onto your headset. The
guide currently covers the Pico Neo 2. Follow it exactly: writing the
wrong partition, or a bootloader older than the device allows, can
permanently brick the headset. Backing up the stock software first is
covered in the guide as well.

## Getting started

The flashing guide lives on the
[project site](https://hibiscusxr-37c6a7.gitlab.io/#/flashdocs).

## Links

- Project site: https://hibiscusxr-37c6a7.gitlab.io/
