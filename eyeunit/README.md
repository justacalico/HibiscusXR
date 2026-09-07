# eyeunit

English | [中文](#中文) | [Русский](#русский)

## What this is

SPI flash dumps from a working eye-tracking unit: full 256 KiB reads (eye_262144.bin, eye_verify.bin) and the top-level bitmap pages (A exact, B page, C 32k, D full256k).

## How to remake this dump

Same w25q path as deadunit - the reads were analysed by tools/14_analyze_eye.sh (log: notes/14_eye_dump.log).

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

来自正常工作的眼动追踪单元的 SPI 闪存转储：完整 256 KiB 读取（eye_262144.bin、eye_verify.bin）以及顶层位图各页（A 精确、B 页、C 32k、D 全 256k）。

### 如何重新制作这些转储

重新制作：与 deadunit 相同的 w25q 读取方式；分析脚本为 tools/14_analyze_eye.sh（日志见 notes/14_eye_dump.log）。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Дампы SPI флеша с рабочего модуля айтрекинга: полные чтения по 256 КиБ (eye_262144.bin, eye_verify.bin) и страницы верхнеуровневой битовой карты (A exact, B page, C 32k, D full256k).

### Как воспроизвести дамп

Воспроизведение: тот же путь через w25q, что и для deadunit; чтения анализировались tools/14_analyze_eye.sh (лог: notes/14_eye_dump.log).

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

6 files / 共 6 个文件 / всего файлов: 6

```
 256.0 KiB  eye_262144.bin
 256.0 KiB  eye_verify.bin
  31.5 KiB  top_level_bitmap_A_exact.bin
  31.8 KiB  top_level_bitmap_B_page.bin
  32.0 KiB  top_level_bitmap_C_32k.bin
 256.0 KiB  top_level_bitmap_D_full256k.bin
```
