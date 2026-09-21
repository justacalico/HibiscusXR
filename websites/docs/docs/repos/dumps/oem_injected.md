# oem_injected

[HibiscusXR/oem/oem_injected](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/oem/oem_injected)

## What this is

The /oem apps with extra native libraries injected into the apk (for example ToBToolService carrying lib/arm64/libPvr_UnitySDK.so) before re-signing.

## How to remake this dump

Inject the required .so files into the apk zip, then rebuild and sign via tools/133_oem_build.py.

## Files

6 files / 共 6 个文件 / всего файлов: 6

```
   3.8 MiB  PVRHome/PVRHome.apk
 991.1 KiB  PVRLauncher/PVRLauncher.apk
 700.9 KiB  ToBToolService/ToBToolService.apk
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
   2.9 MiB  provision2d/provision2d.apk
   3.2 MiB  store2d/store2d.apk
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
