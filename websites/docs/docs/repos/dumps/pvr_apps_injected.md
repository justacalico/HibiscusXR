# pvr_apps_injected

[gitlab.com/neosalsa/pvr_apps_injected](https://gitlab.com/neosalsa/pvr_apps_injected)

## What this is

Repack stage: apks with the needed native libraries injected into the package (for example VRShell2 carrying its own libPvr_UnitySDK) before final signing.

## How to remake this dump

Inject the .so files into the apk zip, then rebuild and sign with the same pipeline.

## Files

12 files / 共 12 个文件 / всего файлов: 12

```
 414.2 KiB  CVService/CVService.apk
   1.1 MiB  InitServer/InitServer.apk
   1.6 MiB  PVRVerify/PVRVerify.apk
  58.2 KiB  PicoSettingsProvider/PicoSettingsProvider.apk
   1.2 MiB  PicoToSvrService/PicoToSvrService.apk
  87.6 KiB  PxrNotification/PxrNotification.apk
   1.8 MiB  ShortcutMenu/ShortcutMenu.apk
  84.9 MiB  VRShell2/VRShell2.apk
   1.6 MiB  VRUserCenter2/VRUserCenter2.apk
   1.5 MiB  configserverservice/configserverservice.apk
 915.0 KiB  pvr_adapter/pvr_adapter.apk
 966.5 KiB  pvrdisplay/pvrdisplay.apk
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
