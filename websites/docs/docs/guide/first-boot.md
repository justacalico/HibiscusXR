# First boot

The stock Pico launcher loops trying to start **Provision** (the setup
wizard), which crashes in its language picker
(`IndexOutOfBounds` in `Language1Adapter`). Disable it once and the launcher
skips setup and hands straight off to VRShell:

```bash
adb shell su -c 'pm disable com.picovr.provision'
adb shell su -c 'settings put global hide_error_dialogs 1'
```

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
