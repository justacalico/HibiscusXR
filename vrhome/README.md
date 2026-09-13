# vrhome

VR home environment for the Pico Neo 2 running the open-source LineageOS
17.1 port. It replaces the stock Pico shell entirely: no Pico compositor,
no closed runtime, just a NativeActivity on the plain Android EGL path.

Regular 2D apps run on virtual displays and appear as floating windows in
the scene. The app library is a normal Android activity on its own window,
so there is no separate launcher UI to maintain.

## How it works

- `PanelActivity` (a NativeActivity) owns the physical display. The native
  side renders two eye buffers per frame and warps them through barrel
  distortion onto the two halves of the panel.
- Head tracking is 3DoF from the game rotation vector via `ASensorManager`.
- `ShellBridge` (Java) creates a virtual display per window, launches or
  adopts tasks onto it, injects input, and resolves app labels. A poller
  notices stray tasks stuck on display 0 and hands them to the native side
  to adopt onto a new window, and tears down displays whose task went away.
- Each window is a rounded quad with a soft shadow, a thin border, and a
  bottom bar carrying the app's display name. The gaze cursor is a small
  ring plus a dot.

## Controls

- Look at a window to focus it, press the headset confirm button (or
  ENTER / DPAD_CENTER) to tap at the gaze point.
- BACK closes the newest window.
- HOME recentres the window ring on where you're looking.

At most three windows float at once, including the library. Opening
another app closes the oldest app window; the library itself is never
closed automatically.

## Building

Needs the Android SDK and NDK. Everything resolves from
`ANDROID_SDK_ROOT` (default `/opt/android-sdk`):

```bash
./build.sh
```

No Gradle: it's clang for the native lib, javac + d8 for the Java side,
then aapt2, zipalign and apksigner. `NDK`, `BT`, `JAR` and `KEYS` env vars
override the defaults.

Output is `out/vrhome.apk`. If platform signing keys exist in
`../build/keys` the APK is signed with them and gets the system
permissions the manifest asks for; otherwise a generated debug keystore
is used and the virtual-display permissions will NOT be granted, so the
shell won't be able to host apps.

## Installing

```bash
adb install -r out/vrhome.apk
adb shell cmd package set-home-activity org.pn2.vrhome/.PanelActivity
```

## Testing

The platform-independent code (matrix math, the head-tracking transform,
panel slot/pick/evict policy, UTF-8 and text layout, scenery geometry,
key handling) is split into pure modules under `src/` and exercised by
host-side unit tests:

```bash
./test.sh
```

## Layout

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
tests/           host unit tests (see test.sh)
```

## Debugging

Live-tunable system properties (setprop on the device):

- `debug.vrhome.sensor`   0 pins the head tracking
- `debug.vrhome.tq`       0 uses the untransposed sensor matrix
- `debug.vrhome.roll`     static view roll in degrees
- `debug.vrhome.sensroll` sensor-frame roll correction
- `debug.vrhome.worldx`   mount tilt correction
- `debug.vrhome.hud`      0 hides the status line
- `debug.vrhome.fill`     1 paints each eye a different colour
- `debug.vrhome.launch`   set to a package name to open it on a window
- `debug.vrhome.tap`      "displayId,x,y" injects a tap

Logcat tag is `vrhome`.

## Known limits

- 3DoF only, no positional tracking, no controller support.
- No async reprojection; head motion is less smooth than the stock shell.
- Apps are plain 2D surfaces. VR-native apps that expect the Pico
  compositor won't run correctly here.

## Licence

AGPL v3. See `LICENSE`. Bundles `stb_truetype.h` (public domain/MIT) in
`third_party/`.
