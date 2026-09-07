# vrhome

Proof-of-concept VR home environment for the Pico Neo 2 running the
open-source LineageOS 17.1 port. Renders a ring of launchable apps in
stereo on the open EGL path, tracks the head with the IMU, and launches
the app under the gaze reticle with the headset confirm button.

This exists to prove that a home environment can be built entirely
without the Pico compositor or any closed runtime.

## What it does

- Full-screen NativeActivity, no Java code in the APK
- Stereo rendering: two eye viewports + barrel distortion in a warp pass
- 3DoF head tracking from `ASensorManager` (game rotation vector)
- App list from `PackageManager.queryIntentActivities` over JNI, refreshed
  every few seconds
- Gaze reticle picks a panel; pressing the headset confirm button
  (`DEFINE_CONFIRM`, `KEYCODE_ENTER` or `DPAD_CENTER`) launches that app
- Immersive mode hides the status and navigation bars

## Building

Needs the Android SDK and NDK. Everything is resolved from
`ANDROID_SDK_ROOT` (default `/opt/android-sdk`):

```bash
./build.sh
```

Produces `out/vrhome.apk` signed with a generated debug key. There is no
Gradle: the build is clang, aapt2, zipalign and apksigner.

## Installing

```bash
adb install -r out/vrhome.apk
adb shell am start -n org.pn2.vrhome/android.app.NativeActivity
```

To make it the system launcher pick it from Settings, or:

```bash
adb shell cmd package set-home-activity org.pn2.vrhome/android.app.NativeActivity
```

## Debugging

Live-tunable system properties:

- `debug.vrhome.sensor` - set to 0 to pin the view dead ahead

Logcat tag is `vrhome`.

## Known limits

- Panels are colour-keyed rectangles; app names go to logcat only
- No 6DoF, no controllers, no app icons or text rendering
- No async reprojection, so head motion is less smooth than the stock shell
- Launched apps open on the normal panel path, not as floating panels

## Licence

AGPL v3. See `LICENSE`.
