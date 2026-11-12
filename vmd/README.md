# vmd

Virtual machine driver for Hibiscus: runs the OS in qemu on a Linux PC
so changes can be tested without a headset. The dist binaries carry
the guest set embedded - a single `vmd-linux-<arch>` file is the whole
VM, run it and the Hibiscus Linux image boots.

```
make                   build out/vmd
make ARCH=aarch64 SYSROOT=<dir>   # arm64 cross build -> out/aarch64/vmd;
                                  # sysroot: fetch-sysroot.sh --vmd
make guest             build the guest set -> out/guest-<arch>/
make dist              embed it -> out/dist/vmd-linux-<arch>
                       (EMBED=0 ships the guest as vmd-guest-<arch>.tar.xz)

out/dist/vmd-linux-x86_64    # boots the embedded guest, no args needed
out/vmd -img out/guest-x86_64 # same thing from a plain build
out/vmd -desktopsim        # window, mouse + WASD drive the HMD pose
out/vmd -novr | -pc        # same thing
out/vmd -openxr            # OpenXR app mode - a WiVRn/Monado runtime
                           # feeds a real HMD's pose in, and vmd submits
                           # the guest framebuffer to its swapchains
out/vmd -selftest          # pose channel smoke test
out/vmd -guestinfo         # what guest this binary would boot
```

Options: `-img <dir>` (default `$HIBISCUS_ROOT/out`), `-novm`,
`-kernel/-dtb/-initrd/-append/-cpu/-machine/-qemu` to override what
vmd picks up from the image dir or its `guest.env` manifest.

The guest arch matches the binary arch, so on an aarch64 or x86_64
host KVM accelerates it; a foreign-arch guest set still boots under
TCG, just slower.

## The guest set

`vmd/vm/make-guest.sh` assembles everything a Hibiscus Linux boot
needs into four files:

```
vmlinuz        Alpine linux-virt kernel for the guest arch
initramfs.img  busybox + the virtio_blk/ext4 module set; loads the
               modules, mounts /dev/vda, switch_roots into systemd
rootfs.img     ext4 rootfs: Fedora userspace (systemd, dnf, udev,
               NetworkManager) + the system/linux staging tree -
               our daemons, units and the device profile
guest.env      manifest vmd reads: arch, qemu, machine, cpu, cmdline
```

- `-a` guest arch (default host), `-o` output dir, `-d` device
  profile (default `generic`), `-b` a local base tarball instead of
  the Fedora download.
- The kernel + busybox come from Alpine v3.20 packages, resolved
  through APKINDEX; the base is Fedora's WSL rootfs from
  releases.json, sha256-verified (the container bases ship no init,
  the WSL one is a real install).
- Assembly runs under `unshare -r` when not root so the 000-mode and
  suid files and the image's uid 0 ownership come out right.
- Downloads cache under `vmd/out/cache/<arch>` (VMD_GUEST_CACHE
  overrides); the staging tree comes from `system/linux` via
  mkimage, so `SYSROOT` feeds the aarch64 cross build.

On boot: kernel -> initramfs -> virtio-blk mount -> systemd ->
hibiscus session. Nothing sits on a login prompt: the account is
baked into the image and the console getty autologins into it, and
when the profile's xr stack is staged the shell comes up on its own
(monado.service + hibiscus-shell.service). `make guest` builds
whatever xr pieces the guest arch is missing first, so the image
arrives at the dash instead of a bare console; a host that cannot
build them still gets the console guest, and the serial keeps a
root autologin for debug either way. `VMD_GUEST_REQUIRE_XR=1`
(release pipelines set it) turns the console fallback into a hard
failure so a broken xr build can never ship inside a dist binary. The serial log lands in
`/tmp/vmd-serial-<pid>.log` and qemu's own stderr in
`/tmp/vmd-qemu-<pid>.log`; the guest framebuffer is pulled through
QMP screendump like before. If the qemu binary or a file the
manifest names is missing vmd says so and exits instead of sitting
on a black window.

`make dist` appends the guest set to the binary as
`<tar.xz><u64 size><HIBGUEST>`; at startup vmd extracts it under
`/var/tmp/vmd-guest-*` (`VMD_GUEST_TMPDIR` overrides) and boots that.
An `-img` dir with a real image set always wins over the embedded
bundle, so day-to-day dev keeps using `out/guest-<arch>`.

## How it connects to the OS

```
vmd (host)                       Hibiscus in qemu
  pose TCP server :7781  <----   drivers/vmd kit driver (10.0.2.2:7781)
  qemu + QMP screendump  ---->   guest framebuffer -> desktop / HMD
  hostfwd 15555      ---->       guest adbd :5555 (persist.hibiscus.adbwifi=1)
  hostfwd 17340      ---->       guest cted :7340 (persist.hibiscus.cted)
```

The in-OS side of the pose channel is a normal hsvr kit driver
(`drivers/vmd/`) on the Android tree; the Linux guest runs its own
profiles instead - `generic` (default) feeds the runtime a simulated
headset, `mouse`/`headset-*` profiles can drive the stack off the
virtio-input devices vmd attaches.

## The Android bootenv (legacy)

`bootenv/make-bootenv.sh` is the Android-tree path: it fetches the
GKI 5.4 arm64 kernel + virtual-device module set and the API 29
vendor partition (dynamic-partition fstab stripped), packs
`bootenv/ramdisk/` into `vmd-boot.img` - a busybox bootstrap that
loads the modules, finds the disks by content, mounts
system/vendor/data, then execs the image's own `/system/bin/init` -
and writes `kernel-gki`, `vendor.img`, `vmd-boot.img`,
`userdata.img` next to `system-hibiscus-*.img`. `vmd -img` picks all
four up automatically; `-kernel/-initrd/-append/-cpu` still
override.

    vmd/bootenv/make-bootenv.sh -o <image-dir>
    DEVICE=vmd bash system/dist/scripts/build-image.sh

Verified path on the Android tree: kernel + vmd-boot.img + the three
virtio-blk disks gets the image through first-stage init, SELinux,
binderfs, apex activation and userspace services; adbd listens on
guest :5555 and answers `adb connect localhost:15555` with a root
shell.

## Display

The guest drives QEMU's virtio-gpu through `/dev/dri/card0`
(`virtio-gpu-pci`, since the GKI module will not bind the mmio
transport). A DRM modeset flips the scanout to the guest framebuffer
and vmd pulls it through QMP screendump - guest-drawn pixels reach the
host window. For the Linux guest the xr stack lands on the same path:
lavapipe renders for monado, and the compositor's `drm` target
modesets the connector and copies each composited frame into a dumb
scanout buffer, so the dash is what the screendump picks up.

Stock QEMU has no `/dev/qemu_pipe`, so the ranchu graphics HALs in the
emulator vendor image can never present. The bootenv instead ships a
pair of freestanding HALs built from `vmd/hal/`:

- `gralloc.vmd.so` - a gralloc0 module over `/dev/ashmem`. The vendor's
  generic `allocator@2.0`/`mapper@2.0` passthrough services wrap it
  (`ro.hardware.gralloc=vmd`).
- `hwcomposer.vmd.so` - an hwc1 module that modesets one dumb KMS
  buffer, blits the client-composed framebuffer target plus any
  background/overlay layers into it, and flushes with DIRTYFB. The
  generic `composer@2.1` passthrough wraps it
  (`ro.hardware.composer=vmd`, `ro.hardware.hwcomposer=vmd`).

Both are built libc-free with `aarch64-linux-gnu-gcc -nostdlib`, so
they dlopen in any process with no dependencies. `make-bootenv.sh`
builds them and writes them into `vendor.img`'s `/lib64/hw`, and the
bootstrap ramdisk sets the `ro.hardware.*` props plus
`ro.hardware.egl=swiftshader` so guest GLES runs on SwiftShader.
The present path is verified end to end: dumb-buffer writes reach the
QEMU scanout through `DRM_IOCTL_MODE_DIRTYFB`, and QMP screendumps
show them. Guest apps compositing through SwiftShader are a separate
work in progress.

`bootenv/tools/drmpaint.c` is a freestanding guest-side test: build it
with `aarch64-linux-gnu-gcc -static -nostdlib -ffreestanding`, `adb
push` it in, and it modesets card0 and paints a Hibiscus pattern. If
the QEMU window (or a vmd framebuffer pull) shows the pattern, the
display path is verified end to end.
