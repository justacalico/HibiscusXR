# CtrlShareMem writer semantics (CVService)

Decompiled `/system/priv-app/CVService/lib/arm/libCVController.so`
(stock + hsvr-patched builds share this code).

## Write loop

`PVR::setFusedPoseThreadMain` runs while the CV controller thread lives
(`setFusedPoseThread` flag, set by `startCVControllerThread` /
the `com.picovr.picovrlib.cv.broadcast.start.thread` broadcast with
`HeadTrackMode`/`CtrlerTrackMode` extras, cleared only when the SpiSensor
object dies).

Each pass, `usleep(4000)` (~250 Hz):

1. pulls `EmImuDataProcess` state (head pose, per-controller IMU, fused
   poses, key bytes);
2. calls `WriteInfo2SharMemory` (Java, `CVControllerService`), which for
   each controller writes the pose flag (1 then 2), pose fields, key flag
   (1 then 2), key fields - all four flag bytes flap every pass.

The writes are unconditional: a parked controller produces byte-identical
frames while the wire keeps moving. Content change is therefore NOT the
link heartbeat - the flag flaps are. `ctrl_share_write_edge()` in
`ctrl_state.c` spin-samples them; vrhome's `ctrl_live` and the pn2
monado driver use that as liveness.

## Connect / disconnect

- Connect: `emSpiSensorThreadMain` calls `resetShareMemData(which)` once
  (writes an all-zero block), then the loop fills real data. A slot that
  reads all-zero apart from the flag bytes has no controller on it.
- Disconnect: `txImuInterruptProcess` does `SetconnectState(i,0)` +
  `ClearIMUData(i)` + `ndiexec_reset` + `ndiexec_set_home_poses`, but the
  write loop keeps publishing. The block ends up holding stale keys and
  the home/sentinel pose - nonzero, byte-stable, and indistinguishable
  in-band from a parked controller. Per-controller disconnect is simply
  not visible in the file; the authoritative signal is the binder call
  `getCV2ControllerConnectionState(i)` (what the settings app polls).
- With 6DoF fusion down (note 375), a connected controller writes the
  default sentinel pose `(-300,+-100,-300, q0=-1)`, which is what
  "connected but not tracked" looks like on the wire.

## Related

- `375_ctrl_6dof_blocker.md` - why the fused pose is a sentinel.
- `pvr_addendum*.md` - RemoteService crash loop notes.
