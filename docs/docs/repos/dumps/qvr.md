# qvr

[gitlab.com/neosalsa/qvr](https://gitlab.com/neosalsa/qvr)

## What this is

Qualcomm QVR service client libraries pulled from stock: libqvrcamera_client and libqvrservice_client in both 32 and 64 bit.

## How to remake this dump

adb pull or `debugfs -R rdump` of the libqvr* libraries from /system/lib and /system/lib64 on the stock image.

## Files

4 files / 共 4 个文件 / всего файлов: 4

```
 118.9 KiB  lib/libqvrcamera_client.so
  98.9 KiB  lib/libqvrservice_client.so
 140.8 KiB  lib64/libqvrcamera_client.so
 120.6 KiB  lib64/libqvrservice_client.so
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
