# Current status

## Works

- Boots, audio, landscape 3840x2160, no sensor aborts, suspend as stock
- `pvrservice` runs and publishes **live head rotation** (valid unit
  quaternions)
- `airservice`, `virtual_input` and `pn2_qvrd` all start
- **VRShell launches reliably** and drives the real Pico compositor: async
  TimeWarp, single-buffered direct present, EGL high priority, 16 layers, warp
  thread pinned to core 6, correct `hmdInfo` (3840x2160, lensSeparation 0.062,
  eyeTextureFov 80)
- See-through calibration app installed, platform-signed and launching
- 2D Pico apps render (VRUserCenter, Pico Store)
- **VR renders on the display.** VRShell presents frames to the panel with
  live head rotation tracking
- Optional wireless adb: `setprop persist.pn2.adbwifi 1` (off by default)

## Does not work yet

- **6DoF / SLAM.** Needs the tracking cameras. `qvrservice` starts but never
  opens them (`Plugin not valid`, ~15 MB resident vs stock's ~187 MB, no
  `QVRServiceCamDeviceHAL3` activity). It is an 8.1 binary trying to reach
  Android 10's camera HAL - a service-to-HAL boundary, not a missing file.
- **Passthrough imagery.** `getLockedImageInfo` and `lockImageFromBuffer` are
  libgui internals deleted in Q; they are stubbed and log a warning.
- **CVService controllers.** Starts 32-bit correctly, but crashes in
  `WriteParameter` from a wifi-state broadcast. Currently disabled.
- **Provision (setup wizard)** crashes in its language picker
  (`IndexOutOfBounds` in `Language1Adapter`) - disable it per
  [first boot](../guide/first-boot.md).
