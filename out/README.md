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

---

# out

English | [中文](#中文) | [Русский](#русский)

## What this is

Build output. system-pn2.img / system-pn2-full.img are the flashable LineageOS 17.1 system images for Pico Neo 2 (the GSI plus our overlay written in with debugfs), and dev_sensor.so / dev_shim.so are our own compiled shim libraries.

## How to remake this dump

tools/142-147: copy gsi/gsi_raw.img, `debugfs -w` the overlay files into it, then tools/147_flash_full.ps1 flashes the result. The full details are in the README below.

## License

The built images combine the third-party LineageOS GSI (under its own license) with our own original work. The AGPL v3 in this repository applies to this README and our own files only (shims, overlay files, scripts). LineageOS content stays under its own license, and no Pico proprietary content is shipped here.

## 中文

### 这是什么

构建产物。system-pn2.img / system-pn2-full.img 是 Pico Neo 2 可刷入的 LineageOS 17.1 系统镜像（GSI 加上用 debugfs 写入的我们的 overlay），dev_sensor.so / dev_shim.so 是我们自己编译的 shim 库。

### 如何重新制作这些转储

重新制作：tools/142-147——复制 gsi/gsi_raw.img，用 `debugfs -w` 写入 overlay 文件，再用 tools/147_flash_full.ps1 刷入。详细说明见下方 README。

### 许可证说明

构建出的镜像由第三方 LineageOS GSI（受其自身许可证约束）与我们自己的原创工作组合而成。本仓库的 AGPL v3 仅适用于本 README 以及我们自己的文件（shim、overlay 文件、脚本）。LineageOS 的内容仍受其自身许可证约束，本仓库不包含任何 Pico 专有内容。

## Русский

### Что это

Результаты сборки. system-pn2.img / system-pn2-full.img — прошиваемые образы LineageOS 17.1 для Pico Neo 2 (GSI плюс наш overlay, записанный через debugfs), а dev_sensor.so / dev_shim.so — наши собственные собранные шим-библиотеки.

### Как воспроизвести дамп

Воспроизведение: tools/142-147 — копия gsi/gsi_raw.img, запись overlay-файлов через `debugfs -w`, затем прошивка через tools/147_flash_full.ps1. Подробности — в README ниже.

### Лицензия

Собранные образы объединяют сторонний LineageOS GSI (под его собственной лицензией) с нашей собственной работой. AGPL v3 в этом репозитории распространяется только на этот README и наши собственные файлы (шимы, overlay-файлы, скрипты). Контент LineageOS остаётся под своей лицензией, проприетарные файлы Pico здесь не распространяются.

## Files in this folder / 本目录文件 / Файлы в этой папке

4 files / 共 4 个文件 / всего файлов: 4

```
   0.2 KiB  dev_sensor.so
   0.0 KiB  dev_shim.so
   0.0 GiB  system-pn2-full.img
   0.0 GiB  system-pn2.img
```

---

# out

English | [中文](#中文) | [Русский](#русский)

## What this is

Build output. system-pn2.img / system-pn2-full.img are the flashable LineageOS 17.1 system images for Pico Neo 2 (the GSI plus our overlay written in with debugfs), and dev_sensor.so / dev_shim.so are our own compiled shim libraries.

## How to remake this dump

tools/142-147: copy gsi/gsi_raw.img, `debugfs -w` the overlay files into it, then tools/147_flash_full.ps1 flashes the result. The full details are in the README below.

## License

The built images combine the third-party LineageOS GSI (under its own license) with our own original work. The AGPL v3 in this repository applies to this README and our own files only (shims, overlay files, scripts). LineageOS content stays under its own license, and no Pico proprietary content is shipped here.

## 中文

### 这是什么

构建产物。system-pn2.img / system-pn2-full.img 是 Pico Neo 2 可刷入的 LineageOS 17.1 系统镜像（GSI 加上用 debugfs 写入的我们的 overlay），dev_sensor.so / dev_shim.so 是我们自己编译的 shim 库。

### 如何重新制作这些转储

重新制作：tools/142-147——复制 gsi/gsi_raw.img，用 `debugfs -w` 写入 overlay 文件，再用 tools/147_flash_full.ps1 刷入。详细说明见下方 README。

### 许可证说明

构建出的镜像由第三方 LineageOS GSI（受其自身许可证约束）与我们自己的原创工作组合而成。本仓库的 AGPL v3 仅适用于本 README 以及我们自己的文件（shim、overlay 文件、脚本）。LineageOS 的内容仍受其自身许可证约束，本仓库不包含任何 Pico 专有内容。

## Русский

### Что это

Результаты сборки. system-pn2.img / system-pn2-full.img — прошиваемые образы LineageOS 17.1 для Pico Neo 2 (GSI плюс наш overlay, записанный через debugfs), а dev_sensor.so / dev_shim.so — наши собственные собранные шим-библиотеки.

### Как воспроизвести дамп

Воспроизведение: tools/142-147 — копия gsi/gsi_raw.img, запись overlay-файлов через `debugfs -w`, затем прошивка через tools/147_flash_full.ps1. Подробности — в README ниже.

### Лицензия

Собранные образы объединяют сторонний LineageOS GSI (под его собственной лицензией) с нашей собственной работой. AGPL v3 в этом репозитории распространяется только на этот README и наши собственные файлы (шимы, overlay-файлы, скрипты). Контент LineageOS остаётся под своей лицензией, проприетарные файлы Pico здесь не распространяются.

## Files in this folder / 本目录文件 / Файлы в этой папке

4 files / 共 4 个文件 / всего файлов: 4

```
 221.9 KiB  dev_sensor.so
   7.8 KiB  dev_shim.so
   3.3 GiB  system-pn2-full.img
   1.9 GiB  system-pn2.img
```
