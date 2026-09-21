# pvr_apps_dexed

[HibiscusXR/pvr/pvr_apps_dexed](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/pvr/pvr_apps_dexed)

## What this is

First stage of the repack pipeline: the pvr_apps apks deodexed, dex code recovered from the odex/vdex so the apps can be modified and rebuilt.

## How to remake this dump

Run the unquicken/deodex pass over pvr_apps (same tooling as tools/132_deodex_oem.sh).

## Files

24 files / 共 24 个文件 / всего файлов: 24

```
 417.0 KiB  CVService/CVService.apk
   5.8 KiB  CVService/CVService.apk.idsig
   1.1 MiB  InitServer/InitServer.apk
  17.8 KiB  InitServer/InitServer.apk.idsig
   1.6 MiB  PVRVerify/PVRVerify.apk
  21.8 KiB  PVRVerify/PVRVerify.apk.idsig
  60.8 KiB  PicoSettingsProvider/PicoSettingsProvider.apk
   5.8 KiB  PicoSettingsProvider/PicoSettingsProvider.apk.idsig
   1.2 MiB  PicoToSvrService/PicoToSvrService.apk
  17.8 KiB  PicoToSvrService/PicoToSvrService.apk.idsig
  93.7 KiB  PxrNotification/PxrNotification.apk
   5.8 KiB  PxrNotification/PxrNotification.apk.idsig
   1.9 MiB  ShortcutMenu/ShortcutMenu.apk
  21.8 KiB  ShortcutMenu/ShortcutMenu.apk.idsig
  84.9 MiB  VRShell2/VRShell2.apk
 693.8 KiB  VRShell2/VRShell2.apk.idsig
   1.6 MiB  VRUserCenter2/VRUserCenter2.apk
  21.8 KiB  VRUserCenter2/VRUserCenter2.apk.idsig
   1.5 MiB  configserverservice/configserverservice.apk
  21.8 KiB  configserverservice/configserverservice.apk.idsig
 921.3 KiB  pvr_adapter/pvr_adapter.apk
  13.8 KiB  pvr_adapter/pvr_adapter.apk.idsig
 971.8 KiB  pvrdisplay/pvrdisplay.apk
  13.8 KiB  pvrdisplay/pvrdisplay.apk.idsig
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
