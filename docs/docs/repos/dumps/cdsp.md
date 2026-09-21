# cdsp

[gitlab.com/neosalsa/cdsp](https://gitlab.com/neosalsa/cdsp)

## What this is

Qualcomm CDSP RPC libraries pulled from the stock vendor image: libadsprpc_system, libcdsprpc_system, libsdsprpc_system, libcdsprpc and libmdsprpc. This is the user-space side of talking to the Hexagon compute DSP that the CV workloads run on.

## How to remake this dump

`adb pull` the matching paths from /vendor/lib, /vendor/lib64 and /vendor/cdsp on a stock device, or `debugfs -R rdump` them out of the rebuilt vendor.img under images/. See tools/278_cdsprpc.sh and 282_vendor_cdsp.sh.

## Files

9 files / 共 9 个文件 / всего файлов: 9

```
 111.8 KiB  lib/libadsprpc_system.so
 107.9 KiB  lib/libcdsprpc_system.so
 107.9 KiB  lib/libsdsprpc_system.so
 141.0 KiB  lib64/libadsprpc_system.so
 141.0 KiB  lib64/libcdsprpc_system.so
 141.0 KiB  lib64/libsdsprpc_system.so
 107.9 KiB  vendor/libcdsprpc.so
 107.8 KiB  vendor/libmdsprpc.so
  23.8 KiB  vendor/libqvr_cdsp_driver_stub.so
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
