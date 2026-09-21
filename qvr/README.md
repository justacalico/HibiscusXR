# qvr

English | [中文](#中文) | [Русский](#русский)

## What this is

Qualcomm QVR service client libraries pulled from stock: libqvrcamera_client and libqvrservice_client in both 32 and 64 bit.

## How to remake this dump

adb pull or `debugfs -R rdump` of the libqvr* libraries from /system/lib and /system/lib64 on the stock image.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从官方系统提取的高通 QVR 服务客户端库：32 位和 64 位的 libqvrcamera_client 与 libqvrservice_client。

### 如何重新制作这些转储

重新获取：从官方镜像的 /system/lib 和 /system/lib64 中 adb pull 或 `debugfs -R rdump` 导出 libqvr* 库。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Клиентские библиотеки Qualcomm QVR из стока: libqvrcamera_client и libqvrservice_client в 32- и 64-битных версиях.

### Как воспроизвести дамп

Воспроизведение: `adb pull` или `debugfs -R rdump` библиотек libqvr* из /system/lib и /system/lib64 стокового образа.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

4 files / 共 4 个文件 / всего файлов: 4

```
 118.9 KiB  lib/libqvrcamera_client.so
  98.9 KiB  lib/libqvrservice_client.so
 140.8 KiB  lib64/libqvrcamera_client.so
 120.6 KiB  lib64/libqvrservice_client.so
```
