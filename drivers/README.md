# drivers

One directory per supported device. This is the provider layer: the OS and
the monado runtime are written once against the hsvr driver kit and never
change when a device is added - a new device is a new directory here.

```
drivers/<name>/
  driver.json        provider manifest: detect rules, payload, build steps
  monado/            kit driver sources (hsvr_drv_<name>), flattened into
                     drv_hsvr at build time; prefix files with <name>_
  scripts/           optional per-device image scripts referenced by
                     driver.json
```

Input devices that are not bound to one headset (Bluetooth wands,
gamepads, anything usable on every port) do not belong here - they live
in `controllers/` at the repo root and attach to whatever driver wins.
See `controllers/README.md`.

`driver.json` fields:

- `detect.sysprops` - props that identify the hardware
- `detect.channel` - host channel signature (vmd-style virtual devices)
- `monado` - the kit driver symbol suffix (`hsvr_drv_<monado>`)
- `image.inputs` - fetch-inputs pin names this device needs
- `image.steps.stage` / `image.steps.inject` - ordered `{run,log,marker}`
  entries the build pipeline runs; paths under `tools/` resolve inside
  `$PN2_ROOT`, anything else resolves inside the driver dir

`tools/provider.py` is the resolver the pipeline calls. `HSVR_DRIVERS`
limits which drivers are active (space separated); unset means all.
