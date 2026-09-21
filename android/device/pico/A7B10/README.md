# device/pico/A7B10 — Pico Neo 2

LineageOS device tree for the Pico Neo 2 (`A7B10` / `PICOA7B10`).

Every value in this tree was read out of the stock firmware, not copied from a
similar sdm845 device. Where something was derived rather than read directly,
the source is noted inline in the makefile.

## Hardware

| | |
|---|---|
| SoC | Qualcomm SDM845, Adreno 630 |
| Stock OS | Android 8.1.0, SDK 27, VNDK 27 |
| Stock build | PUI 4.1.3 b346 (2021-04-09), security patch 2019-01-05 |
| Kernel | 4.9.65-perf+ (sdm845 launch BSP, never rebased) |
| Partitioning | Non-A/B, UFS 6 LUNs, discrete `recovery`, no `super` |
| Panel | JDI 4K, dual DSI 1080×3840 per link → 3840×2160 @ 72 Hz, DSC 540×8 |
| Tracking cameras | OmniVision ov9282 stereo, 2560×800 |
| Eye cameras | OmniVision ov6211 stereo, 800×400 (Eye SKU only, Tobii) |
| IMU | InvenSense ICM-206xx |
| Controllers | Electromagnetic — Lattice iCE40LP1K FPGA + SLPI DSP, Nordic BLE |

## Two facts that shape this port

**SELinux is permissive from the factory.** `androidboot.selinux=permissive` is
baked into the stock `boot.img` cmdline on retail *user* builds. This tree does
not add it — it is reproducing what shipped.

**The board is a Qualcomm reference design.** The bootloader reports
`QC_REFERENCE_PHONE`, and all 40 DTBO entries carry stock Qualcomm model strings
(MTP / CDP / QRD / HDK / SVR / QVR). Pico never renamed them. Verified on device,
the board actually selects **`dtbo_05` — "sda845 v2.1 MTP"** (not the QVR or SVR
entry). The device tree base is therefore Qualcomm's public CAF sdm845 reference,
with Pico hardware added as overlay fragments:

| Node | Purpose |
|---|---|
| `picovr,spi-w25q` | FPGA config flash — `fpga_rset` GPIO 84, `power-en` GPIO 81, `switch-gpio` 122 |
| `picovr,nordic` | Nordic BLE radio, controller link |
| `icm@68` | `imu,icm206xx` |
| `eepromi2c@57` | Calibration EEPROM + `tof-en` |
| `nq@28` | NXP NFC |
| `gpio_fan` | Active cooling, PWM tach IRQ |
| `hw_version` | 3-bit board revision straps, GPIO 105/106/107 |
| `gpio_keys` | `app_key`, `confirm_key` |

## The VNDK 27 ceiling

The stock vendor partition is VNDK 27. Google removed the VNDK snapshot
mechanism in Android 15, so any build that reuses the stock vendor partition
realistically tops out around Android 11, and 9/10 will be far less painful.
Reaching a genuinely modern Android requires replacing the vendor side
(mainline kernel + Mesa), which is a much larger project.

## Blobs

No proprietary code is committed here. `proprietary-files.txt` lists 469 entries
generated directly from the real vendor partition. `extract-files.sh` pulls them
from either:

1. the user's own device over adb (needs root), or
2. an official Pico OTA zip the user downloads themselves:
   `./extract-files.sh /path/to/update_PicoNeo2_pui4.1.3_*.zip`

Because the device is non-A/B with a real separate `vendor` partition, a release
zip can also simply **not touch** `/vendor` or `/firmware` and let the device
supply its own blobs from what is already installed. That yields a prebuilt,
one-click zip with no proprietary redistribution and no build step for the user.

## Kernel

Pico published no GPL kernel source. Until a matching CAF tree is reconstructed
(`msm-4.9`, tag family `LA.UM.7.1.r1-*-sdm845.0`), this tree ships the stock
prebuilt kernel from `prebuilt/`. Because the base tree is Qualcomm's reference,
reconstruction should be a matter of applying the enumerated Pico overlay
fragments to the CAF baseline rather than reverse-engineering an unknown tree.

## Backups — read this before flashing

`/persist` holds factory per-unit camera/IMU calibration
(`/persist/pvr/camera/device_calibration.xml`) and `/persist/ndi/` holds the EM
coil and controller calibration (`tx1.eep`, `tx2.eep`, `rxcache.eep`). **None of
it is recoverable if lost.** Back up every partition before flashing anything.

Also: the flash counter on `xbl`/`abl` is protected by anti-rollback fuses.
Writing an older bootloader than the blown fuse value is the one genuine
hard-brick vector that survives EDL. Treat the bootloader partitions as
read-only unless there is a specific reason not to.

## Flashing

The bootloader ships **already unlocked** on this unit and verity is off:

```
ro.boot.flash.locked         0
ro.boot.vbmeta.device_state  unlocked
ro.boot.verifiedbootstate    orange
androidboot.veritymode       disabled
```

**But `fastboot flash` will still be refused until you unlock the session.**
Pico gate writes behind a vendor OEM command that must be issued **once per
fastboot boot** — it does not persist across reboots:

```bash
adb reboot bootloader
fastboot oem pico unlock        # REQUIRED every time you enter fastboot
fastboot flash boot boot.img
```

Forgetting this produces a permission/`FAILED` response that looks like a locked
bootloader even though the device reports unlocked from Android.

## Status

Pre-boot. Nothing in this tree has been built or flashed yet.

Done:
- Full partition backup of the target unit, including `persist`
- Device tree scaffolding written from real extracted values
- `proprietary-files.txt` generated from the actual vendor partition (469 entries)
- Bootloader confirmed unlocked, verity confirmed disabled
- Booted DTBO and display node identified on device

Next gate: get a Treble arm64 A-only GSI to boot against the stock vendor
partition, flashing `system` only and leaving `vendor` and `firmware` untouched.
