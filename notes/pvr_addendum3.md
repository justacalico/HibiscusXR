
---

# The DSP tracking stack (root cause of the black screen)

This is the single most important finding of the port. All of it is proprietary
Qualcomm/Pico content extracted from a stock Pico Neo 2 (PUI 4.1.3) or copied from
the device's own untouched `/vendor` partition.

## Why it matters

Android 8.1 let `/system` processes load libraries from `/vendor/lib`. Android 10's
Treble namespace separation does not, and the LineageOS GSI never carried the
DSP-side files at all. The result was a chain that ended in a black display:

```
libcdsprpc.so missing from /system
  -> libqvr_cdsp_driver_stub.so cannot dlopen it
  -> QVRServiceDspWrapper "UNRECOVERABLE"
  -> QVRServiceTracker: tracker module init failed
  -> "QVRService started. VR mode is not supported."
  -> QVRServiceClient_Create returns NULL
  -> supportedTrackingModes = 0
  -> pose fails the compositor's unit-quaternion check
  -> SelectRT "Bad Pose.Orientation" -> "Nothing to draw, draw black!"
```

Qualcomm's own `/vendor/bin/qvrservicetest64` is the fastest way to check this:
stock prints `starting VR mode`, a broken system prints `VR not supported`.

## Files

| Path | Size | Origin | Notes |
|---|---:|---|---|
| `/system/lib/libcdsprpc.so` | 110452 | device `/vendor/lib` | Compute DSP RPC. **Use the /vendor copy, not `libcdsprpc_system.so`** - the stub's `verneed` names SONAME `libcdsprpc.so` with version `SDSPRPC`, and the `_system` variant declares a different SONAME, so the loader rejects it on symbol versioning even if the file is renamed. Its DT_NEEDED is only liblog/libcutils/libc++/libc/libm/libdl, so nothing vendor-side follows it into the system namespace. |
| `/system/lib/libmdsprpc.so` | 110396 | device `/vendor/lib` | Modem DSP RPC. Needed by `libqvr_mapper_stub.so`; without it the mapper wrapper is NULL and the tracker still fails one step later. |

### `/system/lib/rfsa/adsp/` — the DSP-side libraries (15 files)

FastRPC loads these **onto** the Compute DSP via `remote_handle_open`. They live on
the normal filesystem but execute on the DSP. We had no `rfsa` directory at all, so
every handle came back null.

| File | Size | What it is |
|---|---:|---|
| `libtracker_6dof_skel.so` | 21272022 | **The 6DoF tracker itself, running on the DSP** |
| `libVIOMapping_6dof_skel.so` | 16356736 | Visual-inertial odometry / mapping |
| `libfastcvadsp.so` | 1277032 | FastCV DSP runtime |
| `libtobii_eyecore_skel.so` | 1198400 | Eye tracking (Neo 2 Eye) |
| `libfastcvdsp_skel.so` | 545040 | FastCV skel |
| `libscveT2T_skel.so` | 334300 | SCVE track-to-track |
| `libqvr_dsp_driver_skel.so` | 98496 | The driver `remote_handle_open` asks for |
| `libscveBlobDescriptor_skel.so` | 95748 | SCVE blob descriptor |
| `libqvr_cam_dsp_driver_skel.so` | 92344 | Camera DSP driver |
| `libapps_mem_heap.so` | 54320 | DSP heap |
| `libqvr_mapper_skel.so` | 34260 | Mapper skel |
| `libeye_tracking_dsp_sample_skel.so` | 33636 | Eye-tracking sample |
| `libdspCV_skel.so` | 33332 | dspCV skel |
| `libsns_low_lat_stream_skel.so` | 29596 | Low-latency sensor stream |
| `libdsp_streamer_qvrcam_receiver.so` | 7568 | QVR camera receiver |

## Verified result

With all of the above installed, `qvrservicetest64` reports the same as stock:

```
setting tracking mode: 3
starting VR mode
starting sensor sampling thread
gx=-0.002728 gy=-0.003430 gz=-0.005176  ax=9.570419 ay=0.144616 az=0.653827
QVRService: VR Mode started
QVRServiceDspWrapper: DSP stub dlopen successful
QVRServiceDspWrapper: qvr_dsp_driver version: 4.3.0-rbarnes-038b578
QVRServiceMapperWrapper: Mapper stub dlopen successful
```

The version string comes back **from the DSP itself**, which proves FastRPC is
working end to end.

## Still open

The VR apps continue to report `trackingstate = 0x0,0x0` even with QVR healthy.
That value is read from a cached pair of u16 fields in an SDK global
(`libPvr_UnitySDK.so`, base `0x2049b0`, `+0x570`/`+0x572`) rather than queried
live, and neither `pvrservice` nor VRShell maps any `libqvr*` library - only
`airservice` talks to QVR directly. So the SDK's tracking state is populated by
some other path that has not been identified yet. The QVR/DSP work is necessary
(cameras and 6DoF cannot work without it) but is evidently not sufficient on its own.

## Note on SELinux

Stock runs qvrservice in `u:r:qvrd:s0`. Our build cannot: init reports
`setexeccon('u:r:qvrd:s0') failed: Invalid argument`, because the vendor sepolicy
is not loaded - `/vendor/etc/selinux/precompiled_sepolicy` expects plat hash
`f6f5bf2d...` and the GSI provides `14484197...`. We run it as `u:r:shell:s0`
instead. SELinux is permissive, and the DSP came up regardless, so this does not
appear to matter for tracking - but it does mean no vendor domain is available to
any service on this build.
