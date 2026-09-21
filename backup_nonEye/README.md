# backup_nonEye

English | [中文](#中文) | [Русский](#русский)

## What this is

Complete partition-by-partition backup of a Pico Neo 2 without the eye-tracking module (the 'non-Eye' unit): abl, aop, apdp, bluetooth, boot, cdt, cmnlib, cmnlib64, modem, recovery, system, vendor and the rest of the partition table. Kept as a safe rollback source and as a donor for stock blobs.

## How to remake this dump

On a stock device, dump each partition over adb: `adb shell su -c 'dd if=/dev/block/bootdevice/by-name/<part> of=/sdcard/<part>.img'` then `adb pull`. Alternatively read them out in EDL/firehose mode with a Sahara loader.

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.

## 中文

### 这是什么

不带眼动追踪模块的 Pico Neo 2（non-Eye 机器）的完整分区备份：abl、aop、apdp、bluetooth、boot、cdt、cmnlib、cmnlib64、modem、recovery、system、vendor 等全部分区。用于安全回滚，也作为官方 blob 的来源。

### 如何重新制作这些转储

重新制作：在原版设备上通过 adb 逐分区导出：`adb shell su -c 'dd if=/dev/block/bootdevice/by-name/<分区名> of=/sdcard/<分区名>.img'`，然后 `adb pull`；也可以用 EDL/firehose 模式配合 Sahara loader 读出。

### 许可证说明

本仓库中的 AGPL v3 许可证仅适用于本 README 以及我们原创的文件（脚本、配置、补丁、笔记）。下面列出的二进制文件属于 Pico（字节跳动）及/或相关厂商（高通、Tobii、NDI 等）的专有固件或软件，不是我们的作品，不受 AGPL 授权，不会在本仓库中分发（已被 gitignore 排除），也不得再分发。如需获取，请从你合法拥有的设备或固件中自行提取。

## Русский

### Что это

Полный бэкап всех разделов Pico Neo 2 без модуля айтрекинга (модель 'non-Eye'): abl, aop, apdp, bluetooth, boot, cdt, cmnlib, cmnlib64, modem, recovery, system, vendor и остальные разделы. Хранится для отката и как источник стоковых блобов.

### Как воспроизвести дамп

Воспроизведение: на стоковом устройстве через adb — `adb shell su -c 'dd if=/dev/block/bootdevice/by-name/<раздел> of=/sdcard/<раздел>.img'`, затем `adb pull`. Либо через режим EDL/firehose с загрузчиком Sahara.

### Лицензия

Лицензия AGPL v3 в этом репозитории распространяется ТОЛЬКО на этот README и другие наши собственные работы (скрипты, конфиги, патчи, заметки). Перечисленные ниже бинарные файлы являются проприетарной прошивкой или ПО компании Pico (ByteDance) и/или соответствующих вендоров (Qualcomm, Tobii, NDI и др.). Они не являются нашей работой, не покрываются AGPL, не распространяются в этом репозитории (исключены через gitignore) и не подлежат повторному распространению. Чтобы получить их, извлеките их из оборудования или прошивки, которыми вы легально владеете.

## Files in this folder / 本目录文件 / Файлы в этой папке

46 files / 共 46 个文件 / всего файлов: 46

```
   1.0 MiB  ImageFv.img
   1.0 MiB  abl.img
 512.0 KiB  aop.img
 256.0 KiB  apdp.img
   1.0 MiB  bluetooth.img
  64.0 MiB  boot.img
 128.0 KiB  cdt.img
 512.0 KiB  cmnlib.img
 512.0 KiB  cmnlib64.img
   1.0 MiB  ddr.img
 128.0 KiB  devcfg.img
   4.0 KiB  devinfo.img
   1.0 MiB  dip.img
  32.0 MiB  dsp.img
   8.0 MiB  dtbo.img
 512.0 KiB  frp.img
 128.0 KiB  fsc.img
   2.0 MiB  fsg.img
 512.0 KiB  hyp.img
 512.0 KiB  keymaster.img
 512.0 KiB  keystore.img
   4.0 KiB  limits.img
   8.0 MiB  logfs.img
  32.0 MiB  mdtp.img
   4.0 MiB  mdtpsecapp.img
   1.0 MiB  misc.img
 120.0 MiB  modem.img
   2.0 MiB  modemst1.img
   2.0 MiB  modemst2.img
 256.0 KiB  msadp.img
  32.0 MiB  persist.img
   4.0 KiB  picocfg.img
  64.0 KiB  qupfw.img
  64.0 MiB  recovery.img
  16.0 KiB  sec.img
  32.6 MiB  splash.img
   8.0 MiB  spunvm.img
   8.0 KiB  ssd.img
   2.0 MiB  sti.img
 128.0 KiB  storsec.img
   1.0 MiB  toolsfv.img
   2.0 MiB  tz.img
  64.0 KiB  vbmeta.img
 408.4 MiB  vendor.img
   3.5 MiB  xbl.img
 128.0 KiB  xbl_config.img
```
