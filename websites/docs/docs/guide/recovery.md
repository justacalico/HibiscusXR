# Restoring stock

Two tools, both in `tools/backup/`:

## Backing up

`backup_partitions.sh` dd-dumps every partition off a live device, most
irreplaceable first (`persist`, `picocfg`, `modemst*`, `fsg`, `fsc`, boot
images, then the big `vendor`/`oem`/`system` last). It aborts unless exactly
one device is connected, and never keeps more than one image on the device's
own storage.

```bash
PN2_SERIAL=XXXXXXX backup/backup_partitions.sh [out-dir]
```

## Restoring

`restore_stock.sh` flashes a backup back:

```bash
backup/restore_stock.sh            # boot + dtbo + vbmeta + recovery + system
backup/restore_stock.sh --all      # also vendor, oem, persist
backup/restore_stock.sh --dry-run  # print the plan, flash nothing
```

It refuses to flash a partial restore (all listed images must exist) and
**deliberately does not touch the bootloader chain** - `xbl`, `abl`, `tz`,
`hyp`, `keymaster` and friends are never written, because an older bootloader
below the anti-rollback fuse is the one true hard-brick on this chip.

`system` comes from the OTA copy rather than the dd backup - it is
byte-identical stock content.
