<p align="center">
  <img src="hibiscusxr-icon.svg" width="96" alt="Hibiscus">
</p>

# Hibiscus

Hibiscus is a Linux distribution for standalone VR headsets: one OS
that runs on any of them, and on any PC, x86_64 or aarch64.

The idea is a single image that does not care what it boots on.
There is no per-device OS tree: a headset is a profile file plus the
standard kernel interfaces it exposes (V4L2, IIO, evdev, DRM). Apps
speak OpenXR, and the runtime builds its devices out of whatever the
box actually has, so the same build runs a headset, a dev board or
the simulated headset on a laptop.

> [!WARNING]
> Early work in progress. The Linux base has not shipped headset
> support yet; everything below describes the tree as it stands. The
> deprecated Android-based build is documented in
> [readme-android.md](readme-android.md).

## Layout

- `system/linux/` - the distro: userspace, generic drivers, device
  profiles, the image build and the OpenXR runtime
- `applications/` - the shell (`dash-wayland`) and the bundled apps
- `vmd/` - boots a Hibiscus image in qemu on a Linux PC for testing
- `system/dist`, `system/hsvr`, `drivers/` - the deprecated Android
  tree, kept as the reference for device ports until it is dropped
- `websites/` - the project site

## Building

```
cd system/linux
make                    # host build
make test               # the whole suite, on the host
images/mkimage.sh -a x86_64 -o hibiscus.tar -d generic
```

`mkimage` produces a staging tree that untars over any systemd
distro; `-b <base.tar.xz>` folds in a base rootfs (Alpine, Debian,
Fedora, anything with systemd) to produce a full image. Device
profiles live in `system/linux/devices/`:

| Profile | Runs on |
| --- | --- |
| `generic` / `sim` | Any PC, simulated headset |
| `mouse` | Any PC, a mouse drives a virtual 3-dof headset |
| `headset-iio-evdev-v4l2` | Any standalone headset on stock kernel interfaces |

Supporting a new headset means adding a profile, not forking the OS.
See `system/linux/README.md` for the architecture, the runtime and
the image build in detail.

## Links

- Project site: https://hibiscusxr-37c6a7.gitlab.io/
