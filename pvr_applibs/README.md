# pvr_applibs

English | [中文](#中文) | [Русский](#русский)

## What this is

The app-private native libraries that sit beside each system apk in lib/<arch>/ - libmain, libunity, libil2cpp, libPvr_UnitySDK(CV), libCVController, the NDI and head/hand calibration libs and more. The first extraction took only apks and oat, so these were silently missing.

## How to remake this dump

tools/107_extract_applibs.sh - `debugfs -R rdump` of /app/<name>/lib/<arch> and /priv-app/<name>/lib/<arch> out of the stock system.img.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

位于每个系统应用 apk 旁边 lib/<arch>/ 下的应用私有原生库——libmain、libunity、libil2cpp、libPvr_UnitySDK(CV)、libCVController、NDI 以及头/手校准库等。第一次提取只拿了 apk 和 oat，这些库当时被漏掉了。

### 如何重新制作这些转储

重新获取：tools/107_extract_applibs.sh——用 `debugfs -R rdump` 从官方 system.img 导出 /app/<名称>/lib/<arch> 和 /priv-app/<名称>/lib/<arch>。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Приватные нативные библиотеки приложений, лежащие рядом с каждым системным apk в lib/<arch>/ — libmain, libunity, libil2cpp, libPvr_UnitySDK(CV), libCVController, библиотеки NDI и калибровки головы/рук и др. Первая выгрузка брала только apk и oat, поэтому они были пропущены.

### Как воспроизвести дамп

Воспроизведение: tools/107_extract_applibs.sh — `debugfs -R rdump` каталогов /app/<имя>/lib/<arch> и /priv-app/<имя>/lib/<arch> из стокового system.img.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

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
