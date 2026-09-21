# Requirements

## Hardware

- A **Pico Neo 2** (`A7B10` / `PICOA7B10`). The Eye and non-Eye SKUs both work;
  the eye-tracking stack is extra work on top.
- A USB cable and a PC that can reach the headset over adb and fastboot.
- The device must be rooted (the stock build accepts `su` via the usual paths)
  and the bootloader must be unlockable with `fastboot oem pico unlock`.

## Host tools

A Linux host. The toolchain check in `tools/setup/04_recon.sh` expects:

| tool | used for |
|---|---|
| `adb`, `fastboot` | talking to the device |
| `debugfs` | reading/writing ext4 images without mounting |
| `simg2img` | converting sparse images to raw |
| `brotli` | decompressing OTA `.br` payloads |
| `python3` | the helper scripts (`03_sdat2img.py` etc.) |
| `dtc` | decompiling device-tree blobs |
| `unzip`, `7z`, `cpio` | unpacking OTA zips and ramdisks |
| `readelf`, `strings` | inspecting ELF binaries |

## Source material

- The official **PUI 4.1.3 OTA package** for the Neo 2 (goes in `images/ota_4.1.3/`)
- A **LineageOS 17.1 `treble_arm64_avS` GSI** (phh build) - goes in `gsi/`
- The proprietary Pico stack - pulled from **your own device** or its stock
  `system.img`; it is never distributed

## Environment

Scripts honour these variables:

```bash
export PN2_ROOT=~/PN2Lineage     # workspace root
export PN2_SERIAL=XXXXXXX        # adb/fastboot serial when >1 device attached
export PICO_FW_DIR=~/Pico_Neo_Firmware   # where the OTA zips live
```
