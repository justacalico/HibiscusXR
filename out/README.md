# Pico Neo 2 (A7B10) — LineageOS 17.1 / Android 10

LineageOS 17.1 GSI (`treble_arm64_avS`) plus the fixes needed to run it on Pico
Neo 2 hardware.

`system-pn2-full.img` — 3,565,158,400 bytes, ext4, fsck-clean.

Full-proprietary build: the GSI plus Pico's complete stack, extracted from a stock
Pico Neo 2 (PUI 4.1.3). Every proprietary file is mapped in
`overlay/PROPRIETARY-PVR.md` with path, size and purpose, so the OS can be rebuilt
without any of them and an end user restores them from firmware they own.

The clean/no-proprietary image is no longer maintained.

`/vendor` is untouched. Verified byte-identical to stock.

## Flashing

```
fastboot oem pico unlock          # required once per fastboot session
fastboot -S 128M flash system system-pn2-full.img
```

`-S 128M` is not optional. The bootloader advertises a 512MB max download size,
but sending chunks that large kills the USB link partway through
("Write to device failed (no link)") and leaves system half-written.

## The bugs this fixes

1. **vold deadlock.** Stock `/vendor/manifest.xml` declares
   `android.hardware.boot@1.0::IBootControl` but ships no implementation, on a
   device with no A/B partitions. 8.1 never called it; Android 10 vold does, every
   boot, and libhidl blocks forever on a VINTF-declared but unregistered
   interface. system_server was watchdog-killed every ~80s, permanently.

2. **No sound card.** The sdm845 audio drivers are kernel modules and stock loads
   them at ~2.7s via `/vendor/bin/modprobe`, which under Android 10 fails with
   `cannot execve` — it is `toybox_vendor`, linked against bionic in the runtime
   APEX, which apexd has not mounted that early.

3. **Sensor asserts (two).** Pico's HAL emits event types 57, 58, 126 and 127;
   Android 10 fatally `CHECK`s on anything between 36 and 65535. Its
   `DYNAMIC_SENSOR_META` path also aborts on a connect for an unregistered handle.

4. **ABI breaks in `libpvrmodule_platform.so`.** `getBuiltInDisplay(int)` removed,
   `DisplayEventReceiver::Event` grew 24 → 32 bytes, `DisplayInfo` grew 48 → 56.
   The last one overwrote the display token stored right after it — the crash
   address was literally the screen resolution: `0x87000000f00` = 2160<<32 | 3840.

5. **Suspend.** The device hangs entering kernel suspend and the watchdog reboots
   it. Stock never hits this: its `pvrservice` holds a kernel wakelock permanently,
   so PUI never suspends at all. We hold an equivalent one. This **matches stock
   behaviour rather than repairing suspend**; idle drain is higher, as on stock.

6. **The linker whitelist.** `/system/etc/public.libraries.txt` came from the GSI,
   so it carried only the AOSP list and dropped stock's 22 Pico entries. On
   Android 8+ that file is what lets an app's linker namespace `dlopen` a
   non-public `/system` library — without it **every** Pico library load from an
   app returns null, and Pico's code calls the result unchecked. This was the
   single biggest blocker; fixing it took VRShell from dying at startup to
   creating 16 compositor layers.

7. **Six missing libraries and two missing daemons.** `libvirtualinputclient.so`
   (which defines `pvrVirtualInputCreate`), `libairclient.so`, `libSafetyArea.so`,
   `libImageGrid.so`, `libdatabuffer.so`, `libvirtualinput.so`, plus
   `/system/bin/airservice` and `/system/bin/virtual_input` with their init entries.

8. **`qvrd` never started.** Qualcomm's VR service is defined in vendor init
   without a `seclabel`, and init refuses a service with no SELinux domain even
   when permissive. A corrected block under the same name is discarded as a
   duplicate, so it runs as **`pn2_qvrd`** instead; socket names are unchanged,
   which is what clients actually use.

9. **Two x28 binary patches.** Android 10's `art_quick_generic_jni_trampoline`
   parks the caller's stack pointer in x28 across a JNI call; x28 comes back zero
   and `sp` becomes null. Patched to use x29, which holds the same frame base and
   survives. The same register loss inside `pvr_EnterVrMode` is worked around by
   recomputing the struct base from x27.

## What works

- Boots, audio, landscape 3840x2160, no sensor aborts, suspend as stock
- `pvrservice` runs and publishes **live head rotation** (valid unit quaternions)
- `airservice`, `virtual_input` and `pn2_qvrd` all start
- **VRShell (the VR home shell) launches reliably** and drives the real Pico
  compositor: async TimeWarp, single-buffered direct present, EGL high priority,
  16 layers, warp thread pinned to core 6, correct `hmdInfo` (3840x2160,
  lensSeparation 0.062, eyeTextureFov 80)
- See-through calibration app installed, platform-signed and launching
- 2D Pico apps render (VRUserCenter, Pico Store)
- Optional wireless adb: `setprop persist.pn2.adbwifi 1` (off by default)

## What does not work yet

- **The display stays black in VR.** `pvrservice` publishes good rotation, but the
  SDK *inside the app* reports `trackingstate = 0x0,0x0`, so the pose it submits
  fails the compositor's unit-quaternion check and every frame is dropped
  (`SelectRT Bad Pose.Orientation` → `Nothing to draw, draw black!`). This is the
  single blocker for seeing anything, and it is a client-side problem: the data
  exists on the service side and arrives as "no tracking" on the client side.
- **6DoF / SLAM.** Needs the tracking cameras. `qvrservice` starts but never opens
  them (`Plugin not valid`, ~15 MB resident vs stock's ~187 MB, no
  `QVRServiceCamDeviceHAL3` activity). It is an 8.1 binary trying to reach
  Android 10's camera HAL — a service-to-HAL boundary, not a missing file.
- **Passthrough imagery.** `getLockedImageInfo` and `lockImageFromBuffer` are
  libgui internals deleted in Q; they are stubbed and log a warning.
- **CVService controllers.** Starts 32-bit correctly now, but crashes in
  `WriteParameter` from a wifi-state broadcast. Currently disabled.
- **Provision (setup wizard)** crashes in its language picker
  (`IndexOutOfBounds` in `Language1Adapter`).

## First boot

The Pico launcher loops trying to start Provision, which crashes. Disable it and
the launcher skips setup and hands off to VRShell:

```
adb shell su -c 'pm disable com.picovr.provision'
adb shell su -c 'settings put global hide_error_dialogs 1'
```

## Recovery

Every patched proprietary file has a `.orig` beside it on the device
(`libart.so.orig`, `libPvr_UnitySDK.so.orig`, …), so a bad patch can be reverted
in place without reflashing.
