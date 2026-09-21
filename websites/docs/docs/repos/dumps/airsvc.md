# airsvc

[HibiscusXR/system/airsvc](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/airsvc)

## What this is

The Pico airservice pieces pulled from stock: the airservice and virtual_input ELF daemons plus their init rc files (init/airservice.rc, init/virtual_input.rc). These are two of the missing daemons Android 10 needed.

## How to remake this dump

adb pull /system/bin/airservice, /system/bin/virtual_input and the matching rc files from /system/etc/init - or rdump them from the stock system.img.

## Files

18 files / 共 18 个文件 / всего файлов: 18

```
  24.4 KiB  bin/airclient_test
  43.3 KiB  bin/airservice
  27.6 KiB  bin/virtual_input
      86 B  init/airservice.rc
     191 B  init/virtual_input.rc
  57.0 KiB  lib/libaircamera.so
 126.8 KiB  lib/libairservice.so
   1.7 MiB  lib/libicui18n.so
   1.3 MiB  lib/libicuuc.so
   5.6 MiB  lib/libskia.so
  28.1 KiB  lib/libvirtualinput.so
  55.4 KiB  lib64/libaircamera.so
 168.0 KiB  lib64/libairservice.so
   2.4 MiB  lib64/libicui18n.so
   1.7 MiB  lib64/libicuuc.so
   8.7 MiB  lib64/libskia.so
  63.2 KiB  lib64/libtinyxml2.so
  30.7 KiB  lib64/libvirtualinput.so
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
