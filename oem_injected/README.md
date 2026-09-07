# oem_injected

English | [中文](#中文) | [Русский](#русский)

## What this is

The /oem apps with extra native libraries injected into the apk (for example ToBToolService carrying lib/arm64/libPvr_UnitySDK.so) before re-signing.

## How to remake this dump

Inject the required .so files into the apk zip, then rebuild and sign via tools/133_oem_build.py.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

在重新签名之前，向 apk 中注入了额外原生库的 /oem 应用（例如 ToBToolService 注入了 lib/arm64/libPvr_UnitySDK.so）。

### 如何重新制作这些转储

重新制作：把需要的 .so 注入 apk 压缩包，再用 tools/133_oem_build.py 重打包并签名。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Приложения /oem с внедрёнными в apk нативными библиотеками (например, ToBToolService с lib/arm64/libPvr_UnitySDK.so) перед переподписью.

### Как воспроизвести дамп

Воспроизведение: внедрить нужные .so в zip apk, затем пересобрать и подписать через tools/133_oem_build.py.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

6 files / 共 6 个文件 / всего файлов: 6

```
   3.8 MiB  PVRHome/PVRHome.apk
 991.1 KiB  PVRLauncher/PVRLauncher.apk
 700.9 KiB  ToBToolService/ToBToolService.apk
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
   2.9 MiB  provision2d/provision2d.apk
   3.2 MiB  store2d/store2d.apk
```
