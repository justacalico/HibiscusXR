<div align="center">

# vrhome

**An open VR home environment for the Pico Neo 2**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
![Platform](https://img.shields.io/badge/platform-Pico%20Neo%202%20%C2%B7%20Android%2010-3DDC84)

[Features](#features) • [Getting started](#getting-started) • [Controls](#controls) • [How it works](#how-it-works) • [Development](#development)

</div>

vrhome replaces the stock Pico shell on the LineageOS 17.1 port: no Pico compositor, no closed runtime, just a `NativeActivity` on the plain Android EGL path. Regular 2D apps run on virtual displays and appear as floating windows in the scene, while real Pico VR apps still launch fullscreen.

## Features

- **2D apps as floating windows.** Every app runs on its own virtual display, rendered as a textured quad with rounded corners, a soft shadow and a label pill carrying its name.
- **Gaze and click input.** Look at a window to focus it, press the headset confirm button to tap at the gaze point, hold and move your head to scroll or drag.
- **A library that is just an app.** The app grid is a regular Android activity on its own panel, so there is no separate launcher UI to maintain.
- **VR apps stay VR.** Packages declaring `pvr.app.type=vr` launch straight to fullscreen on the physical display and hand the headset back to the shell when they exit.
- **3DoF head tracking** from the game rotation vector, with barrel-distorted eye buffers matched to the Neo 2's panel.
- **No Gradle.** The whole APK builds from a Makefile: clang, javac, d8, aapt2, zipalign, apksigner.
- **Host-testable core.** Layout, math, text and input policy are pure C++ modules exercised by `make test`.

## Requirements

- A Pico Neo 2 running the LineageOS 17.1 port (Android 10)
- Android SDK and NDK
- Platform signing keys for full functionality (see below)

## Getting started

```bash
make            # build out/vrhome.apk
make install    # adb install -r
adb shell cmd package set-home-activity gitlab.neosalsa.home/.PanelActivity
```

Everything resolves from `ANDROID_SDK_ROOT` (default `/opt/android-sdk`); the `NDK`, `BT`, `JAR` and `KEYS` env vars override the defaults.

> [!IMPORTANT]
> If platform signing keys exist in `../build/keys`, the APK is signed with them and gets the system permissions the manifest asks for. With the fallback debug keystore the virtual-display permissions are NOT granted, so the shell can't host apps.

## Controls

| Input | Action |
| ----- | ------ |
| Look at a window | Focus it; the gaze cursor tracks the panel |
| Confirm button (or ENTER / DPAD_CENTER) | Tap at the gaze point |
| Hold confirm and move your gaze | Drag or scroll the window content |
| Confirm on a pill button | – minimizes the window, × closes it |
| BACK | Close the newest window |
| HOME | Recenter the window ring on where you're looking |

At most three windows float at once, including the library. Opening another app evicts the oldest app window; the library itself is never closed automatically. Minimized apps keep running in the background; tapping their icon in the library brings the same window back.

## How it works

- `PanelActivity` (a `NativeActivity`) owns the physical display. The native side renders two eye buffers per frame and warps them through barrel distortion onto the two halves of the panel.
- Head tracking is 3DoF from the game rotation vector via `ASensorManager`.
- `ShellBridge` (Java) creates a virtual display per window, launches or adopts tasks onto it, injects input, and resolves app labels. A poller adopts stray tasks stuck on display 0 and tears down displays whose task went away.
- The render loop suspends while a fullscreen app owns the display, so VR titles get the panel to themselves.

## Project structure

```
src/main.cpp     entry point and frame loop
src/engine.h     shared render/engine state
src/common/      constants, logging, system properties
src/math/        matrices and the head-tracking chain
src/panels/      window layout policy + panel/display lifecycle
src/bridge/      JNI side of ShellBridge, launch queue, adopt/release
src/render/      shaders, EGL, scenery, window chrome
src/text/        utf8, glyph layout, font atlas, text drawing
src/input/       headset button handling
src/sensor/      rotation vector drain
java/            ShellBridge, PanelActivity, LauncherActivity
tests/           host unit tests
```

## Development

Platform-independent logic (matrix math, the head-tracking transform, panel slot/pick/evict policy, UTF-8 and text layout, scenery geometry, key handling) lives in pure modules under `src/` and runs against host-side unit tests:

```bash
make test
```

See `AGENTS.md` for the testing and file-size rules this repo follows.

### Debugging on device

Live-tunable system properties (`setprop` on the headset):

| Property | Effect |
| -------- | ------ |
| `debug.vrhome.sensor` | `0` pins the head tracking |
| `debug.vrhome.tq` | `0` uses the untransposed sensor matrix |
| `debug.vrhome.roll` | static view roll in degrees |
| `debug.vrhome.sensroll` | sensor-frame roll correction |
| `debug.vrhome.worldx` | mount tilt correction |
| `debug.vrhome.hud` | `0` hides the status line |
| `debug.vrhome.fov` | vertical render FOV in degrees (default `90`) |
| `debug.vrhome.k0` / `k2` / `k4` / `k6` | lens warp polynomial (stock Pico coefficients; set `k0` to `1` and the rest to `0` for no distortion) |
| `debug.vrhome.cr` / `debug.vrhome.cb` | chromatic warp scales (default `0.992` / `1.012`) |
| `debug.vrhome.lensx` | lens centre offset toward the temples in eye uv (default `0.02`) |
| `debug.vrhome.lensy` | lens centre height in eye uv (default `0.5`) |
| `debug.vrhome.fill` | `1` paints each eye a different colour |
| `debug.vrhome.launch` | set to a package name to open it on a window |
| `debug.vrhome.tap` | `"displayId,x,y"` injects a tap |

Logcat tag is `vrhome`.

> [!NOTE]
> Known limits: 3DoF only, no positional tracking, no controller support, and no async reprojection, so head motion is less smooth than the stock shell.
