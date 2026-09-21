# Flashing

`tools/build/flash_full.sh` handles it end to end, including the two
non-optional quirks:

```bash
build/flash_full.sh out/system-pn2-full.img
```

## The two hard-won rules

1. **`fastboot oem pico unlock` is required once per fastboot session** - and
   it wedges on a stale session, so the script runs it under a 45 s timeout.
   If it hangs, power-cycle back into fastboot and retry.
2. **`-S 128M` is mandatory.** The bootloader advertises a 512 MB
   `max-download-size`, but chunks that large kill the USB link partway
   through (`Write to device failed (no link)`) and leave `system`
   half-written.

## Manual equivalent

```bash
adb reboot bootloader
fastboot oem pico unlock          # once per session, may need a retry
fastboot -S 128M flash system out/system-pn2-full.img
fastboot reboot
```

!!! danger
    Only `system` (and optionally `vendor`/`oem`/`persist` via
    `restore_stock.sh`) is flashed this way.
    **Never** flash `xbl`/`abl`/`tz`/`hyp` or anything in the bootloader
    chain - a version below the anti-rollback fuse hard-bricks sdm845.
