# Hibiscus Linux

The Linux distribution half of Hibiscus: an OS for standalone VR
headsets that also runs on any PC, x86_64 or aarch64.

The rule that shapes everything here is one direction:

```
hardware -> service -> OS
```

Drivers are the only code that talks to hardware, and they only talk
to it through kernel interfaces (V4L2, IIO, evdev, DRM). Each driver is
a service daemon that publishes a typed interface on the hsvc bus. The
OS, meaning the runtime, the shell and the tools, links `libhsvc` and
consumes services by kind. It never opens a device node. Swapping
hardware means swapping a service, and the `hsvc-sim` virtual headset
makes the whole stack runnable on any machine.

## Layout

```
include/hsvc/   proto.h      the wire protocol: frames + stream types
                hsvc.h       libhsvc: client + service socket helpers
                registry.h   hsvcd's pure service table
src/            hsvc.c, registry.c, broker.c, hsvcd.c, hsvcctl.c
drivers/        hsvc-sim     virtual headset (hmd + 2 ctrls + eye)
                hsvc-vmd     virtual motion device: a mouse is a 3-dof
                             headset, click is the trigger
                hsvc-eye     generic eye tracking camera over V4L2
                hsvc-imu     generic IMU over IIO
                hsvc-ctrl    generic controllers over evdev
devices/        *.env        device profiles: which services a box runs
units/          systemd units (hsvcd + the hsvc-svc@ template,
                             monado + the shell)
images/         mkimage.sh, fetch-sysroot.sh
xr/             build-runtime.sh + drv_hsvc, the monado driver that
                serves the bus as OpenXR devices
tests/          run.sh + unit tests
```

## The bus

`hsvcd` owns `/run/hibiscus` and listens on `daemon.sock`. A driver
binds its socket under `svc/` and REGISTERs it, holding the control
connection open for its lifetime, so a dead driver's services drop out
of the registry on their own. The OS asks the daemon to RESOLVE a kind
to a socket path, then speaks the frame protocol from
`include/hsvc/proto.h` directly.

Service kinds (`enum hsvc_kind`):

| kind | serves |
| --- | --- |
| `hmd` | head pose + display/lens info (`hsvc_pose`) |
| `ctrl` | controller pose, buttons, stick, battery (`hsvc_ctrl`) |
| `eye` | gaze per eye (`hsvc_gaze`) |
| `imu` | raw gyro/accel in SI units (`hsvc_imu`) |
| `cam` | camera frames (`hsvc_frame_hdr` + pixels) |
| `disp` | display panel state |

Resolution is by kind, never by driver name. Two drivers serving the
same kind is fine: the first registered wins RESOLVE, the rest stay
listed. That is what makes the drivers generic - the OS cannot tell
`hsvc-eye` on a real headset's IR camera apart from `hsvc-eye --sim`
on a laptop.

`HSVC_RUNDIR` overrides `/run/hibiscus` everywhere, which is how the
test suite and non-root dev runs get a private bus.

## Drivers

Every driver follows the same shape: a backend that yields hardware
samples (a device node, a replay file, or synthetic data), a thin pure
layer that turns samples into protocol structs, and `hsvc_publish` +
`hsvc_serve`.

- `hsvc-eye` - the eye tracking camera service. V4L2 grayscale frames
  in, pupil centroid per eye half, calibration maps centroid to
  normalized gaze. `--device /dev/video0` on hardware, `--file` to
  replay captured frames, `--sim` for synthetic gaze.
- `hsvc-imu` - IIO inertial sensor. Reads `in_anglvel_*`/`in_accel_*`
  raw + scale from sysfs, converts to rad/s and m/s^2.
- `hsvc-ctrl` - evdev pads/wands. Maps the standard gamepad codes to
  the generic `hsvc_ctrl` button/axis fields; `--index N` selects the
  stream id.
- `hsvc-sim` - the virtual headset. Serves hmd, both ctrls and eye
  with synthetic motion; this is what ships on the `generic` profile
  so any PC runs the real stack.
- `hsvc-vmd` - the virtual motion device. A mouse becomes a 3-dof
  headset: relative motion steers yaw/pitch, the wheel rolls, the
  side button recenters, and clicks drive the wand's trigger. Serves
  `hmd` (orientation-only pose) and `ctrl` (one wand glued to the
  head). `--evdev auto` finds the first mouse-ish node (the default),
  `--file` replays "type code value" lines, `--sim` sways.
  Standalone: when no hsvcd answers it forks the embedded broker on a
  private rundir (`$HSVC_RUNDIR`, or `/tmp/hibiscus-$UID` which it
  prints), so the one binary is the whole stack. `make dist` builds it
  as a single static file per arch - `out/dist/hsvc-vmd-x86_64` and
  `out/dist/hsvc-vmd-aarch64`, the latter linked 16k-page safe so it
  runs on 4k/16k/64k-page kernels. Static linking needs a static libc:
  libc6-dev (Debian) or glibc-static (Fedora) on the host, and
  `A64SYSROOT=<dir from fetch-sysroot.sh>` where the cross gcc is bare.

Pure logic lives in headers (`drivers/gaze.h`, `imuscl.h`,
`ctrlmap.h`, `vmdmap.h`) with zero device code so every line of it is
unit tested on the host.

## Device profiles

A device is data, not code: `devices/<name>.env` names the service
instances the box runs and their arguments. `mkimage` turns that into
`/etc/hibiscus/device.env`, per-service env files and enabled
`hsvc-svc@<name>` units. Adding support for a new headset means adding
a profile that points the generic drivers at its nodes - or, when the
hardware needs it, one more driver binary that speaks the same
protocol.

- `generic` / `sim` - PC/dev board, virtual headset only
- `mouse` - PC where a physical mouse drives the virtual 3-dof
  headset through `hsvc-vmd`
- `headset-iio-evdev-v4l2` - any standalone headset on stock kernel
  interfaces (IIO + evdev + V4L2)

Every profile also sets `HSVC_XR=1`, which tells mkimage to stage the
OpenXR runtime and enable the shell when the artifacts exist.

## The runtime

The bus is the device contract; OpenXR is the app contract. `xr/` is
the piece that joins them: `drv_hsvc` is a monado driver whose
auto-prober asks the bus what exists and builds an xrt device per
service - the hmd service's pose stream plus its INFO display keys
become the head (views, fov, distortion), every ctrl service becomes
a controller. Nothing in it knows which drivers are running, so a
sim, a dev PC and a shipping headset all present identically.

`xr/build-runtime.sh` pins the same monado commit the Android stack
used, applies `xr/patches/hsvc-driver.patch` (cmake registration, no
android bits), drops in the driver + libhsvc sources, and produces
`out/xr/`: `libopenxr_monado.so`, `monado-service`, `monado-cli` and
an `active_runtime.json`. Build flags turn every hardware driver off -
the bus is the only device contract; `SIMULATED` stays as the fallback
when nothing serves it.

```
xr/build-runtime.sh                  # -> out/xr/
make shell                           # builds the dash for ARCH (same
                                     # aarch64 cross path as `make`)
                                     # and stages out/xr/bin/dash-wayland
```

`make shell` wraps `make dash-wayland` in `applications/vrhome` with the
same `ARCH`/`SYSROOT` contract as the rest of the tree, so
`make shell ARCH=aarch64 SYSROOT=<dir>` produces the shell for a
headset-class image. `fetch-sysroot.sh <dir> --shell` fetches the
sysroot dep set the dash's cross build needs.

`mkimage -x` (or the default `out/xr` path) stages the runtime at
`/usr/lib/libopenxr_monado.so` + `/usr/bin/monado-service`, the
manifest at `/etc/openxr/1/active_runtime.json`, and a `dash-wayland`
binary when one is dropped into `out/xr/bin/`; the profile's
`HSVC_XR=1` then enables `monado.service` and
`hibiscus-shell.service` (`dash-wayland -vr`). Every staged ELF is
checked against `-a`, so a host-arch runtime or shell can never slip
into a foreign-arch image - the lane warns and skips instead. The shell
is just an OpenXR client - the same image also runs any other OpenXR
app.

## Images

```
images/mkimage.sh -a x86_64 -o hibiscus.tar -d generic
SYSROOT=<dir> images/mkimage.sh -a aarch64 -b fedora-base.tar.xz \
    -o hibiscus-arm.tar -d headset-iio-evdev-v4l2
```

`-b` takes any distro rootfs tarball (Alpine, Debian, Fedora - anything
with systemd); without one the output is a staging tree containing our
binaries, units and profiles, which is what CI verifies. `SYSROOT` in
the environment feeds the cross build on hosts with a bare cross gcc.
`images/fetch-sysroot.sh` produces the pinned Fedora aarch64 sysroot
used for cross builds; `--shell` adds the dash's dep set.

```
make                    host build
make ARCH=aarch64 SYSROOT=<dir>    cross build
make test               the whole suite, on the host
make dist               static single-file hsvc-vmd per arch
make shell              build + stage the dash-wayland shell for ARCH
```

## Tests

`tests/run.sh` is the gate: unit tests over the codec, registry and
driver pure logic; a live bus run that starts `hsvcd`, the sim
headset and each driver on file replay and exercises them through
`hsvcctl` exactly like the OS does; an mkimage assembly check; and the
aarch64 cross build when the compiler is installed.

## Status

The Android-based tree (`system/dist`, `system/hsvr`, `drivers/pn2`,
`drivers/vmd`) is the reference implementation this layer replaces.
The runtime path now exists end to end: `drv_hsvc` turns the bus into
OpenXR devices, `monado.service` + `hibiscus-shell.service` run the
dash on images whose profiles set `HSVC_XR`. Verified on the host:
hsvcd + hsvc-sim feed a tracked head and two controllers through the
runtime's OpenXR interface. Still to land: real device profiles, eye
gaze as an input, and panel tuning keys per device.
