# pvr_apps_signed

English | [中文](#中文) | [Русский](#русский)

## What this is

Repack stage: the PVR apps re-signed with the local platform test key from the build repo, .idsig files alongside and rebuilt oat where present.

## How to remake this dump

Sign each apk with apksigner/signapk using the platform key from the build repo (the 106-109 and 133-134 script series in tools).

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

重打包阶段：用 build 仓库中的本地 platform 测试密钥重新签名的 PVR 应用，附带 .idsig 文件，有 oat 的也已重建。

### 如何重新制作这些转储

重新制作：用 build 仓库中的 platform 密钥通过 apksigner/signapk 给每个 apk 签名（tools 中的 106-109 与 133-134 系列脚本）。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Стадия пересборки: приложения PVR, переподписанные локальным тестовым ключом platform из репозитория build, с .idsig рядом и пересобранным oat где он есть.

### Как воспроизвести дамп

Воспроизведение: подписать каждый apk через apksigner/signapk ключом platform из репозитория build (серии скриптов 106-109 и 133-134 в tools).

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

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
