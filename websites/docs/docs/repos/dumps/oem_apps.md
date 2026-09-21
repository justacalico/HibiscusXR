# oem_apps

[gitlab.com/neosalsa/oem_apps](https://gitlab.com/neosalsa/oem_apps)

## What this is

Raw pulls of the Pico apps living on the /oem partition: PVRLauncher, PVRHome, store2d, provision2d and ToBToolService, each with its apk plus oat/arm64 odex and vdex. /oem is not in the OTA package, so a stock device is the only source.

## How to remake this dump

`adb pull /oem/priv-app/<name>` on a stock device - see tools/131_pull_oem.ps1.

## Files

16 files / 共 16 个文件 / всего файлов: 16

```
   1.8 MiB  PVRHome/PVRHome.apk
 124.7 KiB  PVRHome/oat/arm64/PVRHome.odex
   5.7 MiB  PVRHome/oat/arm64/PVRHome.vdex
 610.1 KiB  PVRLauncher/PVRLauncher.apk
  32.7 KiB  PVRLauncher/oat/arm64/PVRLauncher.odex
1011.5 KiB  PVRLauncher/oat/arm64/PVRLauncher.vdex
 394.8 KiB  ToBToolService/ToBToolService.apk
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
  36.7 KiB  ToBToolService/oat/arm64/ToBToolService.odex
 801.3 KiB  ToBToolService/oat/arm64/ToBToolService.vdex
  56.7 KiB  provision2d/oat/arm64/provision2d.odex
   2.7 MiB  provision2d/oat/arm64/provision2d.vdex
   1.9 MiB  provision2d/provision2d.apk
 124.7 KiB  store2d/oat/arm64/store2d.odex
   6.2 MiB  store2d/oat/arm64/store2d.vdex
 966.7 KiB  store2d/store2d.apk
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
