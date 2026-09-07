# deadunit

[gitlab.com/neosalsa/deadunit](https://gitlab.com/neosalsa/deadunit)

## What this is

Raw SPI NOR flash dumps read off the eye-tracking board of a dead Pico Neo 2 unit: d8k.bin, dfull.bin, prewrite_262144.bin, rb.bin and the FalconCV2 controller firmware bins under mybin/. Used to compare against, and recover, the firmware on a live board.

## How to remake this dump

The w25q flash is read with the on-device flasher ndi_firmware/*/w25q_write_bin or tools/w25qdump over the FPGA's SPI bus. The analysis is in notes/12_w25q.log, tools/12_w25q.sh and 13_analyze_dumps.sh.

## Files

7 files / 共 7 个文件 / всего файлов: 7

```
   8.0 KiB  d8k.bin
 256.0 KiB  dfull.bin
 124.6 KiB  mybin/FalconCV2Ctrl_sv1.10_Nv0.0.0_20200429_b110.bin
 321.4 KiB  mybin/FalconCV2Ctrl_sv1.10_Nv2.3.4_20200429_b110.bin
  76.1 KiB  mybin/FalconCV2Sta_sv0.66_20200729_b70.bin
 256.0 KiB  prewrite_262144.bin
   8.0 KiB  rb.bin
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
