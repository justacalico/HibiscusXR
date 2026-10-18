# ovrport-compat

A small shim that makes older Oculus games run on Hibiscus.

## What is this?

OVRPort converts Oculus games to OpenXR so they can run on other headsets.
That mostly works, but games built on Unity 2018 or older never start the
OpenXR frame loop, so you just get a black screen.

This shim loads inside the game next to OVRPort's plugin and runs that
frame loop for it. No changes to the game's code are needed.

## What it does

- Runs the xrWaitFrame/xrBeginFrame/xrEndFrame cycle every frame, the part
  the old Unity build forgets to do
- Creates proper eye swapchains and submits them to the compositor
- Fixes an eye texture setup call so Unity retries it the way it expects

It only activates on a libOVRPlugin build it recognises. Everything else is
left untouched.

## Requirements

- Android NDK, with `ANDROID_NDK_HOME` set
- Python 3 with `pip install lief`
- `zipalign` and `apksigner` from the Android SDK build tools

## Build

    ./build.sh

Produces `libhsvrovrp.so`.

## Put it in a game

1. Patch the game's apk with OVRPort (overportcli).
2. Inject the shim:

       ./inject_apk.py patched.apk final.apk --zipalign /path/to/zipalign

3. Sign and install:

       apksigner sign --ks your.keystore final.apk
       adb install -r final.apk

Or let the script sign it for you:

    ./inject_apk.py patched.apk final.apk --ks ks.jks --ks-pass pass:...

## Iterate without reinstalling

    ./build.sh
    adb push libhsvrovrp.so /data/local/tmp/l.new
    adb shell "cat /data/local/tmp/l.new > <app lib dir>/libhsvrovrp.so && chmod 644 <app lib dir>/libhsvrovrp.so"
    adb shell "am force-stop <pkg> && am start -n <pkg>/<activity>"

## Logs

    adb logcat -s hsvrovrp

## How it works

The shim is added to `libOVRPlugin.so` as a `DT_NEEDED` library, so Android
loads it automatically when the game starts. It hooks a few internal plugin
functions and drives the whole OpenXR frame cycle itself inside the
plugin's `EndEye2` call. It talks to whatever OpenXR runtime the generic
loader binds, which on Hibiscus is the hsvr Monado build.
