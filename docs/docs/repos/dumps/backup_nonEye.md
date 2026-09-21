# backup_nonEye

[gitlab.com/neosalsa/backup_nonEye](https://gitlab.com/neosalsa/backup_nonEye)

## What this is

Complete partition-by-partition backup of a Pico Neo 2 without the eye-tracking module (the 'non-Eye' unit): abl, aop, apdp, bluetooth, boot, cdt, cmnlib, cmnlib64, modem, recovery, system, vendor and the rest of the partition table. Kept as a safe rollback source and as a donor for stock blobs.

## How to remake this dump

On a stock device, dump each partition over adb: `adb shell su -c 'dd if=/dev/block/bootdevice/by-name/<part> of=/sdcard/<part>.img'` then `adb pull`. Alternatively read them out in EDL/firehose mode with a Sahara loader.

## Files

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

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
