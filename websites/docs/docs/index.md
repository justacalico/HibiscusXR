# Hibiscus

Hibiscus is a custom operating system for standalone VR headsets, built
on LineageOS 17.1 (Android 10). One OS across hardware from different
vendors - the device-specific drivers and fixes live in their own trees,
so porting to a new headset means writing that layer, not forking the OS.

The **Pico Neo 2** (codename `A7B10`, Snapdragon 845, stock PUI 4.1.3 /
Android 8.1 vendor) is the first supported device and where all
development happens today.

| Headset | Status |
| --- | --- |
| Pico Neo 2 | Supported, in development |
| Oculus Quest 1 | Planned |
| Pico Neo 3 | Planned |

This site documents the whole project: what each repository holds, how the
Neo 2 port works, how to build and flash it, and how to reproduce every
dumped file from hardware you own.

!!! warning "Flashing risk"
    This project involves flashing partition images and patched system
    software. Follow the guides exactly - writing an older bootloader than the
    anti-rollback fuse allows is a permanent hard-brick on sdm845.

## What this is

On the Neo 2, stock PUI 4.1.3 is Android 8.1 with Pico's proprietary VR
stack on top. Hibiscus replaces the system partition with a LineageOS
17.1 GSI plus a small overlay of our own fixes, then layers Pico's VR
stack back on - the proprietary parts come from **your own device**,
never from this repo.

## Current status

On the Neo 2 the port boots and runs VRShell (the VR home) with live head
rotation, audio, suspend matching stock behaviour, the 2D Pico apps
rendering and a real picture on the VR display - see
[status](internals/status.md) for the full list.

## The repositories

Everything lives in the [HibiscusXR monorepo](https://gitlab.com/neosalsa/HibiscusXR).
See the [repository map](repos/index.md) - one tree per component.
