# android

[gitlab.com/neosalsa/android](https://gitlab.com/neosalsa/android)

The LineageOS device tree for the Pico Neo 2: `device/pico/A7B10`.

```
device/pico/A7B10/
  AndroidProducts.mk      product registration
  BoardConfig.mk          board flags (arm64, kernel, partitions)
  device.mk               packages and feature flags
  lineage_A7B10.mk        the lineage_* product
  extract-files.sh        pulls proprietary files from a donor device/image
  proprietary-files.txt   the blob list
  proprietary-files-pvr.txt   PVR-specific blob list
  rootdir/etc/fstab.qcom  partition mount table
  README.md               hardware table + provenance notes
```

Every value in the tree was read out of the stock firmware rather than copied
from a similar sdm845 device - where something was derived rather than read
directly, the source is noted inline in the makefile.

Key facts the tree encodes: arm64, kernel 4.9.65-perf+, non-A/B partitioning,
VNDK 27 vendor (Android 8.1), no `super`.
