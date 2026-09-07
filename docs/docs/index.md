# PN2Lineage

LineageOS 17.1 (Android 10) running on the Pico Neo 2 VR headset
(codename `A7B10`, Snapdragon 845, stock PUI 4.1.3 / Android 8.1 vendor).

This site is the documentation for the port: how the device works, how the
images are built, and how to reproduce every piece from hardware you own.

!!! warning
    This project involves flashing partition images and patched system
    software. Follow the guides exactly - a wrong bootloader flash can
    permanently brick the device.

## Status

The port boots and runs VRShell with live head tracking. The remaining blocker
is the VR display staying black (client-side tracking state reports zero).
See the device tree and notes repositories for the full breakdown.
