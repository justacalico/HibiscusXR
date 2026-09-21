# sensorpatch

[HibiscusXR/system/sensorpatch](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/sensorpatch)

## What this is

Working area for the two libsensorservice patches: the stock lib plus .orig/.patched/.p3 variants, the on-device test builds (ondevice.so, cur.so) and the libgui.so it links against. The patches stop Android 10's sensor asserts on Pico's event types 57/58/126/127 and the dynamic-sensor connect path.

## How to remake this dump

Binary-patch the stock libsensorservice.so as documented in the notes, and rebuild; the patch scripts live in the tools repo.

## Files

9 files / 共 9 个文件 / всего файлов: 9

```
 221.9 KiB  cur.so
 942.0 KiB  libgui.so
 221.9 KiB  libsensorservice.so
 221.9 KiB  libsensorservice.so.orig
 221.9 KiB  libsensorservice.so.p3
 221.9 KiB  libsensorservice.so.patched
 221.9 KiB  libsensorservice.so.patched2
 221.9 KiB  ondev_now.so
 221.9 KiB  ondevice.so
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
