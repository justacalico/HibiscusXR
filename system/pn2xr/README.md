# pn2xr

Raw OpenXR stack for the Pico Neo 2 (A7B10, sdm845) on LineageOS 17.1.

Replaces the Pico PVR application stack with upstream components:

- **Turnip** (`turnip/`) — Mesa Freedreno Vulkan driver built from source for
  the Adreno 630 over the stock kgsl kernel driver. The vendor `vulkan.sdm845`
  blob only exposes Vulkan 1.0.3; Turnip provides 1.3+ plus the external-memory
  and AHB extensions the Monado compositor needs.
- **Monado + pn2 driver** (`monado/`) — in-process `libopenxr_monado.so` with a
  custom `pn2` xrt driver. Tracking fuses the raw BMG160 gyroscope + BMA2x2
  accelerometer through `m_imu_3dof` at the sensor's native rate; the device's
  virtual rotation-vector sensors return identity on this build and QVR
  standalone fusion never produces a pose, so neither is used.
- **xrtest** (`app-xrtest/`) — self-contained OpenXR test APK: NativeActivity +
  GLES2 renderer + the bundled runtime + the `MonadoView` Java helpers the
  runtime loads from its own APK. Used to verify the whole path on the panel.

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
