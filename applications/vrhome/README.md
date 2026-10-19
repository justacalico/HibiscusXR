<div align="center">

# vrhome

**An open VR home environment for Hibiscus**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
![Platform](https://img.shields.io/badge/platform-Hibiscus%20%C2%B7%20Android%2010-3DDC84)

[Features](#features) • [Getting started](#getting-started) • [Controls](#controls) • [How it works](#how-it-works) • [Development](#development)

</div>

vrhome replaces the stock vendor shell on Hibiscus: no closed compositor, no closed runtime, just two small APKs on the plain Android EGL path. `vrhome.apk` is the home environment (scenery, head tracking, lens warp); `vrhud.apk` is a persistent overlay service that renders the window menu. Regular 2D apps run on virtual displays owned by the HUD and appear as floating panels - over the home scene, or summoned on top of a running VR app.

## Features

- **2D apps as floating windows.** Every app runs on its own virtual display, rendered as a textured quad with rounded corners, a soft shadow and a label pill carrying its name.
- **A menu that works inside VR games.** The HUD is a `TYPE_SYSTEM_OVERLAY` window in its own process, so pressing the headset home key pulls the panels up over a fullscreen app without restarting it - and the panels' tasks keep running.
- **Gaze and click input.** Look at a window to focus it, press the headset confirm button to tap at the gaze point, hold and move your head to scroll or drag.
- **A library that is just an app.** The app grid is a regular Android activity on its own panel, so there is no separate launcher UI to maintain.
- **VR apps stay VR.** Packages declaring `pvr.app.type=vr` launch straight to fullscreen on the physical display and hand the headset back to the shell when they exit.
- **3DoF head tracking** from the game rotation vector, with barrel-distorted eye buffers matched to the headset's panel.
- **No Gradle.** Both APKs build from a Makefile: clang, javac, d8, aapt2, zipalign, apksigner.
- **Host-testable core.** Layout, math, text and input policy are pure C++ modules exercised by `make test`.

## Requirements

- A headset running Hibiscus (Android 10)
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
>
> The HUD runs under `android.uid.system` so the panel displays can host the soft keyboard; a debug-signed build won't install at all, and upgrading an older vrhud on a live device needs one `adb uninstall gitlab.neosalsa.hud` before `make install` goes through.

## Keyboard

The IME (`gitlab.neosalsa.keyboard`, in `applications/keyboard`) floats as its own quad under the window that owns the text field, the same place the Quest keyboard hangs. Android 10 only ever docks an IME on the target's display or the default one, so the quad is not an IME window at all: the HUD owns a `Surface`/`SurfaceTexture`, hands the `Surface` to the IME over broadcast, and the IME wraps it in a private virtual display hosting a `Presentation` with the keys (presentations are only allowed on a private display owned by the caller - hence the split). The IME reports its display id back over `gitlab.neosalsa.hud.action.KBD`, the HUD draws the texture under the last-tapped panel, and touches on the quad are injected onto that display like any panel tap. If the surface never arrives the IME falls back to a docked strip inside the app panel.

No task ever lives on the keyboard display, so it is deliberately kept out of `vds` - the poller would otherwise reap it six seconds after creation.

## Controls

| Input | Action |
| ----- | ------ |
| Look at a window | Focus it; the gaze cursor tracks the panel |
| Confirm button (or ENTER / DPAD_CENTER) | Tap at the gaze point |
| Hold confirm and move your gaze | Drag or scroll the window content |
| Confirm on a pill button | – minimizes the window onto the shelf above the dock, × closes it |
| Hold confirm on a window's pill and drag | Moves the window: a docked one hops between the three slots (whoever sits there trades places), a floating one follows freely |
| BACK | Close the newest window (dismiss the menu when it is over an app) |
| Headset home key | Summon or dismiss the menu over whatever is running; hold to recenter |

At most three windows float at once, including the library. Opening another app evicts the oldest app window; the library itself is never evicted automatically. Minimized apps keep running in the background and park as icons on a shelf floating just above the dock; tapping a shelf icon (or the app's icon in the library) brings the same window back.

New apps open on the middle slot: the window already there slides to a free side, left first, and when all three slots are taken the middle window is parked on the shelf instead of evicted so the newcomer can take its place.

The status pill on the dock's left end opens the quick panel: a card inside the dash itself with the volume and brightness sliders and the toggle tiles (radios, mic, night mode, DND, airplane, battery saver). Only one card overlay is up at a time - opening the panel drops the app grid and vice versa.

The library's bar carries the × disc only: it can be closed but never minimized, and picking an app from it swaps the grid window out for the app it launched. Its dock pin brings it back.

> [!NOTE]
> The physical home key is remapped to the custom `DEFINE_HOME` keycode (1003) in `gpio-keys.kl` - stock `KEYCODE_HOME` is swallowed by system_server before any app can see it. The HUD watches for 1003 through a hidden-API input monitor.

## How it works

Two processes, each with its own native library built from one source tree:

- **Environment** (`gitlab.neosalsa.home`, `libvrhome.so`): `PanelActivity`, a `NativeActivity`, owns the physical display and renders the scenery through barrel distortion: the passthrough camera feed, the built-in sky dome, or a user-installed environment loaded from a zip package (`map.obj` + optional `map.png`/`map.json`, a `SpawnUser` part marks the standing spot). The Settings app's Home environment section picks; a `CoverWatch` poller watches the task stack; while a fullscreen app covers display 0 the loop idles on a pbuffer so VR titles get the GPU to themselves.
- **HUD** (`gitlab.neosalsa.hud`, `libvrhud.so`): `HudService` holds a fullscreen `TYPE_SYSTEM_OVERLAY` window with a `SurfaceView`, renders the panel ring in stereo over whatever is front, and reads the rotation vector itself for gaze. `ShellBridge` creates a virtual display per window, launches or adopts tasks onto it, injects input, and resolves app labels. Because the HUD process owns the displays, panel tasks survive environment restarts and stay live while a game is front.
- The headset home key arrives as keycode 1003 via a hidden `InputManager` gesture monitor. When a fullscreen app is front it toggles the overlay between hidden and focused; while focused, confirm and BACK go to the menu instead of the app. Dismissing restores focus to the covered task.

## Project structure

```
src/env/         environment entry point (native_app_glue loop), home
                 environment loader and passthrough camera
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
java/gitlab/neosalsa/hud/     HudService, HudView, ShellBridge, BootReceiver
stubs/           compile-only stubs for hidden framework classes
tests/           host unit tests
```

## Development

Platform-independent logic (matrix math, the head-tracking transform, panel slot/pick/evict policy, UTF-8 and text layout, scenery geometry, key handling) lives in pure modules under `src/` and runs against host-side unit tests:

```bash
make test
```

See `AGENTS.md` for the testing and file-size rules this repo follows.

### Desktop mode

`make preview` builds `out/dash-preview` and opens the real dash renderer in
a desktop window. The host only needs a C++ toolchain, Python 3, GLFW 3,
GLESv2 and zlib; no Android SDK or NDK. Fixture data fills it in - a grid of
apps with generated icons, dock pins and a notification - so dash changes can
be tried without a headset.

The mouse controls the gaze direction, WASD moves the free camera, Q/E move
vertically, Shift moves faster, left click is confirm, Esc parks the window
on the shelf, Shift+Esc closes it, G toggles the app grid, P toggles the
quick panel, K toggles the keyboard, and R recenters the dash.

The preview links the same layout, input, animation and draw code the headset
uses. Android services sit behind `src/shell/backend.h`; the desktop backend is
only fixture data and textures, so dash changes do not need a second UI port.

### Wayland mode

`make dash-wayland` builds `out/dash-wayland`, a third backend next to the
fixture preview: the process itself is a small Wayland compositor (SHM
surfaces and xdg-shell on `wayland-server`, no wlroots) presented in the
same GLFW window. Apps launch onto its socket as real clients - the app grid
reads the host's `.desktop` files, icons come from the freedesktop icon
theme, and window content is live, not fixtures.

```bash
make out/dash-wayland
./out/dash-wayland          # prints WAYLAND_DISPLAY=wayland-N for clients
```

Panels are toplevels the compositor spawns per launch (matched to their
display by pid or app_id; stray toplevels adopt onto their own panels).
Confirm presses become `wl_pointer` input at the panel-local point, focus
drives `xdg_toplevel` activation and `wl_keyboard` enter, and keys the dash
doesn't own are forwarded to the focused app. `` ` `` grabs the focused
app outright: while it holds every key and the mouse go straight to the
client (camera fly keys included), which is what games and other
raw-input apps need. Shift+` always releases the grab. The quick panel
talks to the
host's tools: `nmcli` for the Wi-Fi tile, `bluetoothctl` for Bluetooth,
`wpctl` for the volume slider and `brightnessctl` for brightness. Dock pins
persist to `$XDG_CONFIG_HOME/hibiscus/dash-pins`.

Host deps beyond the preview's: `wayland-server`, `wayland-protocols` (for
the xdg-shell xml `wayland-scanner` compiles at build time), `xkbcommon` and
`libpng`. Clients must render into shared memory - no dmabuf global is
advertised, so every toolkit falls back to `wl_shm`. The `-mcp` flag serves
the same MCP tools as the preview, so a headless shell can launch apps and
grab screenshots for testing.

Known gaps for later work: popups clip to the window edge instead of
sliding, clipboard/DnD are stubs, scroll arrives as a drag rather than an
axis, and presentation is the GLFW window - an OpenXR presenter can hang off
the same backend when an HMD is in the loop.

Pass `-mcp` (`./out/dash-preview -mcp`) and the window also serves MCP on
stdio: one JSON-RPC message per line, tools to drive the dash.

| Tool | Does |
| ---- | ---- |
| `dash_state` | camera pose, open panels, grid and keyboard visibility |
| `dash_look` | point the camera (`yaw` / `pitch` in degrees) |
| `dash_move` | move the camera (`x` / `y` / `z` in metres) |
| `dash_key` | send a key: `confirm`, `enter`, `back`, `dpad_center` or `button_a`; `action` is `tap` (default), `down` or `up` |
| `dash_launch` | open a `package` on a new panel |
| `dash_grid` / `dash_keyboard` / `dash_quick` / `dash_settings` | show or hide the app grid / keyboard quad / quick-settings card / settings card (`shown`) |
| `dash_recenter` | re-anchor the dash ring in front of the camera |
| `dash_screenshot` | return the rendered frame as a PNG image |

Point an MCP client at it the same way as any stdio server; when the client
closes stdin the window exits with it.

### Debugging on device

Live-tunable system properties (`setprop` on the headset):

| Property | Effect |
| -------- | ------ |
| `debug.vrhome.sensor` | `0` pins the head tracking |
| `debug.vrhome.tq` | `0` uses the untransposed sensor matrix |
| `debug.vrhome.roll` | static view roll in degrees |
| `debug.vrhome.sensroll` | sensor-frame roll correction |
| `debug.vrhome.worldx` | mount tilt correction |
| `debug.vrhome.hud` | `0`/`1` overrides the settings card's Debug HUD toggle (unset = follow it) |
| `debug.vrhome.fov` | vertical render FOV in degrees (default `90`) |
| `debug.vrhome.k0` / `k2` / `k4` / `k6` | lens warp polynomial (stock Pico coefficients; set `k0` to `1` and the rest to `0` for no distortion) |
| `debug.vrhome.cr` / `debug.vrhome.cb` | chromatic warp scales (default `0.992` / `1.012`) |
| `debug.vrhome.lensx` | lens centre offset toward the temples in eye uv (default `0.02`) |
| `debug.vrhome.lensy` | lens centre height in eye uv (default `0.5`) |
| `debug.vrhome.fill` | `1` paints each eye a different colour |
| `debug.vrhome.launch` | set to a package name to open it on a window (HUD process) |
| `debug.vrhome.tap` | `"displayId,x,y"` injects a tap (HUD process) |
| `debug.vrhome.env` | overrides the home environment pick: `passthrough`, `builtin` or an environment id |
| `persist.hibiscus.environment` | the actual pick, mirrored from the `hibiscus_environment` settings key by `pn2-envd`; custom ids load `/data/local/tmp/hibiscus/envs/<id>.zip` |

Logcat tags are `vrhome` (environment) and `vrhud` (HUD).

> [!NOTE]
> Known limits: 3DoF only, no positional tracking, no controller support, and no async reprojection, so head motion is less smooth than the stock shell.
