# seethrough

[HibiscusXR/applications/seethrough](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/applications/seethrough)

## What this is

The seethroughsetting app pulled from stock: seethroughsetting.apk plus its signed variant and the app-private lib/arm64 native libraries (libil2cpp, libmain, libunity, libnative(-lib), libPvr_UnitySDK, libtracking_module).

## How to remake this dump

adb pull /system/priv-app/seethroughsetting (or rdump it from system.img), then sign with the platform key.

## Files

10 files / 共 10 个文件 / всего файлов: 10

```
   2.0 MiB  lib/arm64/libPvr_UnitySDK.so
  17.3 MiB  lib/arm64/libil2cpp.so
   6.0 KiB  lib/arm64/libmain.so
 182.1 KiB  lib/arm64/libnative-lib.so
 286.3 KiB  lib/arm64/libnative.so
 269.9 KiB  lib/arm64/libtracking_module.so
  13.0 MiB  lib/arm64/libunity.so
 193.8 MiB  seethroughsetting-signed.apk
   1.5 MiB  seethroughsetting-signed.apk.idsig
 193.8 MiB  seethroughsetting.apk
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
