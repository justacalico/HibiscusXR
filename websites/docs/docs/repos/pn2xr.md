# pn2xr

[HibiscusXR/system/pn2xr](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/pn2xr)

OpenXR runtime stack for the Neo 2 - the PVR-free path to real VR apps.

Monado provides the runtime, a custom `pn2` driver feeds it head pose from
the Android sensor HAL (raw IMU fusion, 3DoF), and a patched Mesa Turnip
`vulkan.sdm845.so` renders. Everything ships inside the system image: the
loader finds the runtime via `/product/etc/openxr/1/active_runtime.json`
(which shadows the stock vendor manifest), pointing at a `/data/local/tmp/xr`
staging dir that `pn2-openxr.rc` repopulates on every boot (app namespaces
can only `dlopen` under `/data`, and the Java helpers need a `base.apk`
next to `lib/arm64/`). The same staging dir also carries
`libqvrservice_client.so` + `libdrm.so`, the driver's last-resort path to
the qvrd 6DoF pose service.

- `monado/` - pinned Monado + `driver/pn2/` (prober, HMD, interface) applied
  via `patches/pn2-driver-registration.patch`, `build.sh` produces
  `libopenxr_monado.so`
- `turnip/` - `build.sh` clones `mesa-25.2.4` and applies
  `patches/ahb-mip-storage-fixes.patch` (AHB export sized for mip chains,
  implicit layout for multi-level images, no `GPU_DATA_BUFFER` on
  framebuffer targets) plus the sdm845 `atrace` compat shim
- `runtime-apk/` - MonadoOpenXR.apk wrapper carrying the runtime lib and its
  Java helper classes
- `app-xrtest/` - minimal OpenXR GLES client (`org.pn2.xrtest`) used to
  validate the stack end to end
- `android/active_runtime.json` - the manifest the image installs

Verified on device: WiVRn (Vulkan, mipmapped swapchains), `hello_xr`
(GLES) and xrtest all render stereo on the physical panel with live head
tracking, no PVR components involved.
</content>
