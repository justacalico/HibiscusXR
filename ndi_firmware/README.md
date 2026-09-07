# ndi_firmware

English | [中文](#中文) | [Русский](#русский)

## What this is

NDI eye-tracker firmware packages shipped inside PUI - one firehose directory per firmware train: EYE_pui4.1.3_firehose, NONEYE_pui3.11.3_b255, NONEYE_pui4.1.0_b336, NONEYE_pui4.1.3_b346. Each contains the w25q_write_bin ARM64 flasher ELF used to write the eye board's SPI flash.

## How to remake this dump

Extracted from the stock ROM and the device's firmware-update paths; the hunt is documented in notes/09a-10 and tools/09_ndi.sh, 10_ndi_firmware.sh.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

PUI 自带的 NDI 眼动仪固件包——每个固件分支一个 firehose 目录：EYE_pui4.1.3_firehose、NONEYE_pui3.11.3_b255、NONEYE_pui4.1.0_b336、NONEYE_pui4.1.3_b346。每个目录包含用于写入眼动板 SPI 闪存的 ARM64 烧写器 w25q_write_bin。

### 如何重新制作这些转储

重新获取：从官方 ROM 和设备的固件更新路径中提取；查找过程记录在 notes/09a-10 以及 tools/09_ndi.sh、10_ndi_firmware.sh。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Пакеты прошивок айтрекера NDI, поставляемые внутри PUI — по одному firehose-каталогу на ветку прошивки: EYE_pui4.1.3_firehose, NONEYE_pui3.11.3_b255, NONEYE_pui4.1.0_b336, NONEYE_pui4.1.3_b346. В каждом лежит ARM64 ELF-прошивальщик w25q_write_bin для записи SPI флеша платы.

### Как воспроизвести дамп

Воспроизведение: извлечено из стокового ROM и путей обновления прошивки на устройстве; процесс задокументирован в notes/09a-10 и tools/09_ndi.sh, 10_ndi_firmware.sh.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

4 files / 共 4 个文件 / всего файлов: 4

```
  10.8 KiB  EYE_pui4.1.3_firehose/w25q_write_bin
  10.8 KiB  NONEYE_pui3.11.3_b255/w25q_write_bin
  10.8 KiB  NONEYE_pui4.1.0_b336/w25q_write_bin
  10.8 KiB  NONEYE_pui4.1.3_b346/w25q_write_bin
```
