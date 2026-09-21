# pvr_apps

[gitlab.com/neosalsa/pvr_apps](https://gitlab.com/neosalsa/pvr_apps)

## What this is

Every PVR system app pulled from the stock PUI 4.1.3 system.img: the /system/app and /system/priv-app apks with their oat/ odex and vdex - VRShell2, CVService, InitServer, PicoSettingsProvider, configserverservice, pvrdisplay, pvr_adapter, PVRVerify, ShortcutMenu, VRUserCenter2, PicoToSvrService, PxrNotification, WebVR and others - plus the etc/permissions xml files.

## How to remake this dump

Extracted with `debugfs -R rdump` of /system/app and /system/priv-app out of images/ota_4.1.3/system.img (the extraction scripts in the tools repo).

## Files

304 files / 共 304 个文件 / всего файлов: 304

```
   1.3 MiB  _certs/gsi_SettingsProvider.apk
 374.9 KiB  app/PicoToSvrService/PicoToSvrService.apk
   1.4 MiB  app/PicoToSvrService/lib/arm64/libPvr_NativeSDK.so
   1.0 MiB  app/PicoToSvrService/lib/arm64/libgnustl_shared.so
   9.6 KiB  app/PicoToSvrService/lib/arm64/libsharedmem.so
  56.7 KiB  app/PicoToSvrService/oat/arm64/PicoToSvrService.odex
   2.3 MiB  app/PicoToSvrService/oat/arm64/PicoToSvrService.vdex
  80.7 KiB  app/PxrNotification/PxrNotification.apk
  16.7 KiB  app/PxrNotification/oat/arm64/PxrNotification.odex
  16.3 KiB  app/PxrNotification/oat/arm64/PxrNotification.vdex
  89.1 MiB  app/WebVR/WebVR.apk
 153.7 KiB  app/WebVR/lib/arm/libaccessibility.cr.so
  42.5 KiB  app/WebVR/lib/arm/libanimation.cr.so
  22.0 KiB  app/WebVR/lib/arm/libapdu.cr.so
   1.3 MiB  app/WebVR/lib/arm/libbase.cr.so
 282.4 KiB  app/WebVR/lib/arm/libbase_i18n.cr.so
 139.8 KiB  app/WebVR/lib/arm/libbindings.cr.so
  62.6 KiB  app/WebVR/lib/arm/libbindings_base.cr.so
  69.9 KiB  app/WebVR/lib/arm/libblink_android_mojo_bindings_shared.cr.so
 662.3 KiB  app/WebVR/lib/arm/libblink_common.cr.so
  75.6 KiB  app/WebVR/lib/arm/libblink_controller.cr.so
  17.4 MiB  app/WebVR/lib/arm/libblink_core.cr.so
  17.7 KiB  app/WebVR/lib/arm/libblink_core_mojo_bindings_shared.cr.so
   7.1 MiB  app/WebVR/lib/arm/libblink_modules.cr.so
 177.9 KiB  app/WebVR/lib/arm/libblink_mojo_bindings_shared.cr.so
  49.8 KiB  app/WebVR/lib/arm/libblink_offscreen_canvas_mojo_bindings_shared.cr.so
   6.2 MiB  app/WebVR/lib/arm/libblink_platform.cr.so
 327.1 KiB  app/WebVR/lib/arm/libbluetooth.cr.so
 896.2 KiB  app/WebVR/lib/arm/libboringssl.cr.so
 621.6 KiB  app/WebVR/lib/arm/libc++_shared.so
  26.3 KiB  app/WebVR/lib/arm/libcaptive_portal.cr.so
  26.0 KiB  app/WebVR/lib/arm/libcapture_base.cr.so
 846.9 KiB  app/WebVR/lib/arm/libcapture_lib.cr.so
  42.2 KiB  app/WebVR/lib/arm/libcbor.cr.so
   2.1 MiB  app/WebVR/lib/arm/libcc.cr.so
 165.1 KiB  app/WebVR/lib/arm/libcc_animation.cr.so
  90.7 KiB  app/WebVR/lib/arm/libcc_base.cr.so
 500.9 KiB  app/WebVR/lib/arm/libcc_blink.cr.so
  29.8 KiB  app/WebVR/lib/arm/libcc_debug.cr.so
 115.6 KiB  app/WebVR/lib/arm/libcc_ipc.cr.so
 303.9 KiB  app/WebVR/lib/arm/libcc_paint.cr.so
  17.8 KiB  app/WebVR/lib/arm/libcdm_manager.cr.so
  19.4 MiB  app/WebVR/lib/arm/libchrome.cr.so
 426.1 KiB  app/WebVR/lib/arm/libchromium_sqlite3.cr.so
 297.3 KiB  app/WebVR/lib/arm/libclient.cr.so
  86.6 KiB  app/WebVR/lib/arm/libcloud_policy_proto_generated_compile.cr.so
 295.5 KiB  app/WebVR/lib/arm/libcodec.cr.so
  50.7 KiB  app/WebVR/lib/arm/libcolor_space.cr.so
  25.8 KiB  app/WebVR/lib/arm/libcommon.cr.so
 190.4 KiB  app/WebVR/lib/arm/libcompositor.cr.so
  20.0 MiB  app/WebVR/lib/arm/libcontent.cr.so
 318.1 KiB  app/WebVR/lib/arm/libcontent_common_mojo_bindings_shared.cr.so
  41.8 KiB  app/WebVR/lib/arm/libcontent_public_common_mojo_bindings_shared.cr.so
  74.8 KiB  app/WebVR/lib/arm/libcrash_key.cr.so
  58.6 KiB  app/WebVR/lib/arm/libcrcrypto.cr.so
  13.7 KiB  app/WebVR/lib/arm/libdevice_base.cr.so
  38.2 KiB  app/WebVR/lib/arm/libdevice_event_log.cr.so
  13.6 KiB  app/WebVR/lib/arm/libdevice_features.cr.so
 520.6 KiB  app/WebVR/lib/arm/libdevice_gamepad.cr.so
   1.6 MiB  app/WebVR/lib/arm/libdevice_vr.cr.so
... and 244 more / 另有 244 个 / и ещё 244
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
