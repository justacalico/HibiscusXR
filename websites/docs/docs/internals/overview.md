# Architecture

Hibiscus is a **GSI + overlay + user-restored-proprietary** port.

```
LineageOS 17.1 GSI (treble_arm64_avS, A-only, arm64)
        +  overlay/        our fixes: init rc's, patched libs, shim, manifests
        +  Pico stack      extracted by YOU from your own device/firmware
        =  out/system-pn2(-full).img   ->  fastboot flash system
```

## Why this shape

- The vendor partition is **untouched** - stock PUI 4.1.3, verified
  byte-identical. All compatibility problems get solved on the system side.
- The GSI must be **A-only** (the device has no A/B slots, no `super`) and must
  carry a **VNDK 27 snapshot** so the Android 10 system can talk to the
  Android 8.1 vendor.
- No Pico file is ever committed. `overlay/PROPRIETARY-PVR.md` is a manifest of
  every blob needed (path, size, sha256 prefix, purpose) so the OS builds
  without them and the end user restores them from firmware they own.

## The stack that gets restored

- `pvrservice` + `libpvrservice.so` - the main Pico VR daemon
- `qvrservice` (32-bit) - Qualcomm VR service; owns the tracking cameras and
  IMU, talks over UNIX sockets + shared memory (not binder)
- `libcompositor.pxr.so` - the closed compositor: async TimeWarp, distortion
  mesh, direct-mode present
- `libPvr_UnitySDK.so` + `libPvr_UnitySDKExt*` - the SDK games link against
- `svrapi` + `etc/pvr/` configs - lens intrinsics and runtime config
- ORB-SLAM vocabulary + `libpxr_6dof_optimization` - 6DoF solver
- the PVR apps themselves (VRShell2, CVService, launcher, ...) deodexed,
  injected and re-signed

## Key mechanisms

- **Shims** (`shim/` repo): `libshim_pvr.so` bridges the 8.1→10 ABI breaks in
  `libpvrmodule_platform.so`; `libshim_air.so` and `libskia_stub.so` cover the
  airservice and skia gaps.
- **Renamed service**: stock `qvrd` has no SELinux domain and init refuses it;
  it runs as `pn2_qvrd` (same name = duplicate rule, so renamed) with unchanged
  socket names.
- **Wakelock parity**: stock never suspends because `pvrservice` holds a kernel
  wakelock forever; we hold an equivalent one - matching stock, not fixing
  suspend.
