# pvr_apps_dexed

English | [中文](#中文) | [Русский](#русский)

## What this is

First stage of the repack pipeline: the pvr_apps apks deodexed, dex code recovered from the odex/vdex so the apps can be modified and rebuilt.

## How to remake this dump

Run the unquicken/deodex pass over pvr_apps (same tooling as tools/132_deodex_oem.sh).

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

重打包流水线的第一阶段：pvr_apps 的 apk 经过反优化，从 odex/vdex 中还原出 dex 代码，以便修改和重建。

### 如何重新制作这些转储

重新制作：对 pvr_apps 执行 unquicken/deodex 处理（与 tools/132_deodex_oem.sh 相同的工具链）。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Первая стадия конвейера пересборки: apk из pvr_apps после деодексирования — dex-код восстановлен из odex/vdex, чтобы приложения можно было модифицировать и пересобрать.

### Как воспроизвести дамп

Воспроизведение: прогнать unquicken/deodex по pvr_apps (тот же инструментарий, что в tools/132_deodex_oem.sh).

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

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
