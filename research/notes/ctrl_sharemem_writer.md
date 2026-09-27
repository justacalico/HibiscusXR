# CtrlShareMem writer semantics (CVService)

Decompiled `/system/priv-app/CVService/lib/arm/libCVController.so` and
measured on-device (Pico Neo 2, this build).

## Write loop

`PVR::setFusedPoseThreadMain` runs while the CV controller thread lives
(`setFusedPoseThread` flag, set by `nativeStartControllerThread`, cleared
only when the SpiSensor object dies). Each pass pulls `EmImuDataProcess`
state and calls `WriteInfo2SharMemory`, which brackets every section
write with the flag bytes - all four flags (0, 100, 512, 612) flap once
per pass.

Measured on-device: ~30 ms per pass (33-35 writes/s), the flag sits at
WRITING only ~7 us, and write gaps spike to ~45-85 ms. The decoded
content is byte-identical between writes when the controller state does
not change, so a content hash is NOT a liveness signal - only the flag
flaps are. `ctrl_probe` in `ctrl_state.c` catches them by spin-sampling
on a throwaway thread (a sync spin would stall the caller for a full
write period; a single read almost never lands inside the ~7 us window).

## Thread lifecycle - who starts it

The SPI/write thread does NOT start by itself. It starts on:

- binder `startCVControllerThread(head, hand)` - the settings app calls
  `startCVControllerThread(1, 1)`;
- broadcast `com.picovr.picovrlib.cv.broadcast.start.thread` with
  `HeadTrackMode`/`CtrlerTrackMode` int extras -> `MSG_START_SPI_THREAD`
  -> `nativeStartControllerThread`.

Every start also `nativeStopControllerThread()`s a live thread first, so
a repeated broadcast is a full ~5 s re-init - never poke a live channel.

On this build stock VRShell is disabled and nothing sends the start
broadcast at boot - the channel stays dead until something pokes.
`RemoteService` also crash-loops; on each respawn the service re-maps
the sharebuffer but the thread does not restart - writes stay silent.

vrhome's HudService now self-heals: when the flags stop flapping for
>3 s the render thread pokes the broadcast (rate-limited to 30 s), which
both boots the pipeline cold and revives it after a RemoteService crash.

## Connect / disconnect

- Connect: `emSpiSensorThreadMain` calls `resetShareMemData(which)` once
  (writes an all-zero block), then the loop fills real data. A slot that
  reads all-zero apart from the flag bytes has no controller on it.
- Disconnect: `txImuInterruptProcess` does `SetconnectState(i,0)` +
  `ClearIMUData(i)` + `ndiexec_reset` + `ndiexec_set_home_poses`, but the
  write loop keeps publishing. The block ends up holding stale keys and
  the home/sentinel pose - nonzero, byte-stable, and indistinguishable
  in-band from a parked controller. Per-controller disconnect is not
  visible in the file; the authoritative signal is the binder call
  `getCV2ControllerConnectionState(i)` (what the settings app polls).
- With 6DoF fusion down (note 375), a connected controller writes the
  default sentinel pose `(-300,+-100,-300, q0=-1)` - "connected but not
  tracked" on the wire.

## Related

- `375_ctrl_6dof_blocker.md` - why the fused pose is a sentinel.
- `pvr_addendum*.md` - RemoteService crash loop notes.
