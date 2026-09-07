# Building the image

The build takes a LineageOS GSI and writes our overlay into it with `debugfs`
- no loop mounts, no root needed on the host. All scripts live in the
[tools](../repos/tools.md) repo.

## 1. Rebuild the stock images

```
extract/03_sdat2img.py   # OTA payload -> raw images
extract/04...            # see tools/setup/04_recon.sh:
brotli -d vendor.new.dat.br
python3 tools/extract/03_sdat2img.py vendor.transfer.list vendor.new.dat vendor.img
```

`tools/setup/04_recon.sh` automates this: decompress the OTA payload, rebuild
`vendor.img`, and pull the identity files out of system/vendor/oem.

## 2. Prepare the GSI

```
unxz lineage-17.1-*-treble_arm64_avS.img.xz
simg2img lineage-17.1-*.img gsi_raw.img
```

`tools/build/22_inspect_gsi.sh` and `23_gsi_raw.sh` sanity-check the result:
A-only layout, VNDK 27 snapshot present (required - the vendor is VNDK 27),
fits in the 3,943,694,336-byte system partition.

## 3. Write the overlay in

`tools/build/142_build_image.sh` copies `gsi/gsi_raw.img` to
`out/system-pn2.img` and `debugfs -w`-writes every file from `overlay/` into it
with correct mode/owner:

- `etc/init/pn2-*.rc` service definitions (sound, qvrd, airservice, fan,
  power, settings, vintf)
- `lib64/libsensorservice.so` (patched) and `lib64/libshim_pvr.so`
- `etc/pn2/vendor_manifest.xml`, `props.append`

## 4. Add the proprietary stack

The clean image ships **no** Pico files. `overlay/PROPRIETARY-PVR.md` maps
every blob - extract them from your own `system.img` or device, then
`tools/build/144_stage_full.sh` + `145_build_full.sh` produce
`out/system-pn2-full.img`.

## 5. Verify

`tools/build/268_verify_img.sh` runs `e2fsck` and confirms the tree. The known
good full image is 3,565,158,400 bytes.
