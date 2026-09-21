# vrdemo

[HibiscusXR/applications/vrdemo](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/applications/vrdemo)

Minimal stereo VR renderer - the "does the open path work at all" smoke test.

It deliberately does **not** use the Pico/Qualcomm stack: standard EGL +
GLES2 on a normal fullscreen surface, split into two eye viewports with barrel
distortion in the fragment shader, and 3DoF orientation from the IMU via the
NDK-stable `ASensorManager`. If it looks right through the lenses, the display,
the Adreno driver and the sensor HAL are all usable without any closed
compositor.

- Pure `NativeActivity`, no Java - the APK is a manifest plus one `.so`, so
  the build is just `aapt2` + `zipalign` + `apksigner` (no Gradle)
- Panel is 2160x3840 portrait; landscape is forced so each eye gets half of a
  3840x2160 frame
- No timewarp, reprojection or positional tracking - those belong in a real
  runtime, not a smoke test

```
src/main.cpp        the renderer
AndroidManifest.xml org.pn2.vrdemo
out/                built apk stages (unsigned -> aligned -> signed pn2vr.apk)
```
