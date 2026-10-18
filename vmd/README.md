# vmd

Virtual machine driver for Hibiscus: runs the OS in qemu on a Linux PC so
changes can be tested without a headset. `vmd` is its own build device -
the dist pipeline publishes `system-hibiscus-vmd.img.xz` next to the
headset images: same GSI base and the same shell/userspace, just no Pico
hardware stack.

```
make
out/vmd -desktopsim        # window, mouse + WASD drive the HMD pose
out/vmd -novr | -pc        # same thing
out/vmd -openxr            # OpenXR app mode - a WiVRn/Monado runtime
                           # feeds a real HMD's pose in, and vmd submits
                           # the guest framebuffer to its swapchains
out/vmd -selftest          # pose channel smoke test
```

Options: `-img <dir>` (default `$HIBISCUS_ROOT/out`), `-novm`,
`-kernel/-dtb/-initrd/-append/-cpu` to override what vmd picks up from
the image dir.

The guest is aarch64 like every shipped image. On an aarch64 host KVM
accelerates it; on x86_64 it runs under TCG - correct but slow, usable
for smoke tests.

## How it connects to the OS

```
vmd (host)                       Hibiscus in qemu
  pose TCP server :7781  <----   drivers/vmd kit driver (10.0.2.2:7781)
  qemu + QMP screendump  ---->   guest framebuffer -> desktop / HMD
  hostfwd 15555      ---->       guest adbd :5555 (persist.pn2.adbwifi=1)
  hostfwd 17340      ---->       guest cted :7340 (persist.hibiscus.cted)
```

The in-OS side is a normal hsvr kit driver (`drivers/vmd/`). It is in the
image like every other driver - on a headset image it only activates
while the host channel answers, on the vmd image it is the device driver.

## Building the vmd image

```
DEVICE=vmd bash system/dist/scripts/build-image.sh
```

same inputs as usual minus the Pico stack (`gsi` + `blobs` only). The
GitHub workflow takes `device: vmd` from the dispatch form.

## Booting the guest all the way

The stock sdm845 kernel cannot run on qemu's virt machine, and the vmd
system image is a bare non-SAR partition with no init of its own.
`bootenv/make-bootenv.sh` assembles what a generic PC boot needs next
to the image:

    vmd/bootenv/make-bootenv.sh -o <image-dir>
    # writes kernel-gki, vendor.img, vmd-boot.img, userdata.img

It fetches the GKI 5.4 arm64 kernel + the virtual-device module set
(virtio blk/net/gpu/pci/...) and the API 29 vendor partition - with
the dynamic-partition fstab stripped, since a virtio disk has no
`super` - and packs `bootenv/ramdisk/` into `vmd-boot.img`: a busybox
bootstrap that loads the modules, finds the disks by content, mounts
system/vendor/data, then execs the image's own `/system/bin/init`.
`vmd -img` picks all four files up automatically.

Verified path: kernel + vmd-boot.img + the three virtio-blk disks gets
the real vmd image through first-stage init, SELinux, binderfs, apex
activation and userspace services; adbd listens on guest :5555 and
answers `adb connect localhost:15555` with a root shell. Under TCG on
x86_64 full boot takes a few minutes.

## Display

The guest drives QEMU's virtio-gpu through `/dev/dri/card0`
(`virtio-gpu-pci`, since the GKI module will not bind the mmio
transport). A DRM modeset flips the scanout to the guest framebuffer
and vmd pulls it through QMP screendump - guest-drawn pixels reach the
host window.

SurfaceFlinger itself still needs the Android emulator's qemu_pipe
host bridge, which stock QEMU does not provide, so the compositor
restarts and the boot animation does not appear. Applications and
services that only need a DRM node work.

`bootenv/tools/drmpaint.c` is a freestanding guest-side test: build it
with `aarch64-linux-gnu-gcc -static -nostdlib -ffreestanding`, `adb
push` it in, and it modesets card0 and paints a Hibiscus pattern. If
the QEMU window (or a vmd framebuffer pull) shows the pattern, the
display path is verified end to end.
