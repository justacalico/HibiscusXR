# deadunit

English | [中文](#中文) | [Русский](#русский)

## What this is

Raw SPI NOR flash dumps read off the eye-tracking board of a dead Pico Neo 2 unit: d8k.bin, dfull.bin, prewrite_262144.bin, rb.bin and the FalconCV2 controller firmware bins under mybin/. Used to compare against, and recover, the firmware on a live board.

## How to remake this dump

The w25q flash is read with the on-device flasher ndi_firmware/*/w25q_write_bin or tools/w25qdump over the FPGA's SPI bus. The analysis is in notes/12_w25q.log, tools/12_w25q.sh and 13_analyze_dumps.sh.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

从一台损坏的 Pico Neo 2 眼动追踪板上读出的 SPI NOR 闪存原始转储：d8k.bin、dfull.bin、prewrite_262144.bin、rb.bin 以及 mybin/ 下的 FalconCV2 控制器固件。用于与正常板子对比和恢复固件。

### 如何重新制作这些转储

重新制作：通过 FPGA 的 SPI 总线，用设备上的烧写器 ndi_firmware/*/w25q_write_bin 或 tools/w25qdump 读取 w25q 闪存。分析过程见 notes/12_w25q.log、tools/12_w25q.sh 和 13_analyze_dumps.sh。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Сырые дампы SPI NOR флеша, считанные с платы айтрекинга мёртвого Pico Neo 2: d8k.bin, dfull.bin, prewrite_262144.bin, rb.bin и бинарники контроллера FalconCV2 в mybin/. Используются для сравнения и восстановления прошивки на живой плате.

### Как воспроизвести дамп

Воспроизведение: флеш w25q читается прошивальщиком на устройстве ndi_firmware/*/w25q_write_bin или tools/w25qdump через SPI-шину FPGA. Анализ — в notes/12_w25q.log, tools/12_w25q.sh и 13_analyze_dumps.sh.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

7 files / 共 7 个文件 / всего файлов: 7

```
   8.0 KiB  d8k.bin
 256.0 KiB  dfull.bin
 124.6 KiB  mybin/FalconCV2Ctrl_sv1.10_Nv0.0.0_20200429_b110.bin
 321.4 KiB  mybin/FalconCV2Ctrl_sv1.10_Nv2.3.4_20200429_b110.bin
  76.1 KiB  mybin/FalconCV2Sta_sv0.66_20200729_b70.bin
 256.0 KiB  prewrite_262144.bin
   8.0 KiB  rb.bin
```
