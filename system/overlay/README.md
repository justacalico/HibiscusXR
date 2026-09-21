# Pico Neo 2 (A7B10) system overlay

Everything here goes into the shipped `system.img`. `/vendor` is **not** modified:
verified byte-identical to stock (`/vendor/manifest.xml` = 22604 bytes with the
boot HAL declaration still in it). That matters for distribution, since the end
user supplies their own proprietary vendor partition.

Target: LineageOS 17.1 / Android 10 GSI (`treble_arm64_avS`) on the stock
Pico 8.1 vendor. VNDK 27, non-A/B, kernel 4.9.65.

## Files

| Path | Purpose |
|---|---|
| `etc/init/pn2-vintf.rc` | bind-mounts the patched VINTF manifest at `early-init` |
| `etc/pn2/vendor_manifest.xml` | stock manifest minus the `android.hardware.boot` block |
| `etc/init/pn2-snd.rc` | loads the audio kernel modules once the runtime APEX is up |
| `etc/init/pn2-settings.rc` | pins rotation on every boot (survives factory reset) |
| `lib64/libsensorservice.so` | one-instruction patch; without it any sensor reboots the device |
| `props.append` | append verbatim to `/system/build.prop` and `/system/etc/prop.default` |

## Why each one exists

**1. Boot HAL (`pn2-vintf.rc`)** — stock `/vendor/manifest.xml` declares
`android.hardware.boot@1.0::IBootControl/default`, but the vendor ships no
implementation and the device is not A/B at all (no `slot_suffix`, no `_a`/`_b`
partitions; it even sets a false `ro.build.ab_update=true`). Android 8.1 never
called it. Android 10 vold calls it every boot from `cp_needsCheckpoint()`, and
libhidl blocks forever on an interface that VINTF declares but nobody registers.
vold's main mutex is held, every later `remountUid`/`setListener` queues behind
it, and `StorageManagerService` trips the watchdog — system_server is killed
about every 80 seconds, permanently. Shadowing the one file leaves the partition
untouched.

**2. Audio modules (`pn2-snd.rc`)** — the sdm845 audio drivers are modules in
`/vendor/lib/modules`, and stock loads them at ~2.7s via an init exec of
`/vendor/bin/modprobe`. Under Android 10 that fails with
`cannot execve('/vendor/bin/modprobe'): No such file or directory`, because it is
`toybox_vendor`, dynamically linked against bionic in the runtime APEX, which
apexd has not mounted that early. Nothing retries, so no ALSA card ever
registers, the vendor audio HAL spins forever in
`audio_extn_utils_get_snd_card_num()`, audioserver is killed by its own 5s
TimeCheck on repeat, `media.audio_policy` never publishes, and boot stalls at
"Phone is starting". Retrying on `apexd.status=ready` fixes it.

Note: `/etc/modprobe.d` and `/etc/modprobe.conf` are **not** required. I added
them earlier chasing the wrong error (that message came from running modprobe by
hand, not from init) and removed them again after a cold boot proved audio comes
up without them.

**3. Rotation (`pn2-settings.rc` + props)** — the panel reports 2160x3840
portrait but is physically mounted landscape spanning both eyes, so a portrait UI
renders across the seam and is unreadable in the headset.
`ro.surface_flinger.primary_display_orientation` makes landscape the *natural*
orientation, so the boot animation, system UI and every app inherit it. The two
`Settings.System` values live in `/data` and would revert on a factory reset,
hence the init service re-applying them each boot.

`seclabel u:r:shell:s0` in that service is load-bearing. Without it init refuses
to start it — *"does not have a SELinux domain defined"* — because a service run
from `/system/bin/sh` as a non-root user has no domain transition. Being globally
permissive does not help; init bails before that matters.

**4. A2DP offload (props)** — vendor advertises
`persist.vendor.bt.a2dp_offload_cap=sbc-aac` but exposes no offload formats, so
`AudioPolicyManager::getHwOffloadEncodingFormatsSupportedForA2DP()` dereferences
null and audioserver SIGSEGVs.

## Verified result

Cold boot, no manual intervention:

```
boot_completed  : 1  (31s)
watchdog kills  : 0
audioserver     : 0 crashes
snd modules     : 8
sound card      : 0 [sdm845tavilsndc] sdm845-tavil-snd-card
media.audio_policy : found
wm size         : 3840x2160
user_rotation   : 0   accelerometer_rotation : 0
```

**5. Sensor type assert (`lib64/libsensorservice.so`)** — Pico's HAL emits sensor
event types 57, 58, 126 and 127. Android 10's `convertToSensorEvent()` fatally
`CHECK`s on any type above the standard range (35) and below
`DEVICE_PRIVATE_BASE` (65536), so system_server dies and the device reboots.
Observed 14 restarts in 63 seconds when a fused sensor was enabled, and again the
instant the camera app touched the tracking cameras. Android 8.1 never checked,
which is why stock was fine.

The whole check is one conditional branch:

```
2bea8: cmp   w19, #16, lsl #12      ; compare with 65536
2beac: b.lt  0x2bef8                ; below -> abort path
2beb0: ldp   q1, q0, [x0, #48]      ; fall-through: copy payload, return
```

`tools/65_patch_sensor.py` replaces the `b.lt` at file offset `0x2beac` with a
`nop`, so out-of-range types take the existing private-sensor payload path
instead of aborting. That restores the 8.1 behaviour, and nothing listens to
those sensors anyway. Deliberately a single 4-byte edit — the smaller the change,
the easier it is to argue it is correct. The script verifies the preceding `cmp`
as an anchor and refuses to patch if either instruction differs, so it will fail
loudly rather than corrupt a different build.

There is a **second** fatal CHECK in the same function, in the
`DYNAMIC_SENSOR_META` case:

```
'Check failed: it != mConnectedDynamicSensors.end()'
```

Pico reports a dynamic-sensor *connect* for a handle sensorservice never
registered, so the inlined `unordered_map::find` misses and Q aborts. Opening
ALVR triggers it, and system_server then crash-loops every ~5 s — 45 aborts in
one boot, which is what made boot appear to hang on the animation.

That one cannot be NOPed: falling through would run
`dst->...sensor = it->second` on an invalid iterator. Instead the miss is
redirected to `0x17d58`, which is already the address the function branches to
when `connected == false` — so "skip filling in sensor/uuid and carry on" is
behaviour the code implements itself.

```
17cf0:  adrp x1, 0xa000   ->   b 0x17d58
```

Result: sensors and cameras both work, `system_server` stable, IMU at 100 Hz,
ALVR no longer reboots the device, cold boot 41 s with 0 aborts of either kind.

Reproduce with:

```
python3 tools/65_patch_sensor.py libsensorservice.so.orig libsensorservice.so
```

**6. PVR ABI shim (`lib64/libshim_pvr.so`)** — this is our code, not a blob, and
it is what lets Pico's 8.1 VR service run on the Android 10 framework.

Of the whole PVR stack, exactly one library is ABI-broken:
`libpvrmodule_platform.so`. `libcompositor.pxr.so`, `libruntime.pxr.so`,
`libpxr_6dof_optimization.so` and the rest import nothing Q removed. The platform
module breaks in two separate ways:

*Missing symbols.* `SurfaceComposerClient::getBuiltInDisplay(int)` and the
one-argument `DisplayEventReceiver(VsyncSource)` constructor were both removed in
Q. The shim forwards them to `getInternalDisplayToken()` and to the two-argument
constructor with `eConfigChangedSuppress`. Written in assembly, because both have
C++ ABI details a C prototype cannot express on AArch64 — `sp<IBinder>` returns
through the hidden `x8` pointer, and the constructor takes `this` in `x0`. A tail
branch preserves all of it.

*Changed struct layout.* Fixing the symbols only got it further: it then died
with `stack corruption detected (-fstack-protector)` inside `PVR::receiver()`.
`DisplayEventReceiver::Event` grew from 24 to 32 bytes between 8.1 and 10
(`Header.id` uint32 became `displayId` uint64, plus a `Config` variant). Pico
reads N events into a stack array sized with the 24-byte layout; Q writes N×32
into it. The shim intercepts `getEvents`, reads into a private Q-layout buffer,
and repacks into the 8.1 layout the caller actually allocated.

This is the general lesson for the port: a symbol shim is not enough when a C++
type's *size* changed. Anything that reads structs out of the framework needs
checking, not just linking.

Result: `pvrservice` runs stably with `libpvrmodule_platform.so` and
`libpvrmodule_orientationtracker.so` loaded, emitting live pose.

Note: `libHeadImuCalibrate_int.so`, `libpvrmodule_externalhmd.so` and
`libpvrmodule_controller.so` are logged as "failed to load" — they do not exist
in the stock image either. Pico probes for optional modules this SKU does not
ship. Not errors.

## Still outstanding

- **Latency.** Rendering goes through normal SurfaceFlinger compositing with no
  direct mode and no timewarp, so there is noticeable motion-to-photon lag. The
  fixes are AOSP vrflinger (`IComposer/vr` is already registered on this device)
  or porting Pico's own compositor.
- **PVR app compatibility.** `/vendor` kept the QVR/PVR client libraries, but the
  `pvrservice`/`qvrservice` daemons, `libcompositor.pxr.so`, `pxr_sdk_api.jar`,
  `/etc/pvr/` and the Pico apps all lived on the stock `/system`. They are
  inventoried in `notes/63_pvr_inventory.txt` and still need restoring.
- Not yet folded into a flashable `system.img`; this is still an overlay applied
  to a running device.
