# airsvc

English | [中文](#中文) | [Русский](#русский)

## What this is

The Pico airservice pieces pulled from stock: the airservice and virtual_input ELF daemons plus their init rc files (init/airservice.rc, init/virtual_input.rc). These are two of the missing daemons Android 10 needed.

## How to remake this dump

adb pull /system/bin/airservice, /system/bin/virtual_input and the matching rc files from /system/etc/init - or rdump them from the stock system.img.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从官方系统提取的 Pico airservice 组件：airservice 和 virtual_input 两个 ELF 守护进程及其 init rc 文件（init/airservice.rc、init/virtual_input.rc）。这是 Android 10 缺失的两个守护进程。

### 如何重新制作这些转储

重新获取：`adb pull` /system/bin/airservice、/system/bin/virtual_input 及 /system/etc/init 下对应的 rc 文件，或从官方 system.img 中 rdump 导出。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Компоненты Pico airservice, извлечённые из стока: ELF-демоны airservice и virtual_input плюс их init rc-файлы (init/airservice.rc, init/virtual_input.rc). Это два демона, которых не хватало под Android 10.

### Как воспроизвести дамп

Воспроизведение: `adb pull` /system/bin/airservice, /system/bin/virtual_input и соответствующие rc-файлы из /system/etc/init, либо rdump из стокового system.img.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

18 files / 共 18 个文件 / всего файлов: 18

```
  24.4 KiB  bin/airclient_test
  43.3 KiB  bin/airservice
  27.6 KiB  bin/virtual_input
      86 B  init/airservice.rc
     191 B  init/virtual_input.rc
  57.0 KiB  lib/libaircamera.so
 126.8 KiB  lib/libairservice.so
   1.7 MiB  lib/libicui18n.so
   1.3 MiB  lib/libicuuc.so
   5.6 MiB  lib/libskia.so
  28.1 KiB  lib/libvirtualinput.so
  55.4 KiB  lib64/libaircamera.so
 168.0 KiB  lib64/libairservice.so
   2.4 MiB  lib64/libicui18n.so
   1.7 MiB  lib64/libicuuc.so
   8.7 MiB  lib64/libskia.so
  63.2 KiB  lib64/libtinyxml2.so
  30.7 KiB  lib64/libvirtualinput.so
```
