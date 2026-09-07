# oem_final

English | [中文](#中文) | [Русский](#русский)

## What this is

Final repacked /oem apps: each apk rebuilt - unquickened dex, re-signed, .idsig alongside - ready to push back to a device.

## How to remake this dump

tools/133_oem_build.py repacks and signs; tools/134_install_oem.ps1 pushes them.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

重新打包完成的 /oem 应用：每个 apk 已重建——还原的 dex、重新签名、附带 .idsig——可直接推回设备安装。

### 如何重新制作这些转储

重新制作：tools/133_oem_build.py 负责重打包和签名；tools/134_install_oem.ps1 负责推送安装。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Финально пересобранные приложения /oem: каждый apk пересобран — деодексированный dex, переподпись, .idsig рядом — готов к установке на устройство.

### Как воспроизвести дамп

Воспроизведение: tools/133_oem_build.py пересобирает и подписывает; tools/134_install_oem.ps1 устанавливает.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

11 files / 共 11 个文件 / всего файлов: 11

```
   3.8 MiB  PVRHome/PVRHome.apk
  37.8 KiB  PVRHome/PVRHome.apk.idsig
 995.1 KiB  PVRLauncher/PVRLauncher.apk
  13.8 KiB  PVRLauncher/PVRLauncher.apk.idsig
 703.6 KiB  ToBToolService/ToBToolService.apk
  13.8 KiB  ToBToolService/ToBToolService.apk.idsig
 549.5 KiB  ToBToolService/lib/arm64/libPvr_UnitySDK.so
   2.9 MiB  provision2d/provision2d.apk
  29.8 KiB  provision2d/provision2d.apk.idsig
   3.2 MiB  store2d/store2d.apk
  33.8 KiB  store2d/store2d.apk.idsig
```
