# pvr_applibs

[gitlab.com/neosalsa/pvr_applibs](https://gitlab.com/neosalsa/pvr_applibs)

## What this is

The app-private native libraries that sit beside each system apk in lib/<arch>/ - libmain, libunity, libil2cpp, libPvr_UnitySDK(CV), libCVController, the NDI and head/hand calibration libs and more. The first extraction took only apks and oat, so these were silently missing.

## How to remake this dump

tools/107_extract_applibs.sh - `debugfs -R rdump` of /app/<name>/lib/<arch> and /priv-app/<name>/lib/<arch> out of the stock system.img.

## Files

23 files / 共 23 个文件 / всего файлов: 23

```
   2.0 MiB  CVService/lib/arm/lib6DofFusion.so
 588.3 KiB  CVService/lib/arm/libCVController.so
 417.7 KiB  CVService/lib/arm/libHandImuCalibrate.so
 425.7 KiB  CVService/lib/arm/libHeadImuCalibrate_int.so
 892.1 KiB  CVService/lib/arm/libNDIExec.so
   1.0 MiB  CVService/lib/arm/libPvr_UnitySDKCV.so
 497.4 KiB  CVService/lib/arm/libSixDofProcessor.so
 593.3 KiB  CVService/lib/arm/libc++.so
  73.3 KiB  CVService/lib/arm/libclientWrapper.so
  41.9 KiB  CVService/lib/arm/libqvrservice_client.so
   5.7 KiB  InitServer/lib/arm64/libmt-jni.so
   2.1 MiB  PVRVerify/lib/arm/libsqlcipher.so
   1.4 MiB  PicoToSvrService/lib/arm64/libPvr_NativeSDK.so
   1.0 MiB  PicoToSvrService/lib/arm64/libgnustl_shared.so
   9.6 KiB  PicoToSvrService/lib/arm64/libsharedmem.so
   2.0 MiB  VRShell2/lib/arm64/libPvr_UnitySDK.so
  15.0 MiB  VRShell2/lib/arm64/libil2cpp.so
   5.7 KiB  VRShell2/lib/arm64/libinput_virtual_display.so
   6.0 KiB  VRShell2/lib/arm64/libmain.so
 286.3 KiB  VRShell2/lib/arm64/libnative.so
 269.9 KiB  VRShell2/lib/arm64/libtracking_module.so
  14.0 MiB  VRShell2/lib/arm64/libunity.so
   1.3 MiB  VRUserCenter2/lib/arm64/libUserCenterJni.so
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
