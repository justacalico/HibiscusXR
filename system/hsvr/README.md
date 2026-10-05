# hsvr

Raw OpenXR stack for the Pico Neo 2 (A7B10, sdm845) on LineageOS 17.1.

Replaces the Pico PVR application stack with upstream components:

- **Turnip** (`turnip/`) — Mesa Freedreno Vulkan driver built from source for
  the Adreno 630 over the stock kgsl kernel driver. The vendor `vulkan.sdm845`
  blob only exposes Vulkan 1.0.3; Turnip provides 1.3+ plus the external-memory
  and AHB extensions the Monado compositor needs.
- **Monado + hsvr driver kit** (`monado/`, `kit/`) — in-process
  `libopenxr_monado.so`. One monado-side prober asks the kit registry
  which headset is present; device drivers live in `drivers/<name>/`
  and plug in with zero edits here. Tracking fuses the raw BMG160 gyroscope + BMA2x2
  accelerometer through `m_imu_3dof` at the sensor's native rate; the device's
  virtual rotation-vector sensors return identity on this build and QVR
  standalone fusion never produces a pose, so neither is used.
- **Runtime APK** (`runtime-apk/`) — the system OpenXR runtime, installed into
  the image as `/system/app/MonadoOpenXR` plus an
  `etc/openxr/1/active_runtime.json` pointing at a world-readable copy under
  `/data/local/tmp/xr/`. Apps that bundle the stock Khronos
  `libopenxr_loader.so` find it automatically - no custom code needed,
  same shape as Quest.
- **xrtest** (`app-xrtest/`) — OpenXR test APK: NativeActivity + GLES2
  renderer + the stock loader, exercising the system runtime (bundled
  libopenxr_monado.so kept only as a fallback). Draws a world-locked debug
  panel with live tracking state (head pose, view flags, per-controller
  poses/buttons/stick, fps) plus marker cubes and aim rays, so the whole
  path can be verified on the panel without adb.
  The debug panel can also reproduce the positional-reprojection judder seen
  in streaming clients (ALVR/WiVRn): set
  `adb shell setprop debug.xrtest.latency_ms 50` to render the scene with a
  stale predicted pose, and `adb shell setprop debug.xrtest.depth 1` to
  submit per-pixel depth so the compositor can do positional timewarp.

## Build

```sh
turnip/build.sh      # mesa -> turnip/libvulkan_freedreno.so
monado/build.sh      # monado + pn2 driver -> libopenxr_monado.so
app-xrtest/build.sh  # test APK bundling the runtime
```

## Notes

- 6DoF camera tracking is not wired up yet; the driver is 3DoF orientation.
- `PN2_AXISMAP` / `debug.pn2.axismap` select the sensor->head axis map while
  the mapping is being verified on-device.
- `PN2_K1` / `PN2_K2` / `PN2_IPD` tune the provisional distortion model; the
  stock lens polynomial from `lens/` is the reference.
