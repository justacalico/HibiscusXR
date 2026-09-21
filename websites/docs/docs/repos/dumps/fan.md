# fan

[HibiscusXR/system/fan](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/fan)

## What this is

Stock thermal and fan control binaries: fancontrol and thermalserviced (ARM64 ELFs) plus the fanservice.rc init script.

## How to remake this dump

adb pull the binaries and the rc from a stock device - see tools/368_fan.sh and tools/370_fancontrol.sh.

## Files

3 files / 共 3 个文件 / всего файлов: 3

```
  25.2 KiB  fancontrol
      93 B  fanservice.rc
  31.8 KiB  thermalserviced
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
