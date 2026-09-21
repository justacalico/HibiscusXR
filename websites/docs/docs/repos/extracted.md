# extracted

[HibiscusXR/dumps/extracted](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/dumps/extracted)

Artifacts decompiled and pulled out of the stock firmware for study.

```
bootclean/    stock boot image: kernel + rd/ (unpacked ramdisk)
bootpatch/    boot image after our patches, with rd/
boot_ota/     boot image straight from the OTA
dtb/          decompiled device trees (kernel_fdt_*.dts, one per hw variant)
props/        build.prop / default.prop dumps per partition
sysdirs/      directory listings of the stock system
vrbins/       the VR-related ELF binaries isolated for disassembly
```

The `dtb/` dts files are the source of truth for the hardware table on the
[Hardware](../internals/hardware.md) page. The three boot variants let you diff
exactly what changed in the ramdisk between stock and patched.
