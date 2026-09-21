# images

English | [中文](#中文) | [Русский](#русский)

## What this is

Stock PUI 4.1.3 firmware: the ota_4.1.3 OTA payload directory (boot.img, dtbo.img, dspso.bin, NON-HLOS.bin, BTFM.bin, oem.bin, transfer lists, vendor.new.dat(.br), file_contexts.bin, metadata, updater-script), the vendor.img rebuilt from it, and snapshot_lun0/ - a full snapshot of UFS LUN0 including the 3.7 GB system.bin.

## How to remake this dump

The OTA files come from the official PUI 4.1.3 update package. vendor.img is rebuilt with `brotli -d vendor.new.dat.br` then `python3 tools/03_sdat2img.py vendor.transfer.list vendor.new.dat vendor.img`. snapshot_lun0 was read off the device block device / via firehose.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

官方 PUI 4.1.3 固件：ota_4.1.3 OTA 包目录（boot.img、dtbo.img、dspso.bin、NON-HLOS.bin、BTFM.bin、oem.bin、transfer list、vendor.new.dat(.br)、file_contexts.bin、metadata、updater-script）、从中重建的 vendor.img，以及 snapshot_lun0/ —— UFS LUN0 的完整快照，内含 3.7 GB 的 system.bin。

### 如何重新制作这些转储

重新制作：OTA 文件来自官方 PUI 4.1.3 升级包。vendor.img 用 `brotli -d vendor.new.dat.br` 解压后执行 `python3 tools/03_sdat2img.py vendor.transfer.list vendor.new.dat vendor.img` 重建。snapshot_lun0 是从设备块设备直接读取 / 通过 firehose 导出的。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Стоковая прошивка PUI 4.1.3: каталог OTA-пакета ota_4.1.3 (boot.img, dtbo.img, dspso.bin, NON-HLOS.bin, BTFM.bin, oem.bin, transfer-листы, vendor.new.dat(.br), file_contexts.bin, metadata, updater-script), пересобранный из него vendor.img и snapshot_lun0/ — полный снапшот UFS LUN0 с system.bin на 3.7 ГБ.

### Как воспроизвести дамп

Воспроизведение: файлы OTA взяты из официального пакета обновления PUI 4.1.3. vendor.img пересобирается: `brotli -d vendor.new.dat.br`, затем `python3 tools/03_sdat2img.py vendor.transfer.list vendor.new.dat vendor.img`. snapshot_lun0 считан с блочного устройства / через firehose.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

26 files / 共 26 个文件 / всего файлов: 26

```
 652.0 KiB  ota_4.1.3/BTFM.bin
  43.5 MiB  ota_4.1.3/NON-HLOS.bin
  64.0 MiB  ota_4.1.3/boot.img
  32.0 MiB  ota_4.1.3/dspso.bin
   8.0 MiB  ota_4.1.3/dtbo.img
   1.5 MiB  ota_4.1.3/file_contexts.bin
     179 B  ota_4.1.3/metadata
   1.9 GiB  ota_4.1.3/oem.bin
   3.7 GiB  ota_4.1.3/system.img
  19.2 KiB  ota_4.1.3/system.transfer.list
   3.3 KiB  ota_4.1.3/updater-script
   4.0 KiB  ota_4.1.3/vbmeta.img
 779.8 MiB  ota_4.1.3/vendor.new.dat
 189.0 MiB  ota_4.1.3/vendor.new.dat.br
       0 B  ota_4.1.3/vendor.patch.dat
   4.3 KiB  ota_4.1.3/vendor.transfer.list
 512.0 KiB  snapshot_lun0/frp.bin
   4.0 KiB  snapshot_lun0/gpt_backup0.bin
  12.0 KiB  snapshot_lun0/gpt_main0.bin
 512.0 KiB  snapshot_lun0/keystore.bin
   1.0 MiB  snapshot_lun0/misc.bin
  32.0 MiB  snapshot_lun0/persist.bin
   4.0 KiB  snapshot_lun0/picocfg.bin
   8.0 KiB  snapshot_lun0/ssd.bin
   3.7 GiB  snapshot_lun0/system.bin
   1.0 GiB  vendor.img
```
