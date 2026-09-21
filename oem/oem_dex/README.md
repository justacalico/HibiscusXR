# oem_dex

English | [中文](#中文) | [Русский](#русский)

## What this is

Deodex stage for the /oem apps: the dex code extracted back out of each app's odex/vdex, with an _extract.log per app.

## How to remake this dump

tools/132_deodex_oem.sh runs the deodex (unquicken) pass over oem_apps/.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

/oem 应用的反优化（deodex）阶段：从每个应用的 odex/vdex 中还原出的 dex 代码，每个应用附一个 _extract.log。

### 如何重新制作这些转储

重新制作：tools/132_deodex_oem.sh 对 oem_apps/ 执行 deodex（unquicken）处理。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Стадия деодексирования приложений /oem: dex-код, извлечённый обратно из odex/vdex каждого приложения, с _extract.log на каждое.

### Как воспроизвести дамп

Воспроизведение: tools/132_deodex_oem.sh запускает деодекс (unquicken) по oem_apps/.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

10 files / 共 10 个文件 / всего файлов: 10

```
   5.1 MiB  PVRHome/PVRHome_classes.dex
     268 B  PVRHome/_extract.log
 892.2 KiB  PVRLauncher/PVRLauncher_classes.dex
     280 B  PVRLauncher/_extract.log
 719.5 KiB  ToBToolService/ToBToolService_classes.dex
     289 B  ToBToolService/_extract.log
     280 B  provision2d/_extract.log
   2.4 MiB  provision2d/provision2d_classes.dex
     268 B  store2d/_extract.log
   5.6 MiB  store2d/store2d_classes.dex
```
