# cdsp

English | [中文](#中文) | [Русский](#русский)

## What this is

Qualcomm CDSP RPC libraries pulled from the stock vendor image: libadsprpc_system, libcdsprpc_system, libsdsprpc_system, libcdsprpc and libmdsprpc. This is the user-space side of talking to the Hexagon compute DSP that the CV workloads run on.

## How to remake this dump

`adb pull` the matching paths from /vendor/lib, /vendor/lib64 and /vendor/cdsp on a stock device, or `debugfs -R rdump` them out of the rebuilt vendor.img under images/. See tools/278_cdsprpc.sh and 282_vendor_cdsp.sh.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从官方 vendor 镜像中提取的高通 CDSP RPC 库：libadsprpc_system、libcdsprpc_system、libsdsprpc_system、libcdsprpc、libmdsprpc。它们是用户空间与 Hexagon 计算 DSP（CV 负载运行的地方）通信的部分。

### 如何重新制作这些转储

重新获取：在原版设备上 `adb pull` /vendor/lib、/vendor/lib64、/vendor/cdsp 下的对应文件；或用 `debugfs -R rdump` 从 images/ 下重建的 vendor.img 中导出。参考 tools/278_cdsprpc.sh 和 282_vendor_cdsp.sh。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Библиотеки Qualcomm CDSP RPC, извлечённые из стокового vendor-образа: libadsprpc_system, libcdsprpc_system, libsdsprpc_system, libcdsprpc и libmdsprpc. Это пользовательская часть взаимодействия с Hexagon compute DSP, на котором работают CV-задачи.

### Как воспроизвести дамп

Воспроизведение: `adb pull` соответствующих путей из /vendor/lib, /vendor/lib64 и /vendor/cdsp на стоковом устройстве, либо `debugfs -R rdump` из пересобранного vendor.img в images/. См. tools/278_cdsprpc.sh и 282_vendor_cdsp.sh.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

9 files / 共 9 个文件 / всего файлов: 9

```
 111.8 KiB  lib/libadsprpc_system.so
 107.9 KiB  lib/libcdsprpc_system.so
 107.9 KiB  lib/libsdsprpc_system.so
 141.0 KiB  lib64/libadsprpc_system.so
 141.0 KiB  lib64/libcdsprpc_system.so
 141.0 KiB  lib64/libsdsprpc_system.so
 107.9 KiB  vendor/libcdsprpc.so
 107.8 KiB  vendor/libmdsprpc.so
  23.8 KiB  vendor/libqvr_cdsp_driver_stub.so
```
