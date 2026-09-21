# oem_apps

English | [中文](#中文) | [Русский](#русский)

## What this is

Raw pulls of the Pico apps living on the /oem partition: PVRLauncher, PVRHome, store2d, provision2d and ToBToolService, each with its apk plus oat/arm64 odex and vdex. /oem is not in the OTA package, so a stock device is the only source.

## How to remake this dump

`adb pull /oem/priv-app/<name>` on a stock device - see tools/131_pull_oem.ps1.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从 /oem 分区直接拉取的 Pico 应用：PVRLauncher、PVRHome、store2d、provision2d、ToBToolService，每个包含 apk 及 oat/arm64 下的 odex 和 vdex。/oem 不在 OTA 包里，只能从原版设备上获取。

### 如何重新制作这些转储

重新获取：在原版设备上 `adb pull /oem/priv-app/<应用名>`，见 tools/131_pull_oem.ps1。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Сырые копии приложений Pico с раздела /oem: PVRLauncher, PVRHome, store2d, provision2d и ToBToolService — каждое со своим apk и odex/vdex в oat/arm64. /oem отсутствует в OTA-пакете, поэтому единственный источник — стоковое устройство.

### Как воспроизвести дамп

Воспроизведение: `adb pull /oem/priv-app/<имя>` на стоковом устройстве — см. tools/131_pull_oem.ps1.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

16 files / 共 16 个文件 / всего файлов: 16

```
   1.8 MiB  PVRHome/PVRHome.apk
 124.7 KiB  PVRHome/oat/arm64/PVRHome.odex
   5.7 MiB  PVRHome/oat/arm64/PVRHome.vdex
 610.1 KiB  PVRLauncher/PVRLauncher.apk
  32.7 KiB  PVRLauncher/oat/arm64/PVRLauncher.odex
1011.5 KiB  PVRLauncher/oat/arm64/PVRLauncher.vdex
 394.8 KiB  ToBToolService/ToBToolService.apk
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
  36.7 KiB  ToBToolService/oat/arm64/ToBToolService.odex
 801.3 KiB  ToBToolService/oat/arm64/ToBToolService.vdex
  56.7 KiB  provision2d/oat/arm64/provision2d.odex
   2.7 MiB  provision2d/oat/arm64/provision2d.vdex
   1.9 MiB  provision2d/provision2d.apk
 124.7 KiB  store2d/oat/arm64/store2d.odex
   6.2 MiB  store2d/oat/arm64/store2d.vdex
 966.7 KiB  store2d/store2d.apk
```
