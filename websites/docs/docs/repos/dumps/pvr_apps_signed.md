# pvr_apps_signed

[HibiscusXR/pvr/pvr_apps_signed](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/pvr/pvr_apps_signed)

## What this is

Repack stage: the PVR apps re-signed with the local platform test key from the build repo, .idsig files alongside and rebuilt oat where present.

## How to remake this dump

Sign each apk with apksigner/signapk using the platform key from the build repo (the 106-109 and 133-134 script series in tools).

## Files

45 files / 共 45 个文件 / всего файлов: 45

```
  48.9 KiB  CVService/CVService.apk
   5.8 KiB  CVService/CVService.apk.idsig
  44.5 KiB  CVService/oat/arm/CVService.odex
   1.5 MiB  CVService/oat/arm/CVService.vdex
  74.8 KiB  InitServer/InitServer.apk
  72.7 KiB  InitServer/oat/arm64/InitServer.odex
   2.7 MiB  InitServer/oat/arm64/InitServer.vdex
 643.4 KiB  PVRVerify/PVRVerify.apk
  13.8 KiB  PVRVerify/PVRVerify.apk.idsig
  76.5 KiB  PVRVerify/oat/arm/PVRVerify.odex
   2.5 MiB  PVRVerify/oat/arm/PVRVerify.vdex
  48.7 KiB  PicoSettingsProvider/PicoSettingsProvider.apk
   5.8 KiB  PicoSettingsProvider/PicoSettingsProvider.apk.idsig
  16.7 KiB  PicoSettingsProvider/oat/arm64/PicoSettingsProvider.odex
  32.8 KiB  PicoSettingsProvider/oat/arm64/PicoSettingsProvider.vdex
 374.9 KiB  PicoToSvrService/PicoToSvrService.apk
  56.7 KiB  PicoToSvrService/oat/arm64/PicoToSvrService.odex
   2.3 MiB  PicoToSvrService/oat/arm64/PicoToSvrService.vdex
  85.7 KiB  PxrNotification/PxrNotification.apk
   5.8 KiB  PxrNotification/PxrNotification.apk.idsig
  16.7 KiB  PxrNotification/oat/arm64/PxrNotification.odex
  16.3 KiB  PxrNotification/oat/arm64/PxrNotification.vdex
 664.5 KiB  ShortcutMenu/ShortcutMenu.apk
  13.8 KiB  ShortcutMenu/ShortcutMenu.apk.idsig
  76.7 KiB  ShortcutMenu/oat/arm64/ShortcutMenu.odex
   3.3 MiB  ShortcutMenu/oat/arm64/ShortcutMenu.vdex
  84.0 MiB  VRShell2/VRShell2.apk
 685.8 KiB  VRShell2/VRShell2.apk.idsig
  68.7 KiB  VRShell2/oat/arm64/VRShell2.odex
   2.7 MiB  VRShell2/oat/arm64/VRShell2.vdex
 207.5 KiB  VRUserCenter2/VRUserCenter2.apk
  80.7 KiB  VRUserCenter2/oat/arm64/VRUserCenter2.odex
   3.7 MiB  VRUserCenter2/oat/arm64/VRUserCenter2.vdex
 411.2 KiB  configserverservice/configserverservice.apk
   5.8 KiB  configserverservice/configserverservice.apk.idsig
  68.7 KiB  configserverservice/oat/arm64/configserverservice.odex
   3.0 MiB  configserverservice/oat/arm64/configserverservice.vdex
  32.7 KiB  pvr_adapter/oat/arm64/pvr_adapter.odex
 891.2 KiB  pvr_adapter/oat/arm64/pvr_adapter.vdex
 589.3 KiB  pvr_adapter/pvr_adapter.apk
  13.8 KiB  pvr_adapter/pvr_adapter.apk.idsig
  20.7 KiB  pvrdisplay/oat/arm64/pvrdisplay.odex
 191.2 KiB  pvrdisplay/oat/arm64/pvrdisplay.vdex
 903.7 KiB  pvrdisplay/pvrdisplay.apk
  13.8 KiB  pvrdisplay/pvrdisplay.apk.idsig
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
