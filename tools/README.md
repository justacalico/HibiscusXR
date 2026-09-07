# tools

Every script used in the PN2Lineage port, sorted by job. Numbered scripts are
roughly chronological - they are research notes in executable form, so read the
comments before running anything.

## Layout

| folder | what lives there |
|---|---|
| `setup/` | host toolchain setup and first-pass recon |
| `backup/` | partition backup/restore, SPI flash (w25q) and eye-tracker firmware dumping |
| `extract/` | pulling files out of OTA packages, partition images and a live device |
| `build/` | assembling, signing and flashing system images |
| `patch/` | binary patches: libsensorservice, libart x28, ABI shims, libinput |
| `sensors/` | sensor/calibration/lens work |
| `audio/` | sound card bring-up |
| `power/` | suspend, thermal and fan |
| `input/` | keylayout, keycode and input patches |
| `dsp/` | Hexagon/CDSP/rfsa/FastRPC pieces of the tracking pipeline |
| `vr/` | VRShell, pvrservice, qvr, 6DoF, launcher, provisioning |
| `boot/` | vold/fstab/boothal boot-time fixes |
| `diag/` | scans, probes, diffs, disassembly and logging helpers |
| `scratch/` | `_*.sh` one-off device snippets kept for reference |

## Conventions

- Host scripts are `bash`; scripts meant to run on the headset start with
  `#!/system/bin/sh`.
- `PN2_ROOT` points at the workspace root (default `~/PN2Lineage`).
- `PN2_SERIAL` selects the adb/fastboot device when more than one is attached.
- `PICO_FW_DIR`, `NDK_BIN`, `GSI_MNT`, `NDI_OUT`, `OBJDUMP` override the
  remaining optional paths.

## The useful entry points

- `backup/backup_partitions.sh` - dd every partition off the device, most
  irreplaceable first
- `backup/restore_stock.sh` - flash a backup back (`--all`, `--dry-run`);
  never touches the bootloader chain
- `extract/pull_oem_apps.sh` - pull the /oem apps (not in the OTA)
- `build/flash_full.sh` - flash `out/system-pn2-full.img` with the required
  `oem pico unlock` + `-S 128M` handling
- `build/install_oem_apps.sh`, `build/install_applibs.sh` - install repacked
  apps and their private libs into /system
