# pvr_apps_injected

English | [中文](#中文) | [Русский](#русский)

## What this is

Repack stage: apks with the needed native libraries injected into the package (for example VRShell2 carrying its own libPvr_UnitySDK) before final signing.

## How to remake this dump

Inject the .so files into the apk zip, then rebuild and sign with the same pipeline.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

重打包阶段：在最终签名前把所需原生库注入 apk 包内（例如 VRShell2 携带自己的 libPvr_UnitySDK）。

### 如何重新制作这些转储

重新制作：把 .so 文件注入 apk 压缩包，然后用同一条流水线重打包并签名。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Стадия пересборки: apk с внедрёнными нативными библиотеками (например, VRShell2 со своим libPvr_UnitySDK) перед финальной подписью.

### Как воспроизвести дамп

Воспроизведение: внедрить .so в zip apk, затем пересобрать и подписать тем же конвейером.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

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
