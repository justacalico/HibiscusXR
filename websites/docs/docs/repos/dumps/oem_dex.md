# oem_dex

[gitlab.com/neosalsa/oem_dex](https://gitlab.com/neosalsa/oem_dex)

## What this is

Deodex stage for the /oem apps: the dex code extracted back out of each app's odex/vdex, with an _extract.log per app.

## How to remake this dump

tools/132_deodex_oem.sh runs the deodex (unquicken) pass over oem_apps/.

## Files

10 files / 共 10 个文件 / всего файлов: 10

```
   5.1 MiB  PVRHome/PVRHome_classes.dex
     268 B  PVRHome/_extract.log
 892.2 KiB  PVRLauncher/PVRLauncher_classes.dex
     280 B  PVRLauncher/_extract.log
 719.5 KiB  ToBToolService/ToBToolService_classes.dex
     289 B  ToBToolService/_extract.log
     280 B  provision2d/_extract.log
   2.4 MiB  provision2d/provision2d_classes.dex
     268 B  store2d/_extract.log
   5.6 MiB  store2d/store2d_classes.dex
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
