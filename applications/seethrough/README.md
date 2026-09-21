# seethrough

English | [中文](#中文) | [Русский](#русский)

## What this is

The seethroughsetting app pulled from stock: seethroughsetting.apk plus its signed variant and the app-private lib/arm64 native libraries (libil2cpp, libmain, libunity, libnative(-lib), libPvr_UnitySDK, libtracking_module).

## How to remake this dump

adb pull /system/priv-app/seethroughsetting (or rdump it from system.img), then sign with the platform key.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从官方系统提取的 seethroughsetting 应用：seethroughsetting.apk 及其签名版本，以及应用私有的 lib/arm64 原生库（libil2cpp、libmain、libunity、libnative(-lib)、libPvr_UnitySDK、libtracking_module）。

### 如何重新制作这些转储

重新获取：`adb pull` /system/priv-app/seethroughsetting（或从 system.img 中 rdump 导出），然后用 platform 密钥签名。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Приложение seethroughsetting, извлечённое из стока: seethroughsetting.apk и его подписанный вариант, плюс приватные нативные библиотеки lib/arm64 (libil2cpp, libmain, libunity, libnative(-lib), libPvr_UnitySDK, libtracking_module).

### Как воспроизвести дамп

Воспроизведение: `adb pull` /system/priv-app/seethroughsetting (или rdump из system.img), затем подписать ключом platform.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

10 files / 共 10 个文件 / всего файлов: 10

```
   2.0 MiB  lib/arm64/libPvr_UnitySDK.so
  17.3 MiB  lib/arm64/libil2cpp.so
   6.0 KiB  lib/arm64/libmain.so
 182.1 KiB  lib/arm64/libnative-lib.so
 286.3 KiB  lib/arm64/libnative.so
 269.9 KiB  lib/arm64/libtracking_module.so
  13.0 MiB  lib/arm64/libunity.so
 193.8 MiB  seethroughsetting-signed.apk
   1.5 MiB  seethroughsetting-signed.apk.idsig
 193.8 MiB  seethroughsetting.apk
```
