<div align="center">

# vrhome

**An open VR home environment for the Pico Neo 2**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
![Platform](https://img.shields.io/badge/platform-Pico%20Neo%202%20%C2%B7%20Android%2010-3DDC84)

[Features](#features) • [Getting started](#getting-started) • [Controls](#controls) • [How it works](#how-it-works) • [Development](#development)

</div>

vrhome replaces the stock Pico shell on the LineageOS 17.1 port: no Pico compositor, no closed runtime, just two small APKs on the plain Android EGL path. `vrhome.apk` is the home environment (scenery, head tracking, lens warp); `vrhud.apk` is a persistent overlay service that renders the window menu. Regular 2D apps run on virtual displays owned by the HUD and appear as floating panels - over the home scene, or summoned on top of a running VR app.

## Features

- **2D apps as floating windows.** Every app runs on its own virtual display, rendered as a textured quad with rounded corners, a soft shadow and a label pill carrying its name.
- **A menu that works inside VR games.** The HUD is a `TYPE_SYSTEM_OVERLAY` window in its own process, so pressing the headset home key pulls the panels up over a fullscreen app without restarting it - and the panels' tasks keep running.
- **Gaze and click input.** Look at a window to focus it, press the headset confirm button to tap at the gaze point, hold and move your head to scroll or drag.
- **A library that is just an app.** The app grid is a regular Android activity on its own panel, so there is no separate launcher UI to maintain.
- **VR apps stay VR.** Packages declaring `pvr.app.type=vr` launch straight to fullscreen on the physical display and hand the headset back to the shell when they exit.
- **3DoF head tracking** from the game rotation vector, with barrel-distorted eye buffers matched to the Neo 2's panel.
- **No Gradle.** Both APKs build from a Makefile: clang, javac, d8, aapt2, zipalign, apksigner.
- **Host-testable core.** Layout, math, text and input policy are pure C++ modules exercised by `make test`.

## Requirements

- A Pico Neo 2 running the LineageOS 17.1 port (Android 10)
- Android SDK and NDK
- Platform signing keys for full functionality (see below)

## Getting started

```bash
make            # build out/vrhome.apk and out/vrhud.apk
make install    # adb install -r both
adb shell cmd package set-home-activity gitlab.neosalsa.home/.PanelActivity
adb shell am startservice -n gitlab.neosalsa.hud/.HudService
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
| BACK | Close the newest window (dismiss the menu when it is over an app) |
| Headset home key | Summon or dismiss the menu over whatever is running; hold to recenter |

At most three windows float at once, including the library. Opening another app evicts the oldest app window; the library itself is never closed automatically. Minimized apps keep running in the background; tapping their icon in the library brings the same window back.

> [!NOTE]
> The physical home key is remapped to the custom `DEFINE_HOME` keycode (1003) in `gpio-keys.kl` - stock `KEYCODE_HOME` is swallowed by system_server before any app can see it. The HUD watches for 1003 through a hidden-API input monitor.

## How it works

Two processes, each with its own native library built from one source tree:

- **Environment** (`gitlab.neosalsa.home`, `libvrhome.so`): `PanelActivity`, a `NativeActivity`, owns the physical display and renders the sky scene through barrel distortion. A `CoverWatch` poller watches the task stack; while a fullscreen app covers display 0 the loop idles on a pbuffer so VR titles get the GPU to themselves.
- **HUD** (`gitlab.neosalsa.hud`, `libvrhud.so`): `HudService` holds a fullscreen `TYPE_SYSTEM_OVERLAY` window with a `SurfaceView`, renders the panel ring in stereo over whatever is front, and reads the rotation vector itself for gaze. `ShellBridge` creates a virtual display per window, launches or adopts tasks onto it, injects input, and resolves app labels. Because the HUD process owns the displays, panel tasks survive environment restarts and stay live while a game is front.
- The headset home key arrives as keycode 1003 via a hidden `InputManager` gesture monitor. When a fullscreen app is front it toggles the overlay between hidden and focused; while focused, confirm and BACK go to the menu instead of the app. Dismissing restores focus to the covered task.

## Project structure

```
src/env/         environment entry point (native_app_glue loop)
src/hud/         HUD entry point (render thread + JNI surface plumbing)
src/engine.h     environment engine state
src/common/      constants, logging, system properties, shared JNI helpers
src/math/        matrices and the head-tracking chain
src/panels/      window layout policy + panel/display lifecycle (HUD)
src/bridge/      JNI side of ShellBridge, launch queue, adopt/release (HUD)
src/render/      shaders, EGL, shared frame helpers, scenery, window chrome
src/text/        utf8, glyph layout, font atlas, text drawing
src/input/       headset button handling (HUD)
src/sensor/      rotation vector drain (environment)
java/gitlab/neosalsa/home/    PanelActivity, CoverWatch
java/gitlab/neosalsa/hud/     HudService, HudView, ShellBridge, LauncherActivity, BootReceiver
stubs/           compile-only stubs for hidden framework classes
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
| `debug.vrhome.launch` | set to a package name to open it on a window (HUD process) |
| `debug.vrhome.tap` | `"displayId,x,y"` injects a tap (HUD process) |

Logcat tags are `vrhome` (environment) and `vrhud` (HUD).

> [!NOTE]
> Known limits: 3DoF only, no positional tracking, no controller support, and no async reprojection, so head motion is less smooth than the stock shell.
