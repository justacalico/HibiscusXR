# fan

English | [中文](#中文) | [Русский](#русский)

## What this is

Stock thermal and fan control binaries: fancontrol and thermalserviced (ARM64 ELFs) plus the fanservice.rc init script.

## How to remake this dump

adb pull the binaries and the rc from a stock device - see tools/368_fan.sh and tools/370_fancontrol.sh.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

官方的风扇与温控二进制文件：fancontrol 和 thermalserviced（ARM64 ELF），以及 fanservice.rc 启动脚本。

### 如何重新制作这些转储

重新获取：从原版设备 adb pull 这些二进制和 rc 文件，见 tools/368_fan.sh 和 tools/370_fancontrol.sh。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Стоковые бинарники управления вентилятором и температурой: fancontrol и thermalserviced (ARM64 ELF) плюс init-скрипт fanservice.rc.

### Как воспроизвести дамп

Воспроизведение: `adb pull` бинарников и rc-файла со стокового устройства — см. tools/368_fan.sh и tools/370_fancontrol.sh.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

3 files / 共 3 个文件 / всего файлов: 3

```
  25.2 KiB  fancontrol
      93 B  fanservice.rc
  31.8 KiB  thermalserviced
```
