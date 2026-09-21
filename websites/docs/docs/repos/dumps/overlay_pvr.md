# overlay_pvr

[HibiscusXR/system/overlay_pvr](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/overlay_pvr)

## What this is

Pico's overlay files pulled from stock - the resource overlays Pico layers on top of AOSP, plus public.libraries.txt carrying stock's extra public-library entries that app linkers need to dlopen the Pico libs.

## How to remake this dump

Pulled with adb / read out of the stock system.img. The linker whitelist is /system/etc/public.libraries.txt.

## Files

11 files / 共 11 个文件 / всего файлов: 11

```
   2.6 MiB  lib/libImageGrid.so
   3.2 MiB  lib/libSafetyArea.so
  61.2 KiB  lib/libairclient.so
  19.8 KiB  lib/libdatabuffer.so
  36.6 KiB  lib/libvirtualinputclient.so
   4.3 MiB  lib64/libImageGrid.so
   5.5 MiB  lib64/libSafetyArea.so
  71.5 KiB  lib64/libairclient.so
  14.7 KiB  lib64/libdatabuffer.so
  39.4 KiB  lib64/libvirtualinputclient.so
   1.1 KiB  public.libraries.txt
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
