# out

[HibiscusXR/system/out](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/out)

## What this is

Build output. system-pn2.img / system-pn2-full.img are the flashable LineageOS 17.1 system images for Pico Neo 2 (the GSI plus our overlay written in with debugfs), and dev_sensor.so / dev_shim.so are our own compiled shim libraries.

## How to remake this dump

tools/142-147: copy gsi/gsi_raw.img, `debugfs -w` the overlay files into it, then tools/147_flash_full.ps1 flashes the result. The full details are in the README below.

## Files

4 files / 共 4 个文件 / всего файлов: 4

```
   0.2 KiB  dev_sensor.so
   0.0 KiB  dev_shim.so
   0.0 GiB  system-pn2-full.img
   0.0 GiB  system-pn2.img
```

---

## License

The built images combine the third-party LineageOS GSI (under its own license) with our own original work. The AGPL v3 in this repository applies to this README and our own files only (shims, overlay files, scripts). LineageOS content stays under its own license, and no Pico proprietary content is shipped here.

---

See the repo README for the full bug-by-bug breakdown of the port.
