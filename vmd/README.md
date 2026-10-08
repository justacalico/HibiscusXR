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

Options: `-img <dir>` (default `$PN2_ROOT/out`), `-novm`,
`-kernel/-dtb/-append` for a virtio-capable kernel when the guest's own
cannot run on qemu's virt machine.

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

## Known limit

The stock sdm845 kernel cannot boot on qemu's virt machine. Until the
LineageOS source build produces a generic kernel, point `-kernel` at a
virtio-capable aarch64 build; the system image and pose path are
unchanged.
