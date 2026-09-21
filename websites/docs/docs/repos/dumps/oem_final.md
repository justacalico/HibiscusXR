# oem_final

[HibiscusXR/oem/oem_final](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/oem/oem_final)

## What this is

Final repacked /oem apps: each apk rebuilt - unquickened dex, re-signed, .idsig alongside - ready to push back to a device.

## How to remake this dump

tools/133_oem_build.py repacks and signs; tools/134_install_oem.ps1 pushes them.

## Files

11 files / 共 11 个文件 / всего файлов: 11

```
   3.8 MiB  PVRHome/PVRHome.apk
  37.8 KiB  PVRHome/PVRHome.apk.idsig
 995.1 KiB  PVRLauncher/PVRLauncher.apk
  13.8 KiB  PVRLauncher/PVRLauncher.apk.idsig
 703.6 KiB  ToBToolService/ToBToolService.apk
  13.8 KiB  ToBToolService/ToBToolService.apk.idsig
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
   2.9 MiB  provision2d/provision2d.apk
  29.8 KiB  provision2d/provision2d.apk.idsig
   3.2 MiB  store2d/store2d.apk
  33.8 KiB  store2d/store2d.apk.idsig
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
