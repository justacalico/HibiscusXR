# controllers

Headset-independent input devices, one directory per controller type.
Everything here registers through the hsvr driver kit and attaches to
whichever headset driver won the probe, so a controller written once
works on every device the OS is ported to - a Wii Remote paired to a
Neo 2 behaves identically on a Quest port, and porting a new headset
never means porting its controllers.

Headset-native controllers (the ones that only exist because of that
headset's own tracking stack, like the PN2's CV2 wands through CVService)
stay inside their `drivers/<name>/monado/` driver.

```
controllers/<name>/
  monado/            kit controller sources (hsvr_ctrl_<name>), flattened
                     into drv_hsvr at build time; prefix files with <name>_
  proto/             optional shared wire-protocol headers, copied along
```

## The contract

Each directory exports exactly one symbol,
`const struct hsvr_controller hsvr_ctrl_<name>` (see
`system/hsvr/kit/hsvr_kit.h`):

- `name` - the directory name, e.g. `"wii"`
- `probe()` - returns >0 when this controller is usable right now
  (paired, in range). Must be cheap and side-effect free since it runs
  on every probe. Leave NULL when there is no cheap presence test - the
  controller then only activates through `HSVR_CTRL`.
- `create(index)` - builds the xrt_device for controller `index`
  (0,1,2,...), returning NULL when there are no more.

Attach order at runtime: the winning headset driver's native
controllers come first, then every generic controller that probed or
was forced, capped by `XRT_MAX_DEVICES_PER_PROBE`.

## Bring-up

`HSVR_CTRL` is a space or comma separated force list that skips probe():

```
HSVR_CTRL=wii ./openxr-app          # attach wii even when probe fails
HSVR_CTRL="wii,joycon"              # several types at once
```

That is the path for developing a controller on vmd or on a headset
where pairing is not wired up yet. `HSVR_DRIVER` still pins the headset
pick the same way it always did.

A new controller only needs its directory here - `build.sh` globs
`controllers/*/monado` into drv_hsvr and regenerates
`hsvr_controllers.c` every build, nothing else in the tree changes.
