# images

[HibiscusXR/dumps/images](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/dumps/images)

## What this is

Stock PUI 4.1.3 firmware: the ota_4.1.3 OTA payload directory (boot.img, dtbo.img, dspso.bin, NON-HLOS.bin, BTFM.bin, oem.bin, transfer lists, vendor.new.dat(.br), file_contexts.bin, metadata, updater-script), the vendor.img rebuilt from it, and snapshot_lun0/ - a full snapshot of UFS LUN0 including the 3.7 GB system.bin.

## How to remake this dump

The OTA files come from the official PUI 4.1.3 update package. vendor.img is rebuilt with `brotli -d vendor.new.dat.br` then `python3 tools/03_sdat2img.py vendor.transfer.list vendor.new.dat vendor.img`. snapshot_lun0 was read off the device block device / via firehose.

## Files

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

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
