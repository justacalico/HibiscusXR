# ndi_firmware

[gitlab.com/neosalsa/ndi_firmware](https://gitlab.com/neosalsa/ndi_firmware)

## What this is

NDI eye-tracker firmware packages shipped inside PUI - one firehose directory per firmware train: EYE_pui4.1.3_firehose, NONEYE_pui3.11.3_b255, NONEYE_pui4.1.0_b336, NONEYE_pui4.1.3_b346. Each contains the w25q_write_bin ARM64 flasher ELF used to write the eye board's SPI flash.

## How to remake this dump

Extracted from the stock ROM and the device's firmware-update paths; the hunt is documented in notes/09a-10 and tools/09_ndi.sh, 10_ndi_firmware.sh.

## Files

4 files / 共 4 个文件 / всего файлов: 4

```
  10.8 KiB  EYE_pui4.1.3_firehose/w25q_write_bin
  10.8 KiB  NONEYE_pui3.11.3_b255/w25q_write_bin
  10.8 KiB  NONEYE_pui4.1.0_b336/w25q_write_bin
  10.8 KiB  NONEYE_pui4.1.3_b346/w25q_write_bin
```

## License

The AGPL v3 license in this repository applies ONLY to this README and to other files that are our own original work (scripts, configs, patches, notes). The binary files listed below are proprietary firmware or software belonging to Pico (ByteDance) and/or their respective vendors (Qualcomm, Tobii, NDI, and others). They are not our work, they are not covered by the AGPL, they are not distributed in this repository (they are gitignored), and they may not be redistributed. To obtain them, dump them from hardware or firmware you legally own.
