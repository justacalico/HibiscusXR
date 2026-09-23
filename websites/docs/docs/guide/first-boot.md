# First boot

Nothing to do - the image finishes setup on its own. The first-boot init
pass marks the device provisioned, disables the stock Pico homes and the
**Provision** setup wizard (which crashes in its language picker), and
hides app crash dialogs. OpenXR apps like WiVRn find the runtime with no
extra steps: `pn2-openxr.rc` stages it under `/data/local/tmp/xr` on every
boot, so a wiped `/data` recovers by itself.

## Optional: wireless adb

```bash
adb shell su -c 'setprop persist.pn2.adbwifi 1'   # off by default
```

## If a patch goes wrong

Every patched proprietary file keeps a `.orig` beside it on the device
(`libart.so.orig`, `libPvr_UnitySDK.so.orig`, ...), so a bad patch reverts in
place without reflashing.

## Recovery

If it will not boot at all, restore stock - see
[Restoring stock](recovery.md).
