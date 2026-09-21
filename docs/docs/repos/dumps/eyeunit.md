# eyeunit

[gitlab.com/neosalsa/eyeunit](https://gitlab.com/neosalsa/eyeunit)

## What this is

SPI flash dumps from a working eye-tracking unit: full 256 KiB reads (eye_262144.bin, eye_verify.bin) and the top-level bitmap pages (A exact, B page, C 32k, D full256k).

## How to remake this dump

Same w25q path as deadunit - the reads were analysed by tools/14_analyze_eye.sh (log: notes/14_eye_dump.log).

## Files

6 files / 共 6 个文件 / всего файлов: 6

```
 256.0 KiB  eye_262144.bin
 256.0 KiB  eye_verify.bin
  31.5 KiB  top_level_bitmap_A_exact.bin
  31.8 KiB  top_level_bitmap_B_page.bin
  32.0 KiB  top_level_bitmap_C_32k.bin
 256.0 KiB  top_level_bitmap_D_full256k.bin
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
