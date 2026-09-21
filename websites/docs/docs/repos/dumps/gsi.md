# gsi

[HibiscusXR/system/gsi](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/gsi)

## What this is

The base system image for the port: the phh LineageOS 17.1 (Android 10) GSI for treble_arm64_avS (arm64, A-only), plus gsi_raw.img (the simg2img'd raw ext4 copy) and boot_adb.img (a boot image with adb enabled).

## How to remake this dump

Download the phh LineageOS 17.1 `treble_arm64_avS` .xz build, `unxz` it, and run `simg2img` on it to get gsi_raw.img. boot_adb.img is the stock boot image repacked with adb/debuggable patches.

## Files

4 files / 共 4 个文件 / всего файлов: 4

```
  15.0 MiB  boot_adb.img
   1.9 GiB  gsi_raw.img
   1.8 GiB  lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img
 567.5 MiB  lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img.xz
```

## License

These files come from a third-party project and remain under that project's own license - see the source for terms. The AGPL v3 in this repository applies only to this README and our own original files.
