# Repositories

Everything lives under the [neosalsa group](https://gitlab.com/neosalsa).
The main pieces:

- [tools](https://gitlab.com/neosalsa/tools) - every script for the port,
  sorted by job (backup, extract, build, patch, vr, ...)
- [android](https://gitlab.com/neosalsa/android) - the LineageOS device tree
  (`device/pico/A7B10`)
- [out](https://gitlab.com/neosalsa/out) - build output and flashing notes
- [notes](https://gitlab.com/neosalsa/notes) - the research log behind every fix
- [overlay](https://gitlab.com/neosalsa/overlay) - the files laid over the GSI,
  including the proprietary-file manifest

## Dumps

The `pvr_apps`, `oem_*`, `images`, `gsi`, `backup_nonEye`, `qvr`, `cdsp`,
`rfsa`, `seethrough`, `sensorpatch` and related repos hold references to binary
dumps of stock firmware. The binaries themselves are never committed - each of
those repos explains what the dump is and how to reproduce it from your own
device.
