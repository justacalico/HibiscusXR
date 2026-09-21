# overlay_pvr

English | [中文](#中文) | [Русский](#русский)

## What this is

Pico's overlay files pulled from stock - the resource overlays Pico layers on top of AOSP, plus public.libraries.txt carrying stock's extra public-library entries that app linkers need to dlopen the Pico libs.

## How to remake this dump

Pulled with adb / read out of the stock system.img. The linker whitelist is /system/etc/public.libraries.txt.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从官方系统提取的 Pico overlay 文件——Pico 叠加在 AOSP 之上的资源覆盖层，以及带有官方额外公共库条目的 public.libraries.txt（应用的 linker 需要它来 dlopen Pico 的库）。

### 如何重新制作这些转储

重新获取：用 adb 拉取或从官方 system.img 中导出；linker 白名单位于 /system/etc/public.libraries.txt。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Overlay-файлы Pico, извлечённые из стока — ресурсные оверлеи, которые Pico накладывает поверх AOSP, плюс public.libraries.txt с дополнительными публичными библиотеками стока, нужными linker'у приложений для dlopen библиотек Pico.

### Как воспроизвести дамп

Воспроизведение: извлекаются через adb или из стокового system.img. Белый список linker'а — /system/etc/public.libraries.txt.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

11 files / 共 11 个文件 / всего файлов: 11

```
   2.6 MiB  lib/libImageGrid.so
   3.2 MiB  lib/libSafetyArea.so
  61.2 KiB  lib/libairclient.so
  19.8 KiB  lib/libdatabuffer.so
  36.6 KiB  lib/libvirtualinputclient.so
   4.3 MiB  lib64/libImageGrid.so
   5.5 MiB  lib64/libSafetyArea.so
  71.5 KiB  lib64/libairclient.so
  14.7 KiB  lib64/libdatabuffer.so
  39.4 KiB  lib64/libvirtualinputclient.so
   1.1 KiB  public.libraries.txt
```
