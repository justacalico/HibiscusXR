# tools

[HibiscusXR/system/tools](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/tools)

Every script used in the port, sorted by job. The numbered scripts are roughly
chronological - they are research notes in executable form, so read the
comments before running anything.

## Layout

| folder | contents |
|---|---|
| `setup/` | host toolchain setup and first-pass recon |
| `backup/` | partition backup/restore, w25q SPI and eye-tracker firmware dumps |
| `extract/` | pulling files out of OTA packages, partition images, live devices |
| `build/` | assembling, signing and flashing system images |
| `patch/` | binary patches: libsensorservice, libart x28, ABI shims, libinput |
| `sensors/` | sensor/calibration/lens work |
| `audio/` | sound card bring-up |
| `power/` | suspend, thermal and fan |
| `input/` | keylayout, keycode and input patches |
| `dsp/` | Hexagon/CDSP/rfsa/FastRPC pieces of the tracking pipeline |
| `vr/` | VRShell, pvrservice, qvr, 6DoF, launcher, provisioning |
| `boot/` | vold/fstab/boothal boot-time fixes |
| `diag/` | scans, probes, diffs, disassembly, logging helpers |
| `scratch/` | `_*.sh` one-off device snippets kept for reference |

## Conventions

- Host scripts are `bash`; scripts that run **on the headset** start with
  `#!/system/bin/sh`.
- `PN2_ROOT` (default `~/PN2Lineage`) is the workspace root;
  `PN2_SERIAL` picks the adb/fastboot device.
- `PICO_FW_DIR`, `NDK_BIN`, `GSI_MNT`, `NDI_OUT`, `OBJDUMP` override the
  remaining optional paths.

## Entry points

- `backup/backup_partitions.sh` - dd every partition off the device
- `backup/restore_stock.sh` - flash it back (`--all`, `--dry-run`)
- `extract/pull_oem_apps.sh` - pull the /oem apps
- `build/flash_full.sh` - flash the built image correctly
- `build/install_oem_apps.sh`, `build/install_applibs.sh` - install repacked
  apps + private libs into /system
