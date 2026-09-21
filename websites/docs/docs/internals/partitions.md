# Partitions

Non-A/B device: single `system`, `vendor`, `oem`, `boot`, discrete `recovery`,
no `super`. Backed up partition-by-partition by
`tools/backup/backup_partitions.sh` into `backup_nonEye/`.

## The ones that matter

| partition | notes |
|---|---|
| `persist` | calibration data (lens, camera) - **irreplaceable, back up first** |
| `picocfg` | Pico device config |
| `modemst1/2`, `fsg`, `fsc` | modem state and calibration - irreplaceable |
| `boot` | kernel + ramdisk (4.9.65-perf+) |
| `dtbo` | device-tree overlay blob |
| `vbmeta` | verified-boot metadata |
| `recovery` | discrete recovery partition |
| `system` | what this project replaces (3.94 GB) |
| `vendor` | stock PUI vendor - **never touched** |
| `oem` | Pico apps: launcher, home, store, provision (not in the OTA) |

## The ones never to write

`xbl`, `xbl_config`, `abl`, `aop`, `tz`, `hyp`, `keymaster`, `cmnlib`,
`cmnlib64`, `sec`, `cdt`, `devcfg`, ... - the bootloader/trust chain. Writing
an older one than the anti-rollback fuse allows is a permanent hard-brick.

Full backup set: `abl aop apdp bluetooth boot cdt cmnlib cmnlib64 ddr devcfg
devinfo dip dsp frp fsc fsg hyp ImageFv keystore keymaster limits logfs mdtp
mdtpsecapp misc modem modemst1 modemst2 oem persist picocfg qupfw recovery sec
splash spunvm ssd sti storsec system toolsfv tz vendor vbmeta xbl xbl_config`.
