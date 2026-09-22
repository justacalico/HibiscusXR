# Hibiscus

LineageOS 17.1 (Android 10) running on the Pico Neo 2 VR headset
(codename `A7B10`, Snapdragon 845, stock PUI 4.1.3 / Android 8.1 vendor).

This site documents the whole project: what each repository holds, how the
port works, how to build and flash it, and how to reproduce every dumped file
from hardware you own.

!!! warning "Flashing risk"
    This project involves flashing partition images and patched system
    software. Follow the guides exactly - writing an older bootloader than the
    anti-rollback fuse allows is a permanent hard-brick on sdm845.

## What this is

Stock PUI 4.1.3 is Android 8.1 with Pico's proprietary VR stack on top.
Hibiscus replaces the system partition with a LineageOS 17.1 GSI plus a
small overlay of our own fixes, then layers Pico's VR stack back on - the
proprietary parts come from **your own device**, never from this repo.

## Current status

The port boots and runs VRShell (the VR home) with live head rotation, audio,
suspend matching stock behaviour, and the 2D Pico apps rendering. The one
remaining blocker is the VR display staying black - see
[status](internals/status.md).

## The repositories

Everything lives in the [HibiscusXR monorepo](https://gitlab.com/neosalsa/HibiscusXR).
See the [repository map](repos/index.md) - one tree per component.
